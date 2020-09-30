// Fill out your copyright notice in the Description page of Project Settings.


#include "UTouchEngine.h"
#include <vector>
#include "TEDXGITexture.h"
#include "DynamicRHI.h"
#include "D3D11RHI.h"
#include "D3D11StateCachePrivate.h"
#include "D3D11Util.h"
#include "D3D11State.h"
#include "D3D11Resources.h"
#include <D3D11RHI.h>
#include <D3D12RHIPrivate.h>

#pragma comment(lib, "d3d11.lib")

void
UTouchEngine::BeginDestroy()
{
	clear();

	Super::BeginDestroy();
}

void
UTouchEngine::clear()
{
	myCHOPOutputs.Empty();
	{
		std::lock_guard<std::mutex> guard(myTOPLock);
	}

	ENQUEUE_RENDER_COMMAND(void)(
		[immediateContext = myImmediateContext,
			d3d11On12 = myD3D11On12,
			cleanups = myTexCleanups,
			context = myContext,
			instance = myInstance]
		(FRHICommandListImmediate& RHICmdList) mutable
		{
			cleanupTextures(immediateContext, &cleanups, FinalClean::True);

			if (immediateContext)
				immediateContext->Release();

			if (d3d11On12)
				d3d11On12->Release();

			TERelease(&context);
			TERelease(&instance);
		});

	myTexCleanups.clear();
	myImmediateContext = nullptr;
	myContext = nullptr;
	myInstance = nullptr;
	myDevice = nullptr;
	myD3D11On12 = nullptr;
}


const FString&
UTouchEngine::getToxPath() const
{
	return myToxPath;
}

static void
eventCallback(TEInstance * instance,
			TEEvent event,
			TEResult result,
			int64_t start_time_value,
			int32_t start_time_scale,
			int64_t end_time_value,
			int32_t end_time_scale,
			void * info)
{
	auto *window = static_cast<UTouchEngine*>(info);

	if (result != TEResultSuccess)
	{
		int i = 32;
	}
	switch (event)
	{
		case TEEventInstanceDidLoad:
			window->setDidLoad();
			break;
		case TEEventParameterLayoutDidChange:
			break;
		case TEEventFrameDidFinish:
			break;
		case TEEventGeneral:
			// TODO: check result here
			break;
		default:
			break;
	}
}

void
UTouchEngine::parameterValueCallback(TEInstance * instance, const char *identifier, void * info)
{
	UTouchEngine *doc = static_cast<UTouchEngine *>(info);
	doc->parameterValueCallback(instance, identifier);
}

void
UTouchEngine::cleanupTextures(ID3D11DeviceContext *context, std::deque<TexCleanup> *cleanups, FinalClean fa)
{
	while (!cleanups->empty())
	{
		auto &cleanup = cleanups->front();
		BOOL result = false;
		context->GetData(cleanup.query, &result, sizeof(result), 0);
		if (result)
		{
			TERelease(&cleanup.texture);
			cleanup.query->Release();
			cleanups->pop_front();
		}
		else
		{
			if (fa == FinalClean::True)
			{
				Sleep(1);
			}
			else
			{
				break;
			}
		}
	}
}

void
UTouchEngine::parameterValueCallback(TEInstance * instance, const char *identifier)
{

	TEParameterInfo *param = nullptr;
	TEResult result = TEInstanceParameterGetInfo(instance, identifier, &param);
	if (result == TEResultSuccess && param->scope == TEScopeOutput)
	{
		switch (param->type)
		{
#if 0
		case TEParameterTypeDouble:
		{
			double value;
			result = TEInstanceParameterGetDoubleValue(myInstance, identifier, TEParameterValueCurrent, &value, 1);
			break;
		}
		case TEParameterTypeInt:
		{
			int32_t value;
			result = TEInstanceParameterGetIntValue(myInstance, identifier, TEParameterValueCurrent, &value, 1);
			break;
		}
		case TEParameterTypeString:
		{
			TEString *value;
			result = TEInstanceParameterGetStringValue(myInstance, identifier, TEParameterValueCurrent, &value);
			if (result == TEResultSuccess)
			{
				// Use value->string here
				TERelease(value);
			}
			break;
		}
#endif
		case TEParameterTypeTexture:
		{
			// Stash the state, we don't do any actual renderer work from this thread
			//std::lock_guard<std::mutex> guard(myMutex);
			//myPendingOutputTextures.push_back(identifier);
			TETexture *dxgiTexture = nullptr;
			result = TEInstanceParameterGetTextureValue(myInstance, identifier, TEParameterValueCurrent, &dxgiTexture);
#if 0
			if (result == TEResultSuccess)
			{
				TETextureType type = TETextureGetType(dxgiTexture);
				switch (type)
				{
					case TETextureTypeDXGI:
						//result = TED3DContextCreateTexture(myContext, dxgiTexture, &d3dTexture);
						break;
					default:
						// error
						break;

				}
				int sdjfh = 3;
			}
#endif

			const char* truncatedName = strchr(identifier, '/');
			if (!truncatedName)
				return;

			FString name(truncatedName + 1);
			ENQUEUE_RENDER_COMMAND(void)(
				[this, name, dxgiTexture](FRHICommandListImmediate& RHICmdList) 
				{

					cleanupTextures(myImmediateContext, &myTexCleanups, FinalClean::False);

					TED3DTexture *teD3DTexture = nullptr;

					std::lock_guard<std::mutex> guard(myTOPLock);

					TED3DContextCreateTexture(myContext, dxgiTexture, &teD3DTexture);

					if (!teD3DTexture)
					{
						TERelease(&dxgiTexture);
						return;
					}

					ID3D11Texture2D *d3dSrcTexture = TED3DTextureGetTexture(teD3DTexture);

					if (!d3dSrcTexture)
					{
						TERelease(&teD3DTexture);
						TERelease(&dxgiTexture);
						return;
					}

					D3D11_TEXTURE2D_DESC desc = { 0 };
					d3dSrcTexture->GetDesc(&desc);


					if (!myTOPOutputs.Contains(name))
					{
						myTOPOutputs.Add(name);
					}

					auto &output = myTOPOutputs[name];

					if (!output.texture ||
						output.w != desc.Width ||
						output.h != desc.Height)
					{
						if (output.wrappedResource)
						{
							output.wrappedResource->Release();
							output.wrappedResource = nullptr;
						}
						output.texture = nullptr;
						output.texture = UTexture2D::CreateTransient(desc.Width, desc.Height);
						output.w = desc.Width;
						output.h = desc.Height;
						FTexture2DMipMap& Mip = output.texture->PlatformData->Mips[0];
						unsigned char* data = (unsigned char*)Mip.BulkData.Lock(LOCK_READ_WRITE);
						for (int i = 0; i < output.w * output.h; i++)
						{
							data[0] = 255;
							data[1] = 128;
							data[2] = 128;
							data[3] = 255;
							data += 4;
						}
						Mip.BulkData.Unlock();
						output.texture->UpdateResource();
					}
					UTexture2D *destTexture = output.texture;

					if (!destTexture->Resource)
					{
						TERelease(&teD3DTexture);
						TERelease(&dxgiTexture);
						return;
					}

					ID3D11Resource* destResource = nullptr;
					if (myRHIType == RHIType::DirectX11)
					{
						FD3D11TextureBase* d3d11Texture = GetD3D11TextureFromRHITexture(destTexture->Resource->TextureRHI);
						destResource = d3d11Texture->GetResource();
					}
					else if (myRHIType == RHIType::DirectX12)
					{
						FD3D12TextureBase* d3d12Texture = GetD3D12TextureFromRHITexture(destTexture->Resource->TextureRHI);
						if (!output.wrappedResource)
						{
							if (myD3D11On12)
							{
								D3D11_RESOURCE_FLAGS flags = {};
								flags.MiscFlags = 0; // D3D11_RESOURCE_MISC_SHARED_KEYEDMUTEX;
								auto fd3dResource = d3d12Texture->GetResource();
								HRESULT res = myD3D11On12->CreateWrappedResource(fd3dResource->GetResource(), &flags, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_COPY_DEST,
																	__uuidof(ID3D11Resource), (void**)&output.wrappedResource);
							}
						}
						else if (output.wrappedResource)
						{
							myD3D11On12->AcquireWrappedResources(&output.wrappedResource, 1);
						}
						destResource = output.wrappedResource;
					}

					if (destResource)
					{
						myImmediateContext->CopyResource(destResource, d3dSrcTexture);

						if (myRHIType == RHIType::DirectX12)
						{
							myD3D11On12->ReleaseWrappedResources(&output.wrappedResource, 1);
						}

						// TODO: We get a crash if we release the teD3DTexture here,
						// so we differ it until D3D tells us it's done with it.
						// Ideally we should be able to release it here and let the ref
						// counting cause it to be cleaned up on it's own later on
#if 1
						TexCleanup cleanup;
						D3D11_QUERY_DESC queryDesc = {};
						queryDesc.Query = D3D11_QUERY_EVENT;
						myDevice->CreateQuery(&queryDesc, &cleanup.query);
						myImmediateContext->End(cleanup.query);
						cleanup.texture = teD3DTexture;

						myTexCleanups.push_back(cleanup);
#else
						TERelease(&teD3DTexture);
#endif
					}
					else
					{
						TERelease(&teD3DTexture);
					}
					TERelease(&dxgiTexture);

				});
				
			break;
		}
		case TEParameterTypeFloatStream:
		{

#if 0
			TEStreamDescription *desc = nullptr;
			result = TEInstanceParameterGetStreamDescription(myInstance, identifier, &desc);

			if (result == TEResultSuccess)
			{
				int32_t channelCount = desc->numChannels;
				std::vector <std::vector<float>> store(channelCount);
				std::vector<float *> channels;

				int64_t maxSamples = desc->maxSamples;
				for (auto &vector : store)
				{
					vector.resize(maxSamples);
					channels.emplace_back(vector.data());
				}

				int64_t start;
				int64_t length = maxSamples;
				result = TEInstanceParameterGetOutputStreamValues(myInstance, identifier, channels.data(), int32_t(channels.size()), &start, &length);
				if (result == TEResultSuccess)
				{
					FString name(identifier);
					// Use the channel data here
					if (length > 0 && channels.size() > 0)
					{
						if (!myCHOPOutputs.Contains(name))
						{
							myCHOPOutputs.Add(name);
						}

						auto &output = myCHOPOutputs[name];
						output.channelData.SetNum(desc->numChannels);
						for (int i = 0; i < desc->numChannels; i++)
						{
							output.channelData[i] = channels[i][length - 1];
						}

						//doc->myLastStreamValue = store.back()[length - 1];
					}
				}
				TERelease(&desc);
			}
#endif
		break;

		}
		default:
			break;
		}
	}
	TERelease(&param);
}



void
UTouchEngine::loadTox(FString toxPath)
{
	clear();
	myToxPath = toxPath;
	myDidLoad = false;

	FString rhiType = FApp::GetGraphicsRHI();
	if (rhiType == "DirectX 11")
	{
		myDevice = (ID3D11Device*)GDynamicRHI->RHIGetNativeDevice();
		myDevice->GetImmediateContext(&myImmediateContext);
		if (!myDevice || !myImmediateContext)
		{
			return;
		}
		myRHIType = RHIType::DirectX11;
	}
	else if (rhiType == "DirectX 12")
	{
		FD3D12DynamicRHI* dx12RHI = static_cast<FD3D12DynamicRHI*>(GDynamicRHI);
		auto dx12Device = (ID3D12Device*)GDynamicRHI->RHIGetNativeDevice();
		ID3D12CommandQueue* queue = dx12RHI->RHIGetD3DCommandQueue();
		D3D_FEATURE_LEVEL feature;
		if (FAILED(D3D11On12CreateDevice(dx12Device, 0, NULL, 0, (IUnknown**)&queue, 1, 0, &myDevice, &myImmediateContext, &feature)))
		{
			return;
		}

		myDevice->QueryInterface(__uuidof(ID3D11On12Device), (void**)&myD3D11On12);
#if 0
		ID3D12Debug* debugInterface = nullptr;
		if (SUCCEEDED(D3D12GetDebugInterface(__uuidof(ID3D12Debug), (void**)&debugInterface)))
		{
			if (debugInterface)
				debugInterface->EnableDebugLayer();
		}
#endif
		myRHIType = RHIType::DirectX12;
	}
	else
	{
		// Error
		return;
	}

	// TODO: need to make this work for all API options unreal works with
	TEResult result = TED3DContextCreate(myDevice, &myContext);

	if (result != TEResultSuccess)
	{
		// error
		return;
	}

	result = TEInstanceCreate(TCHAR_TO_UTF8(*toxPath),
									TETimeInternal,
									eventCallback,	
									parameterValueCallback,
									this,
									&myInstance);

	if (result != TEResultSuccess)
	{
		// error
		return;
	}

	result = TEInstanceAssociateGraphicsContext(myInstance, myContext);

	if (result == TEResultSuccess)
	{
		result = TEInstanceResume(myInstance);
	}

	if (result != TEResultSuccess)
	{
		// error
		return;
	}
}

void
UTouchEngine::cookFrame()
{
	if (myDidLoad)
	{
		TEInstanceStartFrameAtTime(myInstance, 0, 0, false);
	}
}

FTouchTOP
UTouchEngine::getTOPOutput(const FString& identifier)
{
	if (!myDidLoad)
	{
		return FTouchTOP();
	}
	FTouchTOP c;

	std::lock_guard<std::mutex> guard(myTOPLock);

	if (!myTOPOutputs.Contains(identifier))
	{
		myTOPOutputs.Add(identifier);
	}

	if (auto *top = myTOPOutputs.Find(identifier))
	{
		auto &t = *top;

		return *top;
	}
	else
	{
		return c;
	}
}

FTouchCHOPSingleSample
UTouchEngine::getCHOPOutputSingleSample(const FString& identifier)
{
	if (!myDidLoad)
	{
		return FTouchCHOPSingleSample();
	}

	FTouchCHOPSingleSample c;
	/*

	if (auto *chop = myCHOPOutputs.Find(identifier))
	{
		return *chop;
	}
	else
	{
		return c;
	}
	*/


	std::string fullId("output/");
	fullId += TCHAR_TO_UTF8(*identifier);

	TEParameterInfo *param = nullptr;
	TEResult result = TEInstanceParameterGetInfo(myInstance, fullId.c_str(), &param);
	if (result == TEResultSuccess && param->scope == TEScopeOutput)
	{
		switch (param->type)
		{
			case TEParameterTypeFloatStream:
			{

				TEStreamDescription *desc = nullptr;
				result = TEInstanceParameterGetStreamDescription(myInstance, fullId.c_str(), &desc);

				if (result == TEResultSuccess)
				{
					if (!myCHOPOutputs.Contains(identifier))
					{
						myCHOPOutputs.Add(identifier);
					}

					auto &output = myCHOPOutputs[identifier];

					int32_t channelCount = desc->numChannels;
					std::vector <std::vector<float>> store(channelCount);
					std::vector<float *> channels;

					int64_t maxSamples = desc->maxSamples;
					for (auto &vector : store)
					{
						vector.resize(maxSamples);
						channels.emplace_back(vector.data());
					}

					int64_t start;
					int64_t length = maxSamples;
					result = TEInstanceParameterGetOutputStreamValues(myInstance, fullId.c_str(), channels.data(), int32_t(channels.size()), &start, &length);
					if (result == TEResultSuccess)
					{
						// Use the channel data here
						if (length > 0 && channels.size() > 0)
						{
							output.channelData.SetNum(desc->numChannels);
							for (int i = 0; i < desc->numChannels; i++)
							{
								output.channelData[i] = channels[i][length - 1];
							}
						}
					}
					c = output;
					TERelease(&desc);
				}
				break;
			}
			default:
				break;
		}
	}
	TERelease(&param);

	return c;
}

void
UTouchEngine::setCHOPInputSingleSample(const FString &identifier, const FTouchCHOPSingleSample &chop)
{
	if (!myInstance)
		return;

	if (!myDidLoad)
	{
		return;
	}

	if (!chop.channelData.Num())
		return;

	std::string fullId("input/");
	fullId += TCHAR_TO_UTF8(*identifier);

	TEResult result;
	/*
	TEStringArray *groups;
	TEResult result = TEInstanceGetParameterGroups(myInstance, TEScopeInput, &groups);
	if (result == TEResultSuccess)
	{
		int textureCount = 0;
		for (int32_t i = 0; i < groups->count; i++)
		{
			TEStringArray *children;
			result = TEInstanceParameterGetChildren(myInstance, groups->strings[i], &children);
			if (result == TEResultSuccess)
			{
				int sdfkjhsdf = 3;
			}
		}
	}
	*/


	TEParameterInfo *info;
	result = TEInstanceParameterGetInfo(myInstance, fullId.c_str(), &info);

	if (result != TEResultSuccess)
	{
		return;
	}

	if (info->type != TEParameterTypeFloatStream)
	{
		TERelease(&info);
		return;
	}

	int64_t filled;
	std::vector<float>	realData;
	std::vector<const float*>	dataPtrs;
	for (int i = 0; i < chop.channelData.Num(); i++)
	{
		realData.push_back(chop.channelData[i]);
	}
	for (int i = 0; i < chop.channelData.Num(); i++)
	{
		dataPtrs.push_back(&realData[i]);
	}
	TEStreamDescription desc;
	desc.rate = 60;
	desc.numChannels = chop.channelData.Num();
	desc.maxSamples = 1;

	result = TEInstanceParameterSetInputStreamDescription(myInstance, fullId.c_str(), &desc);

	if (result != TEResultSuccess)
	{
		TERelease(&info);
		return;
	}

	result = TEInstanceParameterAppendStreamValues(myInstance, fullId.c_str(), dataPtrs.data(), chop.channelData.Num(), 0, &filled);

	if (result != TEResultSuccess)
	{
		TERelease(&info);
		return;
	}

	TERelease(&info);

}
