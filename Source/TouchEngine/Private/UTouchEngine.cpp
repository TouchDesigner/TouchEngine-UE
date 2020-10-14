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
#include "Runtime/RenderCore/Public/RenderingThread.h"

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
			context = myTEContext,
			instance = myTEInstance]
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
	myTEContext = nullptr;
	myTEInstance = nullptr;
	myDevice = nullptr;
	myD3D11On12 = nullptr;
}


const FString&
UTouchEngine::getToxPath() const
{
	return myToxPath;
}

void
UTouchEngine::eventCallback(TEInstance * instance,
							TEEvent event,
							TEResult result,
							int64_t start_time_value,
							int32_t start_time_scale,
							int64_t end_time_value,
							int32_t end_time_scale,
							void * info)
{
	auto *engine = static_cast<UTouchEngine*>(info);

	if (result != TEResultSuccess)
	{
		int i = 32;
	}
	switch (event)
	{
		case TEEventInstanceDidLoad:
			if (result == TEResultSuccess)
				engine->setDidLoad();
			else
				engine->addError("load() failure: ", result);
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

static EPixelFormat
toEPixelFormat(DXGI_FORMAT fmt)
{
	switch (fmt)
	{
		case DXGI_FORMAT_R8_UNORM:
			return PF_G8;
		case DXGI_FORMAT_R16_UNORM:
			return PF_G16;
		case DXGI_FORMAT_R32G32B32A32_FLOAT:
			return PF_A32B32G32R32F;
		case DXGI_FORMAT_B8G8R8A8_UNORM:
			return PF_B8G8R8A8;
		case DXGI_FORMAT_BC1_UNORM:
			return PF_DXT1;
		case DXGI_FORMAT_BC2_UNORM:
			return PF_DXT3;
		case DXGI_FORMAT_BC3_UNORM:
			return PF_DXT5;
		case DXGI_FORMAT_R16G16B16A16_FLOAT:
			return PF_FloatRGBA;
		case DXGI_FORMAT_R32_FLOAT:
			return PF_R32_FLOAT;
		case DXGI_FORMAT_R16G16_UNORM:
			return PF_G16R16;
		case DXGI_FORMAT_R16G16_FLOAT:
			return PF_G16R16F;
		case DXGI_FORMAT_R32G32_FLOAT:
			return PF_G32R32F;
		case DXGI_FORMAT_R10G10B10A2_UNORM:
			return PF_A2B10G10R10;
		case DXGI_FORMAT_R16G16B16A16_UNORM:
			return PF_R16G16B16A16_UNORM;
		case DXGI_FORMAT_R16_FLOAT:
			return PF_R16F;
		case DXGI_FORMAT_R11G11B10_FLOAT:
			return PF_FloatR11G11B10;
		case DXGI_FORMAT_R8G8B8A8_UNORM:
			return PF_R8G8B8A8;
		case DXGI_FORMAT_R8G8_UNORM:
			return PF_R8G8;
		default:
			return PF_Unknown;
	}
}


static DXGI_FORMAT
toTypedDXGIFormat(EPixelFormat fmt)
{
	switch (fmt)
	{
		case PF_G8:
			return DXGI_FORMAT_R8_UNORM;
		case PF_G16:
			return DXGI_FORMAT_R16_UNORM;
		case PF_A32B32G32R32F:
			return DXGI_FORMAT_R32G32B32A32_FLOAT;
		case PF_B8G8R8A8:
			return DXGI_FORMAT_B8G8R8A8_UNORM;
		case PF_DXT1:
			return DXGI_FORMAT_BC1_UNORM;
		case PF_DXT3:
			return DXGI_FORMAT_BC2_UNORM;
		case PF_DXT5:
			return DXGI_FORMAT_BC3_UNORM;
		case PF_FloatRGB:
		case PF_FloatRGBA:
			return DXGI_FORMAT_R16G16B16A16_FLOAT;
		case PF_R32_FLOAT:
			return DXGI_FORMAT_R32_FLOAT;
		case PF_G16R16:
			return DXGI_FORMAT_R16G16_UNORM;
		case PF_G16R16F:
			return DXGI_FORMAT_R16G16_FLOAT;
		case PF_G32R32F:
			return DXGI_FORMAT_R32G32_FLOAT;
		case PF_A2B10G10R10:
			return DXGI_FORMAT_R10G10B10A2_UNORM;
		case PF_A16B16G16R16:
			return DXGI_FORMAT_R16G16B16A16_UNORM;
		case PF_R16F:
			return DXGI_FORMAT_R16_FLOAT;
		case PF_FloatR11G11B10:
			return DXGI_FORMAT_R11G11B10_FLOAT;
		case PF_R8G8B8A8:
			return DXGI_FORMAT_R8G8B8A8_UNORM;
		case PF_R8G8:
			return DXGI_FORMAT_R8G8_UNORM;
		case PF_R16G16B16A16_UNORM:
			return DXGI_FORMAT_R16G16B16A16_UNORM;
		default:
			return DXGI_FORMAT_UNKNOWN;
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
			result = TEInstanceParameterGetDoubleValue(myTEInstance, identifier, TEParameterValueCurrent, &value, 1);
			break;
		}
		case TEParameterTypeInt:
		{
			int32_t value;
			result = TEInstanceParameterGetIntValue(myTEInstance, identifier, TEParameterValueCurrent, &value, 1);
			break;
		}
		case TEParameterTypeString:
		{
			TEString *value;
			result = TEInstanceParameterGetStringValue(myTEInstance, identifier, TEParameterValueCurrent, &value);
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
			TETexture *dxgiTexture = nullptr;
			result = TEInstanceParameterGetTextureValue(myTEInstance, identifier, TEParameterValueCurrent, &dxgiTexture);

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

					TED3DContextCreateTexture(myTEContext, dxgiTexture, &teD3DTexture);

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

					EPixelFormat pixelFormat = toEPixelFormat(desc.Format);

					if (pixelFormat == PF_Unknown)
					{
						addError(TEXT("Texture with unsupported pixel format being generated by TouchEngine."));
						return;
					}
					auto &output = myTOPOutputs[name];

					if (!output.texture ||
						output.texture->GetSizeX() != desc.Width ||
						output.texture->GetSizeY() != desc.Height ||
						output.texture->GetPixelFormat() != pixelFormat)
					{
						if (output.wrappedResource)
						{
							output.wrappedResource->Release();
							output.wrappedResource = nullptr;
						}
						output.texture = nullptr;
						output.texture = UTexture2D::CreateTransient(desc.Width, desc.Height, pixelFormat);
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
			result = TEInstanceParameterGetStreamDescription(myTEInstance, identifier, &desc);

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
				result = TEInstanceParameterGetOutputStreamValues(myTEInstance, identifier, channels.data(), int32_t(channels.size()), &start, &length);
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
			outputError("loadTox(): Unable to obtain DX11 Device / Context.");
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
			outputError("loadTox(): Unable to Create D3D11On12 Device.");
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
		outputError("loadTox(): Unsupported RHI active.");
		return;
	}

	// TODO: need to make this work for all API options unreal works with
	TEResult result = TED3DContextCreate(myDevice, &myTEContext);

	if (result != TEResultSuccess)
	{
		outputError("loadTox(): Unable to create TouchEngine context: ", result);
		return;
	}

	result = TEInstanceCreate(TCHAR_TO_UTF8(*toxPath),
									TETimeInternal,
									eventCallback,	
									parameterValueCallback,
									this,
									&myTEInstance);

	if (result != TEResultSuccess)
	{
		outputError("loadTox(): Unable to create TouchEngine instance: ", result);
		return;
	}

	result = TEInstanceAssociateGraphicsContext(myTEInstance, myTEContext);

	if (result == TEResultSuccess)
	{
		result = TEInstanceResume(myTEInstance);
	}
	else
	{
		outputError("loadTox(): Unable to associate graphics context: ", result);
		return;
	}
}

void
UTouchEngine::cookFrame()
{
	outputMessages();
	if (myDidLoad)
	{
		FlushRenderingCommands();
		TEInstanceStartFrameAtTime(myTEInstance, 0, 0, false);
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

	FScopeLock lock(&myTOPLock);

	if (!myTOPOutputs.Contains(identifier))
	{
		myTOPOutputs.Add(identifier);
	}

	auto doWarning =
		[this, &identifier]()
		{
			addWarning(FString("getTOPOutput(): Unable to find output named: ") + identifier);
		};

	if (auto *top = myTOPOutputs.Find(identifier))
	{
		if (!top->texture)
			doWarning();

		return *top;
	}
	else
	{
		doWarning();
		return c;
	}
}

void
UTouchEngine::addWarning(const FString& s, TEResult result)
{
	FString s2(s);
	s2 += TEResultGetDescription(result);

	addWarning(s2);
}

void
UTouchEngine::addWarning(const FString& s)
{
#ifdef WITH_EDITOR
	FScopeLock lock(&myMessageLock);
	myWarnings.Add(s);
#endif
}

void
UTouchEngine::addError(const FString& s, TEResult result)
{
	FString s2(s);
	s2 += TEResultGetDescription(result);

	addError(s2);
}

void
UTouchEngine::addError(const FString& s)
{
#ifdef WITH_EDITOR
	FScopeLock lock(&myMessageLock);
	myErrors.Add(s);
#endif
}

void
UTouchEngine::outputMessages()
{
#ifdef WITH_EDITOR
	FScopeLock lock(&myMessageLock);
	FMessageLog log("TouchEngine");
	for (auto& m : myErrors)
	{
		outputError(log, m);
	}
	for (auto& m : myWarnings)
	{
		log.Warning(FText::FromString(m));
	}
#endif
	myErrors.Empty();
	myWarnings.Empty();
}

void
UTouchEngine::outputError(const FString& s, TEResult result)
{
#ifdef WITH_EDITOR
	FString s2(s);
	s2 += TEResultGetDescription(result);

	outputError(s2);
#endif
}

void
UTouchEngine::outputError(FMessageLog& log, const FString &s)
{
#ifdef WITH_EDITOR
	log.Error(FText::FromString(s));
#endif
}

void
UTouchEngine::outputError(const FString &s)
{
#ifdef WITH_EDITOR
	FMessageLog log("TouchEngine");
	outputError(log, s);
#endif
}

void
UTouchEngine::outputWarning(FMessageLog& log, const FString &s)
{
#ifdef WITH_EDITOR
	log.Warning(FText::FromString(s));
#endif
}

void
UTouchEngine::outputWarning(const FString& s)
{
#ifdef WITH_EDITOR
	FMessageLog log("TouchEngine");
	outputWarning(log, s);
#endif
}


static bool
isTypeless(DXGI_FORMAT format)
{
	switch (format)
	{
		case DXGI_FORMAT_R32G32B32A32_TYPELESS:
		case DXGI_FORMAT_R32G32B32_TYPELESS:
		case DXGI_FORMAT_R16G16B16A16_TYPELESS:
		case DXGI_FORMAT_R32G32_TYPELESS:
		case DXGI_FORMAT_R32G8X24_TYPELESS:
		case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
		case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
		case DXGI_FORMAT_R10G10B10A2_TYPELESS:
		case DXGI_FORMAT_R8G8B8A8_TYPELESS:
		case DXGI_FORMAT_R16G16_TYPELESS:
		case DXGI_FORMAT_R32_TYPELESS:
		case DXGI_FORMAT_R24G8_TYPELESS:
		case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
		case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
		case DXGI_FORMAT_R8G8_TYPELESS:
		case DXGI_FORMAT_R16_TYPELESS:
		case DXGI_FORMAT_R8_TYPELESS:
		case DXGI_FORMAT_BC1_TYPELESS:
		case DXGI_FORMAT_BC2_TYPELESS:
		case DXGI_FORMAT_BC3_TYPELESS:
		case DXGI_FORMAT_BC4_TYPELESS:
		case DXGI_FORMAT_BC5_TYPELESS:
		case DXGI_FORMAT_B8G8R8A8_TYPELESS:
		case DXGI_FORMAT_B8G8R8X8_TYPELESS:
		case DXGI_FORMAT_BC6H_TYPELESS:
		case DXGI_FORMAT_BC7_TYPELESS:
			return true;
		default:
			return false;
	}
}

static DXGI_FORMAT
toTypedDXGIFormat(ETextureRenderTargetFormat format)
{
	switch (format)
	{
		case RTF_R8:
			return DXGI_FORMAT_R8_UNORM;
		case RTF_RG8:
			return DXGI_FORMAT_R8G8_UNORM;
		case RTF_RGBA8:
			return DXGI_FORMAT_B8G8R8A8_UNORM;
		case RTF_RGBA8_SRGB:
			return DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
		case RTF_R16f:
			return DXGI_FORMAT_R16_FLOAT;
		case RTF_RG16f:
			return DXGI_FORMAT_R16G16_FLOAT;
		case RTF_RGBA16f:
			return DXGI_FORMAT_R16G16B16A16_FLOAT;
		case RTF_R32f:
			return DXGI_FORMAT_R32_FLOAT;
		case RTF_RG32f:
			return DXGI_FORMAT_R32G32_FLOAT;
		case RTF_RGBA32f:
			return DXGI_FORMAT_R32G32B32A32_FLOAT;
		case RTF_RGB10A2:
			return DXGI_FORMAT_R10G10B10A2_UNORM;
		default:
			return DXGI_FORMAT_UNKNOWN;
	}
}

void
UTouchEngine::setTOPInput(const FString& identifier, UTexture* texture)
{
	if (!myDidLoad)
	{
		return;
	}

	if (!texture || !texture->Resource)
	{
		outputError(TEXT("setTOPInput(): Null or empty texture provided"));
		return;
	}


	std::string fullId("input/");
	fullId += TCHAR_TO_UTF8(*identifier);

	TEParameterInfo *param = nullptr;
	TEResult result = TEInstanceParameterGetInfo(myTEInstance, fullId.c_str(), &param);

	if (result != TEResultSuccess)
	{
		outputError(TEXT("setTOPInput(): Unable to get input info for: ") + identifier + ". ", result);
		return;
	}

	ENQUEUE_RENDER_COMMAND(void)(
		[this, fullId, texture](FRHICommandListImmediate& RHICmdList) 
		{
			UTexture2D* tex2d = dynamic_cast<UTexture2D*>(texture);
			UTextureRenderTarget2D* rt = nullptr;
			if (!tex2d)
			{
				rt = dynamic_cast<UTextureRenderTarget2D*>(texture);
			}

			if (!rt && !tex2d)
				return;

			TETexture* teTexture = nullptr;
			ID3D11Texture2D* wrappedResource = nullptr;
			if (myRHIType == RHIType::DirectX11)
			{
				FD3D11Texture2D* d3d11Texture = nullptr;
				DXGI_FORMAT typedDXGIFormat;
				if (tex2d)
				{
					d3d11Texture = (FD3D11Texture2D*)GetD3D11TextureFromRHITexture(tex2d->Resource->TextureRHI);
					typedDXGIFormat = toTypedDXGIFormat(tex2d->GetPixelFormat());
				}
				else if (rt)
				{
					d3d11Texture = (FD3D11Texture2D*)GetD3D11TextureFromRHITexture(rt->Resource->TextureRHI);
					typedDXGIFormat = toTypedDXGIFormat(rt->RenderTargetFormat);
				}

				if (d3d11Texture)
				{
					D3D11_TEXTURE2D_DESC desc;
					d3d11Texture->GetResource()->GetDesc(&desc);
					if (isTypeless(desc.Format))
					{
						teTexture = TED3DTextureCreateTypeless(d3d11Texture->GetResource(), false, typedDXGIFormat);
					}
					else
					{
						teTexture = TED3DTextureCreate(d3d11Texture->GetResource(), false);
					}
				}
			}
			else if (myRHIType == RHIType::DirectX12)
			{
				FD3D12Texture2D* d3d12Texture = nullptr;
				if (tex2d)
				{
					d3d12Texture = (FD3D12Texture2D*)GetD3D12TextureFromRHITexture(tex2d->Resource->TextureRHI);
				}
				else if (rt)
				{
					d3d12Texture = (FD3D12Texture2D*)GetD3D12TextureFromRHITexture(tex2d->Resource->TextureRHI);
				}
				
				if (d3d12Texture)
				{
					if (myD3D11On12)
					{
						D3D11_RESOURCE_FLAGS flags = {};
						flags.MiscFlags = 0; // D3D11_RESOURCE_MISC_SHARED_KEYEDMUTEX;
						auto fd3dResource = d3d12Texture->GetResource();

						//D3D12_RESOURCE_STATES inState = d3d12Texture->GetResource()->GetResourceState().GetSubresourceState(0);
						D3D12_RESOURCE_STATES inState = D3D12_RESOURCE_STATE_COPY_SOURCE;
						HRESULT res = myD3D11On12->CreateWrappedResource(fd3dResource->GetResource(), &flags, inState, D3D12_RESOURCE_STATE_COPY_SOURCE,
															//__uuidof(ID3D11Resource), (void**)&resource);
															__uuidof(ID3D11Texture2D), (void**)&wrappedResource);
						teTexture = TED3DTextureCreate(wrappedResource, false);
					}
				}
			}

			if (teTexture)
			{
				TEResult res = TEInstanceParameterSetTextureValue(myTEInstance, fullId.c_str(), teTexture, myTEContext);
				TERelease(&teTexture);
			}

			if (wrappedResource)
			{
				myD3D11On12->ReleaseWrappedResources((ID3D11Resource**)&wrappedResource, 1);
				wrappedResource->Release();
			}
		});
}

FTouchCHOPSingleSample
UTouchEngine::getCHOPOutputSingleSample(const FString& identifier)
{
	if (!myDidLoad)
	{
		return FTouchCHOPSingleSample();
	}

	FTouchCHOPSingleSample c;

	std::string fullId("output/");
	fullId += TCHAR_TO_UTF8(*identifier);

	TEParameterInfo *param = nullptr;
	TEResult result = TEInstanceParameterGetInfo(myTEInstance, fullId.c_str(), &param);
	if (result == TEResultSuccess && param->scope == TEScopeOutput)
	{
		switch (param->type)
		{
			case TEParameterTypeFloatStream:
			{

				TEStreamDescription *desc = nullptr;
				result = TEInstanceParameterGetStreamDescription(myTEInstance, fullId.c_str(), &desc);

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
					result = TEInstanceParameterGetOutputStreamValues(myTEInstance, fullId.c_str(), channels.data(), int32_t(channels.size()), &start, &length);
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
					else
					{
						outputError(TEXT("getCHOPOutputSingleSample(): "), result);
					}
					c = output;
					TERelease(&desc);
				}
				break;
			}
			default:
			{
				outputError(TEXT("getCHOPOutputSingleSample(): ") + identifier + TEXT(" is not a CHOP output."));
				break;
			}
		}
	}
	else if (result != TEResultSuccess)
	{
		outputError(TEXT("getCHOPOutputSingleSample(): "), result);
	}
	else if (param->scope == TEScopeOutput)
	{
		outputError(TEXT("getCHOPOutputSingleSample(): ") + identifier + TEXT(" is not a CHOP output."));
	}
	TERelease(&param);

	return c;
}

void
UTouchEngine::setCHOPInputSingleSample(const FString &identifier, const FTouchCHOPSingleSample &chop)
{
	if (!myTEInstance)
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
	TEResult result = TEInstanceGetParameterGroups(myTEInstance, TEScopeInput, &groups);
	if (result == TEResultSuccess)
	{
		int textureCount = 0;
		for (int32_t i = 0; i < groups->count; i++)
		{
			TEStringArray *children;
			result = TEInstanceParameterGetChildren(myTEInstance, groups->strings[i], &children);
			if (result == TEResultSuccess)
			{
				int sdfkjhsdf = 3;
			}
		}
	}
	*/


	TEParameterInfo *info;
	result = TEInstanceParameterGetInfo(myTEInstance, fullId.c_str(), &info);

	if (result != TEResultSuccess)
	{
		outputError(FString("setCHOPInputSingleSample(): Unable to get input info, ") + FString(identifier) + " may not exist. ", result);
		return;
	}

	if (info->type != TEParameterTypeFloatStream)
	{
		outputError(FString("setCHOPInputSingleSample(): Input named: ") + FString(identifier) + " is not a CHOP input.");
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

	result = TEInstanceParameterSetInputStreamDescription(myTEInstance, fullId.c_str(), &desc);

	if (result != TEResultSuccess)
	{
		outputError(FString("setCHOPInputSingleSample(): Unable to get stream desciription: "), result);
		TERelease(&info);
		return;
	}

	result = TEInstanceParameterAppendStreamValues(myTEInstance, fullId.c_str(), dataPtrs.data(), chop.channelData.Num(), 0, &filled);

	if (result != TEResultSuccess)
	{
		outputError(FString("setCHOPInputSingleSample(): Unable to append stream values: "), result);
		TERelease(&info);
		return;
	}

	TERelease(&info);
}
