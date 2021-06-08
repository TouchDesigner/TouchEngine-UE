/* Shared Use License: This file is owned by Derivative Inc. (Derivative)
* and can only be used, and/or modified for use, in conjunction with
* Derivative's TouchDesigner software, and only if you are a licensee who has
* accepted Derivative's TouchDesigner license or assignment agreement
* (which also govern the use of this file). You may share or redistribute
* a modified version of this file provided the following conditions are met:
*
* 1. The shared file or redistribution must retain the information set out
* above and this list of conditions.
* 2. Derivative's name (Derivative Inc.) or its trademarks may not be used
* to endorse or promote products derived from this file without specific
* prior written permission from Derivative.
*/

#include "UTouchEngine.h"
#include <vector>
#include <mutex>
#include "DynamicRHI.h"
#include "D3D11RHI.h"
#include "D3D11StateCachePrivate.h"
#include "D3D11Util.h"
#include "D3D11State.h"
#include "D3D11Resources.h"
#include "TouchEngineDynamicVariableStruct.h"
#include "Runtime/RenderCore/Public/RenderingThread.h"
#include "Engine/TextureRenderTarget2D.h"

void
UTouchEngine::BeginDestroy()
{
	clear();

	Super::BeginDestroy();
}

void
UTouchEngine::clear()
{
	myCHOPSingleOutputs.Empty();

	FScopeLock lock(&myTOPLock);

	/*
	auto _immediateContext = myImmediateContext;
	auto _cleanups = myTexCleanups;
	auto _context = myTEContext;
	auto _instance = myTEInstance;

	ENQUEUE_RENDER_COMMAND(void)(
		[immediateContext = _immediateContext,
		//d3d11On12 = myD3D11On12,
		cleanups = _cleanups,
		context = _context,
		instance = _instance]
	(FRHICommandListImmediate& RHICmdList) mutable
		{
			cleanupTextures(immediateContext, &cleanups, FinalClean::True);

			if (immediateContext)
				immediateContext->Release();

			//if (d3d11On12)
			//	d3d11On12->Release();

			TERelease(&context);
			TERelease(&instance);
		});
	*/

	ENQUEUE_RENDER_COMMAND(void)(
		[this](FRHICommandListImmediate& RHICmdList)
		{
			cleanupTextures(myImmediateContext, &myTexCleanups, FinalClean::True);
			if (myImmediateContext)
				myImmediateContext->Release();
			TERelease(&myTEContext);
			//TERelease(&myTEInstance);

			myTexCleanups.clear();
			myImmediateContext = nullptr;
			myTEContext = nullptr;
			myTEInstance.reset();
			myDevice = nullptr;
			//myD3D11On12 = nullptr;
			myFailedLoad = false;
			myToxPath = "";
		});
}


const FString&
UTouchEngine::getToxPath() const
{
	return myToxPath;
}

void
UTouchEngine::eventCallback(TEInstance* instance, TEEvent event, TEResult result, int64_t start_time_value, int32_t start_time_scale, int64_t end_time_value, int32_t end_time_scale, void* info)
{
	UTouchEngine* engine = static_cast<UTouchEngine*>(info);
	if (!engine)
		return;

	switch (event)
	{
	case TEEventInstanceDidLoad:
		if (result == TEResultSuccess)
		{
			engine->setDidLoad();

			// Broadcast Links loaded event
			TArray<FTouchEngineDynamicVariableStruct> variablesIn, variablesOut;

			for (TEScope scope : { TEScopeInput, TEScopeOutput })
			{
				TEStringArray* groups;
				result = TEInstanceGetLinkGroups(instance, scope, &groups);

				if (result == TEResultSuccess)
				{
					for (int32_t i = 0; i < groups->count; i++)
					{
						switch (scope)
						{
						case TEScopeInput:
							engine->parseGroup(instance, groups->strings[i], variablesIn);
							break;
						case TEScopeOutput:
							engine->parseGroup(instance, groups->strings[i], variablesOut);
							break;
						}
					}
				}

			}

			UTouchEngine* savedEngine = engine;
			AsyncTask(ENamedThreads::GameThread, [savedEngine, variablesIn, variablesOut]()
				{
					savedEngine->OnParametersLoaded.Broadcast(variablesIn, variablesOut);
				}
			);

			// Broadcast engine loaded event
			/*
			AsyncTask(ENamedThreads::GameThread, [savedEngine]()
				{
					savedEngine->OnLoadComplete.Broadcast();
				}
			);
			*/
		}
		else if (result == TEResultFileError)
		{
			UTouchEngine* savedEngine = engine;
			AsyncTask(ENamedThreads::GameThread, [savedEngine]()
				{
					savedEngine->addError("load() failed to load .tox: " + savedEngine->myToxPath);
					savedEngine->myFailedLoad = true;
					savedEngine->OnLoadFailed.Broadcast("file error");
				}
			);
		}
		else if (result == TEResultIncompatibleEngineVersion)
		{
			UTouchEngine* savedEngine = engine;
			AsyncTask(ENamedThreads::GameThread, [savedEngine]()
				{
					savedEngine->addError("plugin version is incompatible with TouchDesigner version");
					savedEngine->myFailedLoad = true;
					savedEngine->OnLoadFailed.Broadcast("plugin version is incompatible with TouchDesigner version");
				}
			);
		}
		else
		{

			TESeverity severity = TEResultGetSeverity(result);

			if (severity == TESeverity::TESeverityError)
			{
				UTouchEngine* savedEngine = engine;
				TEResult savedResult = result;
				AsyncTask(ENamedThreads::GameThread, [savedEngine, savedResult]()
					{
						savedEngine->addResult("load(): tox file severe warning", savedResult);
						savedEngine->myFailedLoad = true;
						savedEngine->OnLoadFailed.Broadcast("severe warning");
					}
				);
			}
			else
			{
				engine->setDidLoad();

				// Broadcast Links loaded event
				TArray<FTouchEngineDynamicVariableStruct> variablesIn, variablesOut;

				for (TEScope scope : { TEScopeInput, TEScopeOutput })
				{
					TEStringArray* groups;
					result = TEInstanceGetLinkGroups(instance, scope, &groups);

					if (result == TEResultSuccess)
					{
						for (int32_t i = 0; i < groups->count; i++)
						{
							switch (scope)
							{
							case TEScopeInput:
								engine->parseGroup(instance, groups->strings[i], variablesIn);
								break;
							case TEScopeOutput:
								engine->parseGroup(instance, groups->strings[i], variablesOut);
								break;
							}
						}
					}

				}

				UTouchEngine* savedEngine = engine;
				AsyncTask(ENamedThreads::GameThread, [savedEngine, variablesIn, variablesOut]()
					{
						savedEngine->OnParametersLoaded.Broadcast(variablesIn, variablesOut);
					}
				);
			}
		}
		break;
	case TEEventFrameDidFinish:
	{
		engine->myCooking = false;
		engine->OnCookFinished.Broadcast();
	}
	break;
	case TEEventGeneral:
		// TODO: check result here
		break;
	default:
		break;
	}
}

void
UTouchEngine::linkValueCallback(TEInstance* instance, TELinkEvent event, const char* identifier, void* info)
{
	UTouchEngine* doc = static_cast<UTouchEngine*>(info);
	doc->linkValueCallback(instance, event, identifier);
}

void
UTouchEngine::cleanupTextures(ID3D11DeviceContext* context, std::deque<TexCleanup>* cleanups, FinalClean fa)
{
	if (cleanups == nullptr)
		return;


	while (!cleanups->empty())
	{
		auto& cleanup = cleanups->front();

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
	case PF_DXT1:
		//return DXGI_FORMAT_BC1_UNORM;
	case PF_DXT3:
		//return DXGI_FORMAT_BC2_UNORM;
	case PF_DXT5:
		//return DXGI_FORMAT_BC3_UNORM;
	default:
		return DXGI_FORMAT_UNKNOWN;
	case PF_G8:
		return DXGI_FORMAT_R8_UNORM;
	case PF_G16:
		return DXGI_FORMAT_R16_UNORM;
	case PF_A32B32G32R32F:
		return DXGI_FORMAT_R32G32B32A32_FLOAT;
	case PF_B8G8R8A8:
		return DXGI_FORMAT_B8G8R8A8_UNORM;
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

	}
}


void
UTouchEngine::linkValueCallback(TEInstance* instance, TELinkEvent event, const char* identifier)
{
	if (!instance)
		return;

	TELinkInfo* param = nullptr;
	TEResult result = TEInstanceLinkGetInfo(instance, identifier, &param);

	if (result == TEResultSuccess && param && param->scope == TEScopeOutput)
	{
		switch (event)
		{

		case TELinkEventAdded:
		{
			// single Link added
		}
		break;
		case TELinkEventRemoved:
		{
			return;
		}
		break;
		case TELinkEventValueChange:
		{
			// current value of the callback 
			if (!myTEInstance)
				return;
			switch (param->type)
			{
			case TELinkTypeTexture:
			{
				// Stash the state, we don't do any actual renderer work from this thread
				TEDXGITexture* dxgiTexture = nullptr;
				result = TEInstanceLinkGetTextureValue(myTEInstance, identifier, TELinkValueCurrent, &dxgiTexture);

				if (result != TEResultSuccess)
				{
					// possible crash without this check
					return;
				}

				FString name(identifier);
				ENQUEUE_RENDER_COMMAND(void)(
					[this, name, dxgiTexture](FRHICommandListImmediate& RHICmdList)
					{
						cleanupTextures(myImmediateContext, &myTexCleanups, FinalClean::False);

						TED3D11Texture* teD3DTexture = nullptr;

						FScopeLock lock(&myTOPLock);

						TED3D11ContextCreateTexture(myTEContext, dxgiTexture, &teD3DTexture);

						if (!teD3DTexture)
						{
							TERelease(&dxgiTexture);
							return;
						}

						ID3D11Texture2D* d3dSrcTexture = TED3D11TextureGetTexture(teD3DTexture);

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
						auto& output = myTOPOutputs[name];

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
							output.texture->MipGenSettings = TMGS_NoMipmaps;
							output.texture->SRGB = false;
							output.texture->CompressionSettings = TextureCompressionSettings::TC_VectorDisplacementmap;
							output.texture->Filter = TextureFilter::TF_Nearest;
							output.texture->UpdateResource();
						}
						UTexture2D* destTexture = output.texture;

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
#if 0
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
#endif
						}

						if (destResource)
						{
							myImmediateContext->CopyResource(destResource, d3dSrcTexture);

							if (myRHIType == RHIType::DirectX12)
							{
								//myD3D11On12->ReleaseWrappedResources(&output.wrappedResource, 1);
							}

							// TODO: We get a crash if we release the teD3DTexture here,
							// so we differ it until D3D tells us it's done with it.
							// Ideally we should be able to release it here and let the ref
							// counting cause it to be cleaned up on it's own later on
#if 1
							TexCleanup cleanup;
							D3D11_QUERY_DESC queryDesc = {};
							queryDesc.Query = D3D11_QUERY_EVENT;
							HRESULT hresult = myDevice->CreateQuery(&queryDesc, &cleanup.query);
							if (hresult == 0)
							{
								myImmediateContext->End(cleanup.query);
								if (teD3DTexture)
								{
									cleanup.texture = teD3DTexture;

									myTexCleanups.push_back(cleanup);
								}
								}
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
			case TELinkTypeFloatBuffer:
			{

#if 0
				TEStreamDescription* desc = nullptr;
				result = TEInstanceLinkGetStreamDescription(myTEInstance, identifier, &desc);

				if (result == TEResultSuccess)
				{
					int32_t channelCount = desc->numChannels;
					std::vector <std::vector<float>> store(channelCount);
					std::vector<float*> channels;

					int64_t maxSamples = desc->maxSamples;
					for (auto& vector : store)
					{
						vector.resize(maxSamples);
						channels.emplace_back(vector.data());
					}

					int64_t start;
					int64_t length = maxSamples;
					result = TEInstanceLinkGetOutputStreamValues(myTEInstance, identifier, channels.data(), int32_t(channels.size()), &start, &length);
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

							auto& output = myCHOPOutputs[name];
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
		break;
}
	}

	TERelease(&param);
}


TEResult
UTouchEngine::parseGroup(TEInstance* instance, const char* identifier, TArray<FTouchEngineDynamicVariableStruct>& variables)
{
	// load each group
	TELinkInfo* group;
	TEResult result = TEInstanceLinkGetInfo(instance, identifier, &group);

	if (result != TEResultSuccess)
	{
		// failure
		return result;
	}

	// use group data here

	TERelease(&group);

	// load children of each group
	TEStringArray* children = nullptr;
	result = TEInstanceLinkGetChildren(instance, identifier, &children);

	if (result != TEResultSuccess)
	{
		//failure 
		return result;
	}

	// use children data here 
	for (int i = 0; i < children->count; i++)
	{
		result = parseInfo(instance, children->strings[i], variables);
	}

	TERelease(&children);
	return result;
}

TEResult
UTouchEngine::parseInfo(TEInstance* instance, const char* identifier, TArray<FTouchEngineDynamicVariableStruct>& variableList)
{
	TELinkInfo* info;
	TEResult result = TEInstanceLinkGetInfo(instance, identifier, &info);

	if (result != TEResultSuccess)
	{
		//failure
		return result;
	}

	// parse our children into a dynamic variable struct
	FTouchEngineDynamicVariableStruct variable;

	variable.VarLabel = FString(info->label);

	FString domainChar = "";

	switch (info->domain)
	{
	case TELinkDomainNone:
	{
	}
	break;
	case TELinkDomainParameter:
	{
		domainChar = "p";
	}
	break;
	case TELinkDomainParameterPage:
	{
	}
	break;
	case TELinkDomainOperator:
	{
		switch (info->scope)
		{
		case TEScopeInput:
			domainChar = "i";
			break;
		case TEScopeOutput:
			domainChar = "o";
			break;
		}
		break;
	}
	}

	variable.VarName = domainChar.Append("/").Append(info->name);
	variable.VarIdentifier = FString(info->identifier);
	variable.count = info->count;
	if (variable.count > 1)
		variable.isArray = true;

	// figure out what type 
	switch (info->type)
	{
	case TELinkTypeGroup:
	{
		//TArray<FTouchEngineDynamicVariable> variables;
		result = parseGroup(instance, identifier, variableList);
	}
	break;
	case TELinkTypeComplex:
	{
		variable.VarType = EVarType::VARTYPE_NOT_SET;
	}
	break;
	case TELinkTypeBoolean:
	{
		variable.VarType = EVarType::VARTYPE_BOOL;
		if (info->domain == TELinkDomainParameter || (info->domain == TELinkDomainOperator && info->scope == TEScopeInput))
		{
			bool defaultVal;
			result = TEInstanceLinkGetBooleanValue(instance, identifier, TELinkValueDefault, &defaultVal);

			if (result == TEResult::TEResultSuccess)
			{
				variable.SetValue(defaultVal);
			}
		}
	}
	break;
	case TELinkTypeDouble:
	{
		variable.VarType = EVarType::VARTYPE_DOUBLE;

		if (info->domain == TELinkDomainParameter || (info->domain == TELinkDomainOperator && info->scope == TEScopeInput))
		{
			if (info->count == 1)
			{
				double defaultVal;
				result = TEInstanceLinkGetDoubleValue(instance, identifier, TELinkValueDefault, &defaultVal, 1);

				if (result == TEResult::TEResultSuccess)
				{
					variable.SetValue(defaultVal);
				}
			}
			else
			{
				double* defaultVal = (double*)_alloca(sizeof(double) * info->count);
				result = TEInstanceLinkGetDoubleValue(instance, identifier, TELinkValueDefault, defaultVal, info->count);

				if (result == TEResult::TEResultSuccess)
				{
					TArray<double> buffer;

					for (int i = 0; i < info->count; i++)
					{
						buffer.Add(defaultVal[i]);
					}

					variable.SetValue(buffer);
				}
			}
		}
	}
	break;
	case TELinkTypeInt:
	{
		variable.VarType = EVarType::VARTYPE_INT;

		if (info->domain == TELinkDomainParameter || (info->domain == TELinkDomainOperator && info->scope == TEScopeInput))
		{
			if (info->count == 1)
			{
				TEStringArray* choiceLabels = nullptr;
				result = TEInstanceLinkGetChoiceLabels(instance, info->identifier, &choiceLabels);

				if (choiceLabels)
				{
					variable.VarIntent = EVarIntent::VARINTENT_DROPDOWN;

#if WITH_EDITORONLY_DATA
					for (int i = 0; i < choiceLabels->count; i++)
					{
						variable.dropDownData.Add(choiceLabels->strings[i], i);
					}
#endif

					TERelease(&choiceLabels);
				}

				FTouchVar<int32_t> c;
				result = TEInstanceLinkGetIntValue(instance, identifier, TELinkValueDefault, &c.data, 1);

				if (result == TEResult::TEResultSuccess)
				{
					variable.SetValue((int)c.data);
				}
			}
			else
			{
				FTouchVar<int32_t*> c;
				c.data = (int32_t*)_alloca(sizeof(int32_t) * 4);

				result = TEInstanceLinkGetIntValue(instance, identifier, TELinkValueDefault, c.data, info->count);

				if (result == TEResult::TEResultSuccess)
				{
					TArray<int> values;

					for (int i = 0; i < info->count; i++)
					{
						values.Add((int)c.data[i]);
					}

					variable.SetValue(values);
				}
			}
		}
	}
	break;
	case TELinkTypeString:
	{
		variable.VarType = EVarType::VARTYPE_STRING;

		if (info->domain == TELinkDomainParameter || (info->domain == TELinkDomainOperator && info->scope == TEScopeInput))
		{
			if (info->count == 1)
			{
				TEStringArray* choiceLabels = nullptr;
				result = TEInstanceLinkGetChoiceLabels(instance, info->identifier, &choiceLabels);

				if (choiceLabels)
				{
					variable.VarIntent = EVarIntent::VARINTENT_DROPDOWN;

#if WITH_EDITORONLY_DATA
					for (int i = 0; i < choiceLabels->count; i++)
					{
						variable.dropDownData.Add(choiceLabels->strings[i], i);
					}
#endif

					TERelease(&choiceLabels);
				}


				TEString* defaultVal = nullptr;
				result = TEInstanceLinkGetStringValue(instance, identifier, TELinkValueDefault, &defaultVal);

				if (result == TEResult::TEResultSuccess)
				{
					variable.SetValue(FString(defaultVal->string));
				}
				TERelease(&defaultVal);
			}
			else
			{
				TEString* defaultVal = nullptr;
				result = TEInstanceLinkGetStringValue(instance, identifier, TELinkValueDefault, &defaultVal);

				if (result == TEResult::TEResultSuccess)
				{
					TArray<FString> values;
					for (int i = 0; i < info->count; i++)
					{
						values.Add(FString(defaultVal[i].string));
					}

					variable.SetValue(values);
				}
				TERelease(&defaultVal);
			}
		}
	}
	break;
	case TELinkTypeTexture:
	{
		variable.VarType = EVarType::VARTYPE_TEXTURE;

		// textures have no valid default values 
	}
	break;
	case TELinkTypeFloatBuffer:
	{
		variable.VarType = EVarType::VARTYPE_FLOATBUFFER;
		variable.isArray = true;

		if (info->domain == TELinkDomainParameter || (info->domain == TELinkDomainOperator && info->scope == TEScopeInput))
		{
			TEFloatBuffer* buf = nullptr;
			result = TEInstanceLinkGetFloatBufferValue(instance, identifier, TELinkValueDefault, &buf);

			if (result == TEResult::TEResultSuccess)
			{
				TArray<float> values;
				int maxChannels = TEFloatBufferGetChannelCount(buf);
				const float* const* channels = TEFloatBufferGetValues(buf);

				for (int i = 0; i < maxChannels; i++)
				{
					values.Add(*channels[i]);
				}

				variable.SetValue(values);
			}
			TERelease(&buf);
		}
	}
	break;
	case TELinkTypeStringData:
	{
		variable.VarType = EVarType::VARTYPE_STRING;
		variable.isArray = true;

		if (info->domain == TELinkDomainParameter || (info->domain == TELinkDomainOperator && info->scope == TEScopeInput))
		{
		}
	}
	break;
	case TELinkTypeSeparator:
	{
		variable.VarType = EVarType::VARTYPE_NOT_SET;
		return result;
	}
	break;
	}

	switch (info->intent)
	{
		//case TELinkIntentNotSpecified:
		//	variable.VarIntent = EVarIntent::VARINTENT_NOT_SET;
		//	break;
	case TELinkIntentColorRGBA:
		variable.VarIntent = EVarIntent::VARINTENT_COLOR;
		break;
	case TELinkIntentPositionXYZW:
		variable.VarIntent = EVarIntent::VARINTENT_POSITION;
		break;
	case TELinkIntentSizeWH:
		variable.VarIntent = EVarIntent::VARINTENT_SIZE;
		break;
	case TELinkIntentUVW:
		variable.VarIntent = EVarIntent::VARINTENT_UVW;
		break;
	case TELinkIntentFilePath:
		variable.VarIntent = EVarIntent::VARINTENT_FILEPATH;
		break;
	case TELinkIntentDirectoryPath:
		variable.VarIntent = EVarIntent::VARINTENT_DIRECTORYPATH;
		break;
	case TELinkIntentMomentary:
		variable.VarIntent = EVarIntent::VARINTENT_MOMENTARY;
		break;
	case TELinkIntentPulse:
		variable.VarIntent = EVarIntent::VARINTENT_PULSE;
		break;
	}

	variableList.Add(variable);

	TERelease(&info);
	return result;
}


void
UTouchEngine::Copy(UTouchEngine* other)
{
	//FString			myToxPath;
	//TEInstance* myTEInstance = nullptr;
	myTEInstance = other->myTEInstance;
	//TEGraphicsContext*  = nullptr;
	myTEContext = other->myTEContext;
	//
	//ID3D11Device* myDevice = nullptr;
	myDevice = other->myDevice;
	//ID3D11DeviceContext* myImmediateContext = nullptr;
	myImmediateContext = other->myImmediateContext;
	//ID3D11On12Device* myD3D11On12 = nullptr;
	//myD3D11On12 = other->myD3D11On12;
	//
	//TMap<FString, FTouchCHOPSingleSample>	myCHOPOutputs;
	myCHOPSingleOutputs = other->myCHOPSingleOutputs;
	//FCriticalSection			myTOPLock;
	//myTOPLock = other->myTOPLock;
	//TMap<FString, FTouchTOP>	myTOPOutputs;
	myTOPOutputs = other->myTOPOutputs;
	//
	//FMessageLog					myMessageLog = FMessageLog(TEXT("TouchEngine"));
	myMessageLog = other->myMessageLog;
	//bool						myLogOpened = false;
	myLogOpened = other->myLogOpened;
	//FCriticalSection			myMessageLock;
	//myMessageLock = other->myMessageLock;
	//TArray<FString>				myErrors;
	myErrors = other->myErrors;
	//TArray<FString>				myWarnings;
	myWarnings = other->myWarnings;
	//
	//std::deque<TexCleanup>		myTexCleanups;
	myTexCleanups = other->myTexCleanups;
	//std::atomic<bool>			myDidLoad = false;
	//myDidLoad = other->myDidLoad;
	//
	//RHIType						myRHIType = RHIType::Invalid;
	myRHIType = other->myRHIType;
}

void
UTouchEngine::loadTox(FString toxPath)
{
	if (GIsCookerLoadingPackage)
		return;

	if (myDevice)
		clear();

	myToxPath = toxPath;
	myDidLoad = false;

	if (toxPath.IsEmpty() || !toxPath.EndsWith(".tox"))
	{
		outputError(FString::Printf(TEXT("loadTox(): Invalid file path - %s"), *toxPath));
		myFailedLoad = true;
		OnLoadFailed.Broadcast("Invalid file path");
		return;
	}

	FString rhiType = FApp::GetGraphicsRHI();
	if (rhiType == "DirectX 11")
	{
		myDevice = (ID3D11Device*)GDynamicRHI->RHIGetNativeDevice();
		myDevice->GetImmediateContext(&myImmediateContext);
		if (!myDevice || !myImmediateContext)
		{
			outputError(TEXT("loadTox(): Unable to obtain DX11 Device / Context."));
			myFailedLoad = true;
			OnLoadFailed.Broadcast("Unable to obtain DX11 Device / Context.");
			return;
		}
		myRHIType = RHIType::DirectX11;
	}
#if 0
	else if (rhiType == "DirectX 12")
	{
		FD3D12DynamicRHI* dx12RHI = static_cast<FD3D12DynamicRHI*>(GDynamicRHI);
		auto dx12Device = (ID3D12Device*)GDynamicRHI->RHIGetNativeDevice();
		ID3D12CommandQueue* queue = dx12RHI->RHIGetD3DCommandQueue();
		D3D_FEATURE_LEVEL feature;
		if (FAILED(D3D11On12CreateDevice(dx12Device, 0, NULL, 0, (IUnknown**)&queue, 1, 0, &myDevice, &myImmediateContext, &feature)))
		{
			outputError(TEXT("loadTox(): Unable to Create D3D11On12 Device."));
			myFailedLoad = true;
			OnLoadFailed.Broadcast("Unable to Create D3D11On12 Device.");
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
#endif
	else
	{
		outputError(*FString::Printf(TEXT("loadTox(): Unsupported RHI active: %s"), *rhiType));
		myFailedLoad = true;
		OnLoadFailed.Broadcast("Unsupported RHI active");
		return;
	}

	// TODO: need to make this work for all API options unreal works with
	TEResult result = TED3D11ContextCreate(myDevice, &myTEContext);
	TESeverity severity;

	if (result != TEResultSuccess)
	{
		outputResult(TEXT("loadTox(): Unable to create TouchEngine context: "), result);

		severity = TEResultGetSeverity(result);

		if (severity == TESeverity::TESeverityError)
		{
			myFailedLoad = true;
			OnLoadFailed.Broadcast("Unable to create TouchEngine context");
			return;
		}
	}


	result = TEInstanceCreate(eventCallback,
		linkValueCallback,
		this,
		myTEInstance.take());

	if (result != TEResultSuccess)
	{
		outputResult(TEXT("loadTox(): Unable to create TouchEngine instance: "), result);

		severity = TEResultGetSeverity(result);

		if (severity == TESeverity::TESeverityError)
		{
			myFailedLoad = true;
			OnLoadFailed.Broadcast("Unable to create TouchEngine instance");
			return;
		}
	}

	result = TEInstanceSetFrameRate(myTEInstance, myFrameRate, 1);

	if (result != TEResultSuccess)
	{
		outputResult(TEXT("loadTox(): Unable to set frame rate: "), result);

		severity = TEResultGetSeverity(result);

		if (severity == TESeverity::TESeverityError)
		{
			myFailedLoad = true;
			OnLoadFailed.Broadcast("Unable to set frame rate");
			return;
		}
	}

	result = TEInstanceLoad(myTEInstance,
		TCHAR_TO_UTF8(*toxPath),
		myTimeMode
	);

	if (result != TEResultSuccess)
	{
		outputResult(TEXT("loadTox(): Unable to load tox file: "), result);

		severity = TEResultGetSeverity(result);

		if (severity == TESeverity::TESeverityError)
		{
			myFailedLoad = true;
			OnLoadFailed.Broadcast("Unable to load tox file");
			return;
		}
	}

	result = TEInstanceAssociateGraphicsContext(myTEInstance, myTEContext);

	if (result != TEResultSuccess)
	{
		outputResult(TEXT("loadTox(): Unable to associate graphics context: "), result);

		severity = TEResultGetSeverity(result);

		if (severity == TESeverity::TESeverityError)
		{
			myFailedLoad = true;
			OnLoadFailed.Broadcast("Unable to associate graphics context");
			return;
		}
	}
	result = TEInstanceResume(myTEInstance);
	}

void
UTouchEngine::cookFrame(int64 FrameTime_Mill)
{
	outputMessages();
	if (myDidLoad && !myCooking)
	{
		// If myDidLoad is true, then we shouldn't have a null instance
		assert(myTEInstance);
		if (!myTEInstance)
			return;

		TEResult result = (TEResult)0;

		FlushRenderingCommands();
		switch (myTimeMode)
		{
		case TETimeInternal:
			result = TEInstanceStartFrameAtTime(myTEInstance, 0, 0, false);
			break;
		case TETimeExternal:
			myTime += FrameTime_Mill;
			result = TEInstanceStartFrameAtTime(myTEInstance, myTime, 10000, false);
			break;
		}

		switch (result)
		{
		case TEResult::TEResultSuccess:
			myCooking = true;
			break;
		default:
			outputResult(FString("cookFrame(): Failed to cook frame: "), result);
			break;
		}
	}
}

bool
UTouchEngine::setCookMode(bool IsIndependent)
{
	if (IsIndependent)
		myTimeMode = TETimeMode::TETimeInternal;
	else
		myTimeMode = TETimeMode::TETimeExternal;

	return true;
}

bool
UTouchEngine::setFrameRate(int64 frameRate)
{
	if (!myTEInstance)
	{
		myFrameRate = frameRate;
		return true;
	}

	return false;
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
UTouchEngine::addResult(const FString& s, TEResult result)
{
	FString s2(s);
	s2 += TEResultGetDescription(result);

	if (TEResultGetSeverity(result) == TESeverityError)
		addError(s2);
	else if (TEResultGetSeverity(result) == TESeverityWarning)
		addWarning(s2);
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
	for (auto& m : myErrors)
	{
		outputError(m);
	}
	for (auto& m : myWarnings)
	{
		outputWarning(m);
	}
#endif
	myErrors.Empty();
	myWarnings.Empty();
}

void
UTouchEngine::outputResult(const FString& s, TEResult result)
{
#ifdef WITH_EDITOR
	FString s2(s);
	s2 += TEResultGetDescription(result);

	if (TEResultGetSeverity(result) == TESeverityError)
		outputError(s2);
	else if (TEResultGetSeverity(result) == TESeverityWarning)
		outputWarning(s2);
#endif
}

void
UTouchEngine::outputError(const FString& s)
{
#ifdef WITH_EDITOR
	myMessageLog.Error(FText::FromString(FString::Printf(TEXT("TouchEngine error - %s"), *s)));
	if (!myLogOpened)
	{
		myMessageLog.Open(EMessageSeverity::Error, false);
		myLogOpened = true;
	}
	else
		myMessageLog.Notify(FText::FromString(FString(TEXT("TouchEngine Error"))), EMessageSeverity::Error);
#endif
}

void
UTouchEngine::outputWarning(const FString& s)
{
#ifdef WITH_EDITOR
	myMessageLog.Warning(FText::FromString(FString::Printf(TEXT("TouchEngine warning - "), *s)));
	if (!myLogOpened)
	{
		myMessageLog.Open(EMessageSeverity::Warning, false);
		myLogOpened = true;
	}
	else
		myMessageLog.Notify(FText::FromString(FString(TEXT("TouchEngine Warning"))), EMessageSeverity::Warning);
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

FTouchTOP
UTouchEngine::getTOPOutput(const FString& identifier)
{
	FTouchTOP c;
	if (!myDidLoad)
	{
		return c;
	}
	assert(myTEInstance);
	if (!myTEInstance)
		return c;

	auto doError =
		[this, &identifier]()
	{
		outputError(FString(TEXT("getTOPOutput(): Unable to find output named: ")) + identifier);
	};

	//std::string fullId("output/");
	std::string fullId("");
	fullId += TCHAR_TO_UTF8(*identifier);
	TELinkInfo* param = nullptr;
	TEResult result = TEInstanceLinkGetInfo(myTEInstance, fullId.c_str(), &param);

	if (result != TEResultSuccess)
	{
		doError();
		return c;
	}
	else
	{
		TERelease(&param);
	}

	FScopeLock lock(&myTOPLock);

	if (!myTOPOutputs.Contains(identifier))
	{
		myTOPOutputs.Add(identifier);
	}


	if (auto* top = myTOPOutputs.Find(identifier))
	{
		return *top;
	}
	else
	{
		doError();
		return c;
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


	//std::string fullId("input/");
	std::string fullId("");
	//std::string fullId("");
	fullId += TCHAR_TO_UTF8(*identifier);

	TELinkInfo* param = nullptr;
	TEResult result = TEInstanceLinkGetInfo(myTEInstance, fullId.c_str(), &param);

	if (result != TEResultSuccess)
	{
		outputResult(TEXT("setTOPInput(): Unable to get input info for: ") + identifier + ". ", result);
		return;
	}
	TERelease(&param);

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
				DXGI_FORMAT typedDXGIFormat = DXGI_FORMAT_UNKNOWN;
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

				if (typedDXGIFormat != DXGI_FORMAT_UNKNOWN)
				{
					if (d3d11Texture)
					{
						D3D11_TEXTURE2D_DESC desc;
						d3d11Texture->GetResource()->GetDesc(&desc);
						if (isTypeless(desc.Format))
						{
							teTexture = TED3D11TextureCreateTypeless(d3d11Texture->GetResource(), false, typedDXGIFormat);
						}
						else
						{
							teTexture = TED3D11TextureCreate(d3d11Texture->GetResource(), false);
						}
					}
					else
					{
						addError(TEXT("setTOPInput(): Unable to create TouchEngine copy of texture."));
					}
				}
				else
				{
					addError(TEXT("setTOPInput(): Unsupported pixel format for texture input. Compressed textures are not supported."));
				}
			}
			else if (myRHIType == RHIType::DirectX12)
			{
#if 0
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
						teTexture = TED3D11TextureCreate(wrappedResource, false);
			}
		}
#endif
}

			if (teTexture)
			{
				TEResult res = TEInstanceLinkSetTextureValue(myTEInstance, fullId.c_str(), teTexture, myTEContext);
				TERelease(&teTexture);
			}

			if (wrappedResource)
			{
				//myD3D11On12->ReleaseWrappedResources((ID3D11Resource**)&wrappedResource, 1);
				wrappedResource->Release();
			}
});
}

FTouchCHOPFull
UTouchEngine::getCHOPOutputSingleSample(const FString& identifier)
{
	if (!myDidLoad)
	{
		return FTouchCHOPFull();
	}

	FTouchCHOPFull f;

	//std::string fullId("output/");
	std::string fullId("");
	fullId += TCHAR_TO_UTF8(*identifier);

	TELinkInfo* param = nullptr;
	TEResult result = TEInstanceLinkGetInfo(myTEInstance, fullId.c_str(), &param);
	if (result == TEResultSuccess && param->scope == TEScopeOutput)
	{
		switch (param->type)
		{
		case TELinkTypeFloatBuffer:
		{

			TEFloatBuffer* buf = nullptr;
			result = TEInstanceLinkGetFloatBufferValue(myTEInstance, fullId.c_str(), TELinkValueDefault, &buf);

			if (result == TEResultSuccess && buf != nullptr)
			{
				if (!myCHOPSingleOutputs.Contains(identifier))
				{
					myCHOPSingleOutputs.Add(identifier);
				}

				auto& output = myCHOPSingleOutputs[identifier];

				int32_t channelCount = TEFloatBufferGetChannelCount(buf);
				int64_t maxSamples = TEFloatBufferGetValueCount(buf);

				int64_t length = maxSamples;

				double rate = TEFloatBufferGetRate(buf);
				if (!TEFloatBufferIsTimeDependent(buf))
				{
					const float* const* channels = TEFloatBufferGetValues(buf);
					const char* const* channelNames = TEFloatBufferGetChannelNames(buf);

					if (result == TEResultSuccess)
					{
						// Use the channel data here
						if (length > 0 && channelCount > 0)
						{
							//output.channelData.SetNum(channelCount * length);
							for (int i = 0; i < channelCount; i++)
							{
								f.sampleData.Add(FTouchCHOPSingleSample());
								f.sampleData[i].channelName = channelNames[i];

								for (int j = 0; j < length; j++)
								{
									//output.channelData[(i * length) + j] = channels[i][j];
									f.sampleData[i].channelData.Add(channels[i][j]);
								}
							}
						}
					}
					// Suppress internal errors for now, some superfluous ones are occuring currently
					else if (result != TEResultInternalError)
					{
						outputResult(TEXT("getCHOPOutputSingleSample(): "), result);
					}
					//c = output;
					TERelease(&buf);
				}
				else
				{
					//length /= rate / myFrameRate;

					const float* const* channels = TEFloatBufferGetValues(buf);
					const char* const* channelNames = TEFloatBufferGetChannelNames(buf);

					if (result == TEResultSuccess)
					{
						// Use the channel data here
						if (length > 0 && channelCount > 0)
						{
							output.channelData.SetNum(channelCount);

							for (int i = 0; i < channelCount; i++)
							{
								output.channelData[i] = channels[i][length - 1];
							}
							output.channelName = channelNames[0];
						}
					}
					// Suppress internal errors for now, some superfluous ones are occuring currently
					else if (result != TEResultInternalError)
					{
						outputResult(TEXT("getCHOPOutputSingleSample(): "), result);
					}
					f.sampleData.Add(output);
					TERelease(&buf);

				}
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
		outputResult(TEXT("getCHOPOutputSingleSample(): "), result);
	}
	else if (param->scope == TEScopeOutput)
	{
		outputError(TEXT("getCHOPOutputSingleSample(): ") + identifier + TEXT(" is not a CHOP output."));
	}
	TERelease(&param);

	return f;
}

void
UTouchEngine::setCHOPInputSingleSample(const FString& identifier, const FTouchCHOPSingleSample& chop)
{
	if (!myTEInstance)
		return;

	if (!myDidLoad)
	{
		return;
	}

	if (!chop.channelData.Num())
		return;

	//std::string fullId("input/");
	std::string fullId("");
	fullId += TCHAR_TO_UTF8(*identifier);


	TEResult result;
	TELinkInfo* info;
	result = TEInstanceLinkGetInfo(myTEInstance, fullId.c_str(), &info);

	if (result != TEResultSuccess)
	{
		outputResult(FString("setCHOPInputSingleSample(): Unable to get input info, ") + FString(identifier) + " may not exist. ", result);
		return;
	}

	if (info->type != TELinkTypeFloatBuffer)
	{
		outputError(FString("setCHOPInputSingleSample(): Input named: ") + FString(identifier) + " is not a CHOP input.");
		TERelease(&info);
		return;
	}

	std::vector<float>	realData;
	std::vector<const float*>	dataPtrs;
	std::vector<std::string> names;
	std::vector<const char*> namesPtrs;
	for (int i = 0; i < chop.channelData.Num(); i++)
	{
		realData.push_back(chop.channelData[i]);
		std::string n("chan");
		n += '1' + i;
		names.push_back(std::move(n));
	}
	// Seperate loop since realData can reallocate a few times
	for (int i = 0; i < chop.channelData.Num(); i++)
	{
		dataPtrs.push_back(&realData[i]);
		namesPtrs.push_back(names[i].c_str());
	}

	TEFloatBuffer* buf = TEFloatBufferCreate(-1.f, chop.channelData.Num(), 1, namesPtrs.data());

	result = TEFloatBufferSetValues(buf, dataPtrs.data(), 1);

	if (result != TEResultSuccess)
	{
		outputResult(FString("setCHOPInputSingleSample(): Failed to set buffer values: "), result);
		TERelease(&info);
		TERelease(&buf);
		return;
	}
	result = TEInstanceLinkAddFloatBuffer(myTEInstance, fullId.c_str(), buf);

	if (result != TEResultSuccess)
	{
		outputResult(FString("setCHOPInputSingleSample(): Unable to append buffer values: "), result);
		TERelease(&info);
		TERelease(&buf);
		return;
	}

	TERelease(&info);
	TERelease(&buf);
}

FTouchCHOPFull UTouchEngine::getCHOPOutputs(const FString& identifier)
{
	if (!myDidLoad)
	{
		return FTouchCHOPFull();
	}

	FTouchCHOPFull c;

	//std::string fullId("output/");
	std::string fullId("");
	fullId += TCHAR_TO_UTF8(*identifier);

	TELinkInfo* param = nullptr;
	TEResult result = TEInstanceLinkGetInfo(myTEInstance, fullId.c_str(), &param);
	if (result == TEResultSuccess && param->scope == TEScopeOutput)
	{
		switch (param->type)
		{
		case TELinkTypeFloatBuffer:
		{

			TEFloatBuffer* buf = nullptr;
			result = TEInstanceLinkGetFloatBufferValue(myTEInstance, fullId.c_str(), TELinkValueDefault, &buf);

			if (result == TEResultSuccess)
			{
				if (!myCHOPFullOutputs.Contains(identifier))
				{
					myCHOPFullOutputs.Add(identifier);
				}

				auto& output = myCHOPFullOutputs[identifier];

				int32_t channelCount = TEFloatBufferGetChannelCount(buf);
				int64_t maxSamples = TEFloatBufferGetValueCount(buf);
				const float* const* channels = TEFloatBufferGetValues(buf);
				const char* const* channelNames = TEFloatBufferGetChannelNames(buf);

				if (TEFloatBufferIsTimeDependent(buf))
				{

				}

				if (result == TEResultSuccess)
				{
					// Use the channel data here
					if (maxSamples > 0 && channelCount > 0)
					{
						output.sampleData.SetNum(channelCount);
						for (int i = 0; i < channelCount; i++)
						{
							output.sampleData[i].channelData.SetNum(maxSamples);
							output.sampleData[i].channelName = channelNames[i];
							for (int j = 0; j < maxSamples; j++)
							{
								output.sampleData[i].channelData[j] = channels[i][j];
							}
						}
					}
				}
				// Suppress internal errors for now, some superfluous ones are occuring currently
				else if (result != TEResultInternalError)
				{
					outputResult(TEXT("getCHOPOutputs(): "), result);
				}
				c = output;
				TERelease(&buf);
			}
			break;
		}
		default:
		{
			outputError(TEXT("getCHOPOutputs(): ") + identifier + TEXT(" is not a CHOP output."));
			break;
		}
		}
	}
	else if (result != TEResultSuccess)
	{
		outputResult(TEXT("getCHOPOutputs(): "), result);
	}
	else if (param->scope == TEScopeOutput)
	{
		outputError(TEXT("getCHOPOutputs(): ") + identifier + TEXT(" is not a CHOP output."));
	}
	TERelease(&param);

	return c;
}

void UTouchEngine::setCHOPInput(const FString& identifier, const FTouchCHOPFull& chop)
{
	/*
	if (!myTEInstance)
		return;

	if (!myDidLoad)
	{
		return;
	}

	if (!chop.sampleData.Num())
		return;

	//std::string fullId("input/");
	std::string fullId("");
	fullId += TCHAR_TO_UTF8(*identifier);


	TEResult result;
	TELinkInfo* info;
	result = TEInstanceLinkGetInfo(myTEInstance, fullId.c_str(), &info);

	if (result != TEResultSuccess)
	{
		outputResult(FString("setCHOPInput(): Unable to get input info, ") + FString(identifier) + " may not exist. ", result);
		return;
	}

	if (info->type != TELinkTypeFloatBuffer)
	{
		outputError(FString("setCHOPInput(): Input named: ") + FString(identifier) + " is not a CHOP input.");
		TERelease(&info);
		return;
	}

	std::vector<float>	realData;
	std::vector<const float*>	dataPtrs;
	std::vector<std::string> names;
	std::vector<const char*> namesPtrs;
	for (int i = 0; i < chop.sampleData.Num(); i++)
	{
		for (int j = 0; j < chop.sampleData[i].Num(); j++)
		{
			realData.push_back(chop.channelData[j]);
			std::string n("chan");
			n += '1' + j;
			names.push_back(std::move(n));
		}
	}
	// Seperate loop since realData can reallocate a few times
	for (int i = 0; i < chop.channelData.Num(); i++)
	{
		dataPtrs.push_back(&realData[i]);
		namesPtrs.push_back(names[i].c_str());
	}

	TEFloatBuffer* buf = TEFloatBufferCreate(-1.f, chop.sampleData[0].Num(), 1, namesPtrs.data());

	result = TEFloatBufferSetValues(buf, dataPtrs.data(), 1);

	if (result != TEResultSuccess)
	{
		outputResult(FString("setCHOPInput(): Failed to set buffer values: "), result);
		TERelease(&info);
		TERelease(&buf);
		return;
	}
	result = TEInstanceLinkAddFloatBuffer(myTEInstance, fullId.c_str(), buf);

	if (result != TEResultSuccess)
	{
		outputResult(FString("setCHOPInput(): Unable to append buffer values: "), result);
		TERelease(&info);
		TERelease(&buf);
		return;
	}

	TERelease(&info);
	TERelease(&buf);
	*/
}

FTouchVar<bool>
UTouchEngine::getBooleanOutput(const FString& identifier)
{
	if (!myDidLoad)
	{
		return FTouchVar<bool>();
	}

	FTouchVar<bool> c = FTouchVar<bool>();

	//std::string fullId("output/");
	std::string fullId("");
	fullId += TCHAR_TO_UTF8(*identifier);

	TELinkInfo* param = nullptr;
	TEResult result = TEInstanceLinkGetInfo(myTEInstance, fullId.c_str(), &param);
	if (result == TEResultSuccess && param->scope == TEScopeOutput)
	{
		switch (param->type)
		{
		case TELinkTypeBoolean:
		{
			//result = TEInstanceLinkGetFloatBufferValue(myTEInstance, fullId.c_str(), TELinkValueCurrent, &buf);
			result = TEInstanceLinkGetBooleanValue(myTEInstance, fullId.c_str(), TELinkValueCurrent, &c.data);

			if (result == TEResultSuccess)
			{
				if (!myCHOPSingleOutputs.Contains(identifier))
				{
					myCHOPSingleOutputs.Add(identifier);
				}
			}
			break;
		}
		default:
		{
			outputError(TEXT("getBooleanOutput(): ") + identifier + TEXT(" is not a boolean output."));
			break;
		}
		}
	}
	else if (result != TEResultSuccess)
	{
		outputResult(TEXT("getBooleanOutput(): "), result);
	}
	else if (param->scope == TEScopeOutput)
	{
		outputError(TEXT("getBooleanOutput(): ") + identifier + TEXT(" is not a boolean output."));
	}
	TERelease(&param);

	return c;
}

void
UTouchEngine::setBooleanInput(const FString& identifier, FTouchVar<bool>& op)
{
	if (!myTEInstance)
		return;

	if (!myDidLoad)
	{
		return;
	}

	//std::string fullId("input/");
	std::string fullId("");
	fullId += TCHAR_TO_UTF8(*identifier);

	TEResult result;
	TELinkInfo* info;
	result = TEInstanceLinkGetInfo(myTEInstance, fullId.c_str(), &info);

	if (result != TEResultSuccess)
	{
		outputResult(FString("setBooleanInput(): Unable to get input info, ") + FString(identifier) + " may not exist. ", result);
		return;
	}

	if (info->type != TELinkTypeBoolean)
	{
		outputError(FString("setBooleanInput(): Input named: ") + FString(identifier) + " is not a boolean input.");
		TERelease(&info);
		return;
	}

	result = TEInstanceLinkSetBooleanValue(myTEInstance, fullId.c_str(), op.data);

	if (result != TEResultSuccess)
	{
		outputResult(FString("setBooleanInput(): Unable to set boolean value: "), result);
		TERelease(&info);
		return;
	}

	TERelease(&info);
}

FTouchVar<double>
UTouchEngine::getDoubleOutput(const FString& identifier)
{
	if (!myDidLoad)
	{
		return FTouchVar<double>();
	}

	FTouchVar<double> c = FTouchVar<double>();

	//std::string fullId("output/");
	std::string fullId("");
	fullId += TCHAR_TO_UTF8(*identifier);

	TELinkInfo* param = nullptr;
	TEResult result = TEInstanceLinkGetInfo(myTEInstance, fullId.c_str(), &param);
	if (result == TEResultSuccess && param->scope == TEScopeOutput)
	{
		switch (param->type)
		{
		case TELinkTypeDouble:
		{
			//result = TEInstanceLinkGetFloatBufferValue(myTEInstance, fullId.c_str(), TELinkValueCurrent, &buf);
			result = TEInstanceLinkGetDoubleValue(myTEInstance, fullId.c_str(), TELinkValueCurrent, &c.data, 1);

			if (result == TEResultSuccess)
			{
				if (!myCHOPSingleOutputs.Contains(identifier))
				{
					myCHOPSingleOutputs.Add(identifier);
				}
			}
			break;
		}
		default:
		{
			outputError(TEXT("getDoubleOutput(): ") + identifier + TEXT(" is not a double output."));
			break;
		}
		}
	}
	else if (result != TEResultSuccess)
	{
		outputResult(TEXT("getDoubleOutput(): "), result);
	}
	else if (param->scope == TEScopeOutput)
	{
		outputError(TEXT("getDoubleOutput(): ") + identifier + TEXT(" is not a double output."));
	}
	TERelease(&param);

	return c;
}

void
UTouchEngine::setDoubleInput(const FString& identifier, FTouchVar<TArray<double>>& op)
{
	if (!myTEInstance)
		return;

	if (!myDidLoad)
	{
		return;
	}

	//std::string fullId("input/");
	std::string fullId("");
	fullId += TCHAR_TO_UTF8(*identifier);

	TEResult result;
	TELinkInfo* info;
	result = TEInstanceLinkGetInfo(myTEInstance, fullId.c_str(), &info);

	if (result != TEResultSuccess)
	{
		outputResult(FString("setDoubleInput(): Unable to get input info, ") + FString(identifier) + " may not exist. ", result);
		return;
	}

	if (info->type != TELinkTypeDouble)
	{
		outputError(FString("setDoubleInput(): Input named: ") + FString(identifier) + " is not a double input.");
		TERelease(&info);
		return;
	}

	if (op.data.Num() != info->count)
	{
		if (op.data.Num() > info->count)
		{
			TArray<double> buffer;

			for (int i = 0; i < info->count; i++)
			{
				buffer.Add(op.data[i]);
			}

			result = TEInstanceLinkSetDoubleValue(myTEInstance, fullId.c_str(), buffer.GetData(), info->count);
		}
		else
		{
			outputError(FString("setDoubleInput(): Unable to set double value: count mismatch"));
			TERelease(&info);
			return;
		}
	}
	else
	{
		result = TEInstanceLinkSetDoubleValue(myTEInstance, fullId.c_str(), op.data.GetData(), op.data.Num());
	}

	if (result != TEResultSuccess)
	{
		outputResult(FString("setDoubleInput(): Unable to set double value: "), result);
		TERelease(&info);
		return;
	}

	TERelease(&info);
}

FTouchVar<int32_t>
UTouchEngine::getIntegerOutput(const FString& identifier)
{
	if (!myDidLoad)
	{
		return FTouchVar<int32_t>();
	}

	FTouchVar<int32_t> c = FTouchVar<int32_t>();

	//std::string fullId("output/");
	std::string fullId("");
	fullId += TCHAR_TO_UTF8(*identifier);

	TELinkInfo* param = nullptr;
	TEResult result = TEInstanceLinkGetInfo(myTEInstance, fullId.c_str(), &param);
	if (result == TEResultSuccess && param->scope == TEScopeOutput)
	{
		switch (param->type)
		{
		case TELinkTypeBoolean:
		{
			//result = TEInstanceLinkGetFloatBufferValue(myTEInstance, fullId.c_str(), TELinkValueCurrent, &buf);
			result = TEInstanceLinkGetIntValue(myTEInstance, fullId.c_str(), TELinkValueCurrent, &c.data, 1);

			if (result == TEResultSuccess)
			{
				if (!myCHOPSingleOutputs.Contains(identifier))
				{
					myCHOPSingleOutputs.Add(identifier);
				}
			}
			break;
		}
		default:
		{
			outputError(TEXT("getIntegerOutput(): ") + identifier + TEXT(" is not an integer output."));
			break;
		}
		}
	}
	else if (result != TEResultSuccess)
	{
		outputResult(TEXT("getIntegerOutput(): "), result);
	}
	else if (param->scope == TEScopeOutput)
	{
		outputError(TEXT("getIntegerOutput(): ") + identifier + TEXT(" is not an integer output."));
	}
	TERelease(&param);

	return c;
}

void
UTouchEngine::setIntegerInput(const FString& identifier, FTouchVar<TArray<int32_t>>& op)
{
	if (!myTEInstance)
		return;

	if (!myDidLoad)
	{
		return;
	}

	//std::string fullId("input/");
	std::string fullId("");
	fullId += TCHAR_TO_UTF8(*identifier);

	TEResult result;
	TELinkInfo* info;
	result = TEInstanceLinkGetInfo(myTEInstance, fullId.c_str(), &info);

	if (result != TEResultSuccess)
	{
		outputResult(FString("setIntegerInput(): Unable to get input info, ") + FString(identifier) + " may not exist. ", result);
		return;
	}

	if (info->type != TELinkTypeInt)
	{
		outputError(FString("setIntegerInput(): Input named: ") + FString(identifier) + " is not an integer input.");
		TERelease(&info);
		return;
	}

	result = TEInstanceLinkSetIntValue(myTEInstance, fullId.c_str(), op.data.GetData(), op.data.Num());

	if (result != TEResultSuccess)
	{
		outputResult(FString("setIntegerInput(): Unable to set integer value: "), result);
		TERelease(&info);
		return;
	}

	TERelease(&info);
}

FTouchVar<TEString*>
UTouchEngine::getStringOutput(const FString& identifier)
{
	if (!myDidLoad)
	{
		return FTouchVar<TEString*>();
	}

	FTouchVar<TEString*> c = FTouchVar<TEString*>();

	////std::string fullId("output/");std::string fullId("");
	std::string fullId("");
	fullId += TCHAR_TO_UTF8(*identifier);

	TELinkInfo* param = nullptr;
	TEResult result = TEInstanceLinkGetInfo(myTEInstance, fullId.c_str(), &param);
	if (result == TEResultSuccess && param->scope == TEScopeOutput)
	{
		switch (param->type)
		{
		case TELinkTypeString:
		{
			//result = TEInstanceLinkGetFloatBufferValue(myTEInstance, fullId.c_str(), TELinkValueCurrent, &buf);
			result = TEInstanceLinkGetStringValue(myTEInstance, fullId.c_str(), TELinkValueCurrent, &c.data);

			if (result == TEResultSuccess)
			{
				if (!myCHOPSingleOutputs.Contains(identifier))
				{
					myCHOPSingleOutputs.Add(identifier);
				}
			}
			break;
		}
		default:
		{
			outputError(TEXT("getStringOutput(): ") + identifier + TEXT(" is not a string output."));
			break;
		}
		}
	}
	else if (result != TEResultSuccess)
	{
		outputResult(TEXT("getStringOutput(): "), result);
	}
	else if (param->scope == TEScopeOutput)
	{
		outputError(TEXT("getStringOutput(): ") + identifier + TEXT(" is not a string output."));
	}
	TERelease(&param);

	return c;
}

void
UTouchEngine::setStringInput(const FString& identifier, FTouchVar<char*>& op)
{
	if (!myTEInstance)
		return;

	if (!myDidLoad)
	{
		return;
	}

	//std::string fullId("input/");
	std::string fullId("");
	fullId += TCHAR_TO_UTF8(*identifier);

	TEResult result;
	TELinkInfo* info;
	result = TEInstanceLinkGetInfo(myTEInstance, fullId.c_str(), &info);

	if (result != TEResultSuccess)
	{
		outputResult(FString("setStringInput(): Unable to get input info, ") + FString(identifier) + " may not exist. ", result);
		return;
	}

	if (info->type == TELinkTypeString)
	{
		result = TEInstanceLinkSetStringValue(myTEInstance, fullId.c_str(), op.data);
	}
	else if (info->type == TELinkTypeStringData)
	{
		TETable* table = TETableCreate();
		TETableResize(table, 1, 1);
		TETableSetStringValue(table, 0, 0, op.data);

		result = TEInstanceLinkSetTableValue(myTEInstance, fullId.c_str(), table);
		TERelease(&table);
	}
	else
	{
		outputError(FString("setStringInput(): Input named: ") + FString(identifier) + " is not a string input.");
		TERelease(&info);
		return;
	}


	if (result != TEResultSuccess)
	{
		outputResult(FString("setStringInput(): Unable to set string value: "), result);
		TERelease(&info);
		return;
	}

	TERelease(&info);
}

FTouchDATFull
UTouchEngine::getTableOutput(const FString& identifier)
{
	if (!myDidLoad)
		return FTouchDATFull();

	FTouchDATFull c = FTouchDATFull();

	////std::string fullId("output/");std::string fullId("");
	std::string fullId("");
	fullId += TCHAR_TO_UTF8(*identifier);

	TELinkInfo* param = nullptr;
	TEResult result = TEInstanceLinkGetInfo(myTEInstance, fullId.c_str(), &param);
	if (result == TEResultSuccess && param->scope == TEScopeOutput)
	{
		switch (param->type)
		{
		case TELinkTypeStringData:
		{
			//result = TEInstanceLinkGetStringValue(myTEInstance, fullId.c_str(), TELinkValueCurrent, &c.data);
			result = TEInstanceLinkGetTableValue(myTEInstance, fullId.c_str(), TELinkValue::TELinkValueCurrent, &c.channelData);
			break;
		}
		default:
		{
			outputError(TEXT("getTableOutput(): ") + identifier + TEXT(" is not a table output."));
			break;
		}
		}
	}
	else if (result != TEResultSuccess)
	{
		outputResult(TEXT("getTableOutput(): "), result);
	}
	else if (param->scope == TEScopeOutput)
	{
		outputError(TEXT("getTableOutput(): ") + identifier + TEXT(" is not a table output."));
	}
	TERelease(&param);

	return c;
}

void
UTouchEngine::setTableInput(const FString& identifier, FTouchDATFull& op)
{
	if (!myTEInstance)
		return;

	if (!myDidLoad)
	{
		return;
	}

	//std::string fullId("input/");
	std::string fullId("");
	fullId += TCHAR_TO_UTF8(*identifier);

	TEResult result;
	TELinkInfo* info;
	result = TEInstanceLinkGetInfo(myTEInstance, fullId.c_str(), &info);

	if (result != TEResultSuccess)
	{
		outputResult(FString("setTableInput(): Unable to get input info, ") + FString(identifier) + " may not exist. ", result);
		return;
	}

	if (info->type == TELinkTypeString)
	{
		const char* string = TETableGetStringValue(op.channelData, 0, 0);
		result = TEInstanceLinkSetStringValue(myTEInstance, fullId.c_str(), string);
	}
	else if (info->type == TELinkTypeStringData)
	{
		result = TEInstanceLinkSetTableValue(myTEInstance, fullId.c_str(), op.channelData);
	}
	else
	{
		outputError(FString("setTableInput(): Input named: ") + FString(identifier) + " is not a table input.");
		TERelease(&info);
		return;
	}


	if (result != TEResultSuccess)
	{
		outputResult(FString("setTableInput(): Unable to set table value: "), result);
		TERelease(&info);
		return;
	}

	TERelease(&info);
}
