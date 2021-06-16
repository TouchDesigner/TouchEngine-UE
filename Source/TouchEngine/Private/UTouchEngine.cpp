/* Shared Use License: This file is owned by Derivative Inc. (Derivative)
* and can only be used, and/or modified for use, in conjunction with
* Derivative's TouchDesigner software, and only if you are a licensee who has
* accepted Derivative's TouchDesigner license or assignment agreement
* (which also govern the use of this file). You may share or redistribute
* a modified version of this file provided the following conditions are met:
*
* 1. The shared file or redistribution must retain the Information set out
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

#define LOCTEXT_NAMESPACE "UTouchEngine"

void UTouchEngine::BeginDestroy()
{
	Clear();

	Super::BeginDestroy();
}

void UTouchEngine::Clear()
{
	MyCHOPSingleOutputs.Empty();

	FScopeLock lock(&MyTOPLock);


	ENQUEUE_RENDER_COMMAND(void)(
		[this](FRHICommandListImmediate& RHICmdList)
		{
			CleanupTextures(MyImmediateContext, &MyTexCleanups, FinalClean::True);
			if (MyImmediateContext)
				MyImmediateContext->Release();
			MyTEContext.reset();
			MyTexCleanups.clear();
			MyImmediateContext = nullptr;
			MyTEInstance.reset();
			MyDevice = nullptr;
			MyFailedLoad = false;
			MyToxPath = "";
		});
}


const FString& UTouchEngine::GetToxPath() const
{
	return MyToxPath;
}

void UTouchEngine::EventCallback(TEInstance* Instance, TEEvent Event, TEResult Result, int64_t StartTimeValue, int32_t StartTimeScale, int64_t EndTimeValue, int32_t EndTimeScale, void* Info)
{
	UTouchEngine* engine = static_cast<UTouchEngine*>(Info);
	if (!engine)
		return;

	switch (Event)
	{
	case TEEventInstanceDidLoad:
	{
		if (Result == TEResultSuccess)
		{
			engine->SetDidLoad();

			// Broadcast Links loaded Event
			TArray<FTouchEngineDynamicVariableStruct> VariablesIn, VariablesOut;

			for (TEScope scope : { TEScopeInput, TEScopeOutput })
			{
				TEStringArray* groups;
				Result = TEInstanceGetLinkGroups(Instance, scope, &groups);

				if (Result == TEResultSuccess)
				{
					for (int32_t i = 0; i < groups->count; i++)
					{
						switch (scope)
						{
						case TEScopeInput:
						{
							engine->ParseGroup(Instance, groups->strings[i], VariablesIn);
							break;
						}
						case TEScopeOutput:
						{
							engine->ParseGroup(Instance, groups->strings[i], VariablesOut);
							break;
						}
						}
					}
				}

			}

			UTouchEngine* savedEngine = engine;
			AsyncTask(ENamedThreads::GameThread, [savedEngine, VariablesIn, VariablesOut]()
				{
					savedEngine->OnParametersLoaded.Broadcast(VariablesIn, VariablesOut);
				}
			);
		}
		else if (Result == TEResultFileError)
		{
			UTouchEngine* savedEngine = engine;
			AsyncTask(ENamedThreads::GameThread, [savedEngine]()
				{
					savedEngine->AddError("load() failed to load .tox: " + savedEngine->MyToxPath);
					savedEngine->MyFailedLoad = true;
					savedEngine->OnLoadFailed.Broadcast("file error");
				}
			);
		}
		else if (Result == TEResultIncompatibleEngineVersion)
		{
			UTouchEngine* savedEngine = engine;
			AsyncTask(ENamedThreads::GameThread, [savedEngine]()
				{
					savedEngine->AddError("plugin version is incompatible with TouchDesigner version");
					savedEngine->MyFailedLoad = true;
					savedEngine->OnLoadFailed.Broadcast("plugin version is incompatible with TouchDesigner version");
				}
			);
		}
		else
		{

			TESeverity severity = TEResultGetSeverity(Result);

			if (severity == TESeverity::TESeverityError)
			{
				UTouchEngine* savedEngine = engine;
				TEResult savedResult = Result;
				AsyncTask(ENamedThreads::GameThread, [savedEngine, savedResult]()
					{
						savedEngine->AddResult("load(): tox file severe warning", savedResult);
						savedEngine->MyFailedLoad = true;
						savedEngine->OnLoadFailed.Broadcast("severe warning");
					}
				);
			}
			else
			{
				engine->SetDidLoad();

				// Broadcast Links loaded Event
				TArray<FTouchEngineDynamicVariableStruct> VariablesIn, VariablesOut;

				for (TEScope scope : { TEScopeInput, TEScopeOutput })
				{
					TEStringArray* groups;
					Result = TEInstanceGetLinkGroups(Instance, scope, &groups);

					if (Result == TEResultSuccess)
					{
						for (int32_t i = 0; i < groups->count; i++)
						{
							switch (scope)
							{
							case TEScopeInput:
							{
								engine->ParseGroup(Instance, groups->strings[i], VariablesIn);
								break;
							}
							case TEScopeOutput:
							{
								engine->ParseGroup(Instance, groups->strings[i], VariablesOut);
								break;
							}
							}
						}
					}

				}

				UTouchEngine* savedEngine = engine;
				AsyncTask(ENamedThreads::GameThread, [savedEngine, VariablesIn, VariablesOut]()
					{
						savedEngine->OnParametersLoaded.Broadcast(VariablesIn, VariablesOut);
					}
				);
			}
		}
		break;
	}
	case TEEventFrameDidFinish:
	{
		engine->MyCooking = false;
		engine->OnCookFinished.Broadcast();
		break;
	}
	case TEEventGeneral:
	default:
		break;
	}
}

void UTouchEngine::LinkValueCallback(TEInstance* Instance, TELinkEvent Event, const char* Identifier, void* Info)
{
	UTouchEngine* doc = static_cast<UTouchEngine*>(Info);
	doc->LinkValueCallback(Instance, Event, Identifier);
}

void UTouchEngine::CleanupTextures(ID3D11DeviceContext* Context, std::deque<TexCleanup>* Cleanups, FinalClean FC)
{
	if (Cleanups == nullptr)
		return;


	while (!Cleanups->empty())
	{
		TexCleanup& cleanup = Cleanups->front();

		BOOL Result = false;

		Context->GetData(cleanup.Query, &Result, sizeof(Result), 0);
		if (Result)
		{
			TERelease(&cleanup.Texture);
			cleanup.Query->Release();
			Cleanups->pop_front();
		}
		else
		{
			if (FC == FinalClean::True)
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

static EPixelFormat toEPixelFormat(DXGI_FORMAT Format)
{
	switch (Format)
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


static DXGI_FORMAT toTypedDXGIFormat(EPixelFormat Format)
{
	switch (Format)
	{
	case PF_DXT1:
	case PF_DXT3:
	case PF_DXT5:
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


void UTouchEngine::LinkValueCallback(TEInstance* Instance, TELinkEvent Event, const char* Identifier)
{
	if (!Instance)
		return;

	TELinkInfo* param = nullptr;
	TEResult Result = TEInstanceLinkGetInfo(Instance, Identifier, &param);

	if (Result == TEResultSuccess && param && param->scope == TEScopeOutput)
	{
		switch (Event)
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
			if (!MyTEInstance)
				return;
			switch (param->type)
			{
			case TELinkTypeTexture:
			{
				// Stash the state, we don't do any actual renderer work from this thread
				TEDXGITexture* dxgiTexture = nullptr;
				Result = TEInstanceLinkGetTextureValue(MyTEInstance, Identifier, TELinkValueCurrent, &dxgiTexture);

				if (Result != TEResultSuccess)
				{
					// possible crash without this check
					return;
				}

				FString name(Identifier);
				ENQUEUE_RENDER_COMMAND(void)(
					[this, name, dxgiTexture](FRHICommandListImmediate& RHICmdList)
					{
						CleanupTextures(MyImmediateContext, &MyTexCleanups, FinalClean::False);

						TED3D11Texture* teD3DTexture = nullptr;

						FScopeLock lock(&MyTOPLock);

						TED3D11ContextCreateTexture(MyTEContext, dxgiTexture, &teD3DTexture);

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


						if (!MyTOPOutputs.Contains(name))
						{
							MyTOPOutputs.Add(name);
						}

						EPixelFormat pixelFormat = toEPixelFormat(desc.Format);

						if (pixelFormat == PF_Unknown)
						{
							AddError(TEXT("Texture with unsupported pixel format being generated by TouchEngine."));
							return;
						}
						FTouchTOP& output = MyTOPOutputs[name];

						if (!output.Texture ||
							output.Texture->GetSizeX() != desc.Width ||
							output.Texture->GetSizeY() != desc.Height ||
							output.Texture->GetPixelFormat() != pixelFormat)
						{
							if (output.WrappedResource)
							{
								output.WrappedResource->Release();
								output.WrappedResource = nullptr;
							}
							output.Texture = nullptr;
							output.Texture = UTexture2D::CreateTransient(desc.Width, desc.Height, pixelFormat);
							output.Texture->UpdateResource();
						}
						UTexture2D* destTexture = output.Texture;

						if (!destTexture->Resource)
						{
							TERelease(&teD3DTexture);
							TERelease(&dxgiTexture);
							return;
						}

						ID3D11Resource* destResource = nullptr;
						if (MyRHIType == RHIType::DirectX11)
						{
							FD3D11TextureBase* d3d11Texture = GetD3D11TextureFromRHITexture(destTexture->Resource->TextureRHI);
							destResource = d3d11Texture->GetResource();
						}
						else if (MyRHIType == RHIType::DirectX12)
						{
						}

						if (destResource)
						{
							MyImmediateContext->CopyResource(destResource, d3dSrcTexture);

							if (MyRHIType == RHIType::DirectX12)
							{

							}

							// TODO: We get a crash if we release the teD3DTexture here,
							// so we differ it until D3D tells us it's done with it.
							// Ideally we should be able to release it here and let the ref
							// counting cause it to be cleaned up on it's own later on
							TexCleanup cleanup;
							D3D11_QUERY_DESC queryDesc = {};
							queryDesc.Query = D3D11_QUERY_EVENT;
							HRESULT hresult = MyDevice->CreateQuery(&queryDesc, &cleanup.Query);
							if (hresult == 0)
							{
								MyImmediateContext->End(cleanup.Query);
								if (teD3DTexture)
								{
									cleanup.Texture = teD3DTexture;

									MyTexCleanups.push_back(cleanup);
								}
							}
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


TEResult UTouchEngine::ParseGroup(TEInstance* Instance, const char* Identifier, TArray<FTouchEngineDynamicVariableStruct>& Variables)
{
	// load each group
	TELinkInfo* group;
	TEResult Result = TEInstanceLinkGetInfo(Instance, Identifier, &group);

	if (Result != TEResultSuccess)
	{
		// failure
		return Result;
	}

	// use group data here

	TERelease(&group);

	// load children of each group
	TEStringArray* children = nullptr;
	Result = TEInstanceLinkGetChildren(Instance, Identifier, &children);

	if (Result != TEResultSuccess)
	{
		//failure 
		return Result;
	}

	// use children data here 
	for (int i = 0; i < children->count; i++)
	{
		Result = ParseInfo(Instance, children->strings[i], Variables);
	}

	TERelease(&children);
	return Result;
}

TEResult UTouchEngine::ParseInfo(TEInstance* Instance, const char* Identifier, TArray<FTouchEngineDynamicVariableStruct>& VariableList)
{
	TELinkInfo* Info;
	TEResult Result = TEInstanceLinkGetInfo(Instance, Identifier, &Info);

	if (Result != TEResultSuccess)
	{
		//failure
		return Result;
	}

	// parse our children into a dynamic Variable struct
	FTouchEngineDynamicVariableStruct Variable;

	Variable.VarLabel = FString(Info->label);

	FString domainChar = "";

	switch (Info->domain)
	{
	case TELinkDomainNone:
	case TELinkDomainParameterPage:
		break;
	case TELinkDomainParameter:
	{
		domainChar = "p";
		break;
	}
	case TELinkDomainOperator:
	{
		switch (Info->scope)
		{
		case TEScopeInput:
		{
			domainChar = "i";
			break;
		}
		case TEScopeOutput:
		{
			domainChar = "o";
			break;
		}
		}
		break;
	}
	}

	Variable.VarName = domainChar.Append("/").Append(Info->name);
	Variable.VarIdentifier = FString(Info->identifier);
	Variable.Count = Info->count;
	if (Variable.Count > 1)
		Variable.IsArray = true;

	// figure out what type 
	switch (Info->type)
	{
	case TELinkTypeGroup:
	{
		Result = ParseGroup(Instance, Identifier, VariableList);
		break;
	}
	case TELinkTypeComplex:
	{
		Variable.VarType = EVarType::NotSet;
		break;
	}
	case TELinkTypeBoolean:
	{
		Variable.VarType = EVarType::Bool;
		if (Info->domain == TELinkDomainParameter || (Info->domain == TELinkDomainOperator && Info->scope == TEScopeInput))
		{
			bool defaultVal;
			Result = TEInstanceLinkGetBooleanValue(Instance, Identifier, TELinkValueDefault, &defaultVal);

			if (Result == TEResult::TEResultSuccess)
			{
				Variable.SetValue(defaultVal);
			}
		}
		break;
	}
	case TELinkTypeDouble:
	{
		Variable.VarType = EVarType::Double;

		if (Info->domain == TELinkDomainParameter || (Info->domain == TELinkDomainOperator && Info->scope == TEScopeInput))
		{
			if (Info->count == 1)
			{
				double defaultVal;
				Result = TEInstanceLinkGetDoubleValue(Instance, Identifier, TELinkValueDefault, &defaultVal, 1);

				if (Result == TEResult::TEResultSuccess)
				{
					Variable.SetValue(defaultVal);
				}
			}
			else
			{
				double* defaultVal = (double*)_alloca(sizeof(double) * Info->count);
				Result = TEInstanceLinkGetDoubleValue(Instance, Identifier, TELinkValueDefault, defaultVal, Info->count);

				if (Result == TEResult::TEResultSuccess)
				{
					TArray<double> buffer;

					for (int i = 0; i < Info->count; i++)
					{
						buffer.Add(defaultVal[i]);
					}

					Variable.SetValue(buffer);
				}
			}
		}
		break;
	}
	case TELinkTypeInt:
	{
		Variable.VarType = EVarType::Int;

		if (Info->domain == TELinkDomainParameter || (Info->domain == TELinkDomainOperator && Info->scope == TEScopeInput))
		{
			if (Info->count == 1)
			{
				TEStringArray* choiceLabels = nullptr;
				Result = TEInstanceLinkGetChoiceLabels(Instance, Info->identifier, &choiceLabels);

				if (choiceLabels)
				{
					Variable.VarIntent = EVarIntent::DropDown;

#if WITH_EDITORONLY_DATA
					for (int i = 0; i < choiceLabels->count; i++)
					{
						Variable.DropDownData.Add(choiceLabels->strings[i], i);
					}
#endif

					TERelease(&choiceLabels);
				}

				FTouchVar<int32_t> c;
				Result = TEInstanceLinkGetIntValue(Instance, Identifier, TELinkValueDefault, &c.Data, 1);

				if (Result == TEResult::TEResultSuccess)
				{
					Variable.SetValue((int)c.Data);
				}
			}
			else
			{
				FTouchVar<int32_t*> c;
				c.Data = (int32_t*)_alloca(sizeof(int32_t) * 4);

				Result = TEInstanceLinkGetIntValue(Instance, Identifier, TELinkValueDefault, c.Data, Info->count);

				if (Result == TEResult::TEResultSuccess)
				{
					TArray<int> values;

					for (int i = 0; i < Info->count; i++)
					{
						values.Add((int)c.Data[i]);
					}

					Variable.SetValue(values);
				}
			}
		}
		break;
	}
	case TELinkTypeString:
	{
		Variable.VarType = EVarType::String;

		if (Info->domain == TELinkDomainParameter || (Info->domain == TELinkDomainOperator && Info->scope == TEScopeInput))
		{
			if (Info->count == 1)
			{
				TEStringArray* choiceLabels = nullptr;
				Result = TEInstanceLinkGetChoiceLabels(Instance, Info->identifier, &choiceLabels);

				if (choiceLabels)
				{
					Variable.VarIntent = EVarIntent::DropDown;

#if WITH_EDITORONLY_DATA
					for (int i = 0; i < choiceLabels->count; i++)
					{
						Variable.DropDownData.Add(choiceLabels->strings[i], i);
					}
#endif

					TERelease(&choiceLabels);
				}


				TEString* defaultVal = nullptr;
				Result = TEInstanceLinkGetStringValue(Instance, Identifier, TELinkValueDefault, &defaultVal);

				if (Result == TEResult::TEResultSuccess)
				{
					Variable.SetValue(FString(defaultVal->string));
				}
				TERelease(&defaultVal);
			}
			else
			{
				TEString* defaultVal = nullptr;
				Result = TEInstanceLinkGetStringValue(Instance, Identifier, TELinkValueDefault, &defaultVal);

				if (Result == TEResult::TEResultSuccess)
				{
					TArray<FString> values;
					for (int i = 0; i < Info->count; i++)
					{
						values.Add(FString(defaultVal[i].string));
					}

					Variable.SetValue(values);
				}
				TERelease(&defaultVal);
			}
		}
		break;
	}
	case TELinkTypeTexture:
	{
		Variable.VarType = EVarType::Texture;

		// textures have no valid default values 
		break;
	}
	case TELinkTypeFloatBuffer:
	{
		Variable.VarType = EVarType::CHOP;
		Variable.IsArray = true;

		if (Info->domain == TELinkDomainParameter || (Info->domain == TELinkDomainOperator && Info->scope == TEScopeInput))
		{
			TEFloatBuffer* buf = nullptr;
			Result = TEInstanceLinkGetFloatBufferValue(Instance, Identifier, TELinkValueDefault, &buf);

			if (Result == TEResult::TEResultSuccess)
			{
				TArray<float> values;
				int maxChannels = TEFloatBufferGetChannelCount(buf);
				const float* const* channels = TEFloatBufferGetValues(buf);

				for (int i = 0; i < maxChannels; i++)
				{
					values.Add(*channels[i]);
				}

				Variable.SetValue(values);
			}
			TERelease(&buf);
		}
		break;
	}
	case TELinkTypeStringData:
	{
		Variable.VarType = EVarType::String;
		Variable.IsArray = true;

		if (Info->domain == TELinkDomainParameter || (Info->domain == TELinkDomainOperator && Info->scope == TEScopeInput))
		{
		}
		break;
	}
	case TELinkTypeSeparator:
	{
		Variable.VarType = EVarType::NotSet;
		return Result;
		break;
	}
	}

	switch (Info->intent)
	{
	case TELinkIntentColorRGBA:
	{
		Variable.VarIntent = EVarIntent::Color;
		break;
	}
	case TELinkIntentPositionXYZW:
	{
		Variable.VarIntent = EVarIntent::Position;
		break;
	}
	case TELinkIntentSizeWH:
	{
		Variable.VarIntent = EVarIntent::Size;
		break;
	}
	case TELinkIntentUVW:
	{
		Variable.VarIntent = EVarIntent::UVW;
		break;
	}
	case TELinkIntentFilePath:
	{
		Variable.VarIntent = EVarIntent::FilePath;
		break;
	}
	case TELinkIntentDirectoryPath:
	{
		Variable.VarIntent = EVarIntent::DirectoryPath;
		break;
	}
	case TELinkIntentMomentary:
	{
		Variable.VarIntent = EVarIntent::Momentary;
		break;
	}
	case TELinkIntentPulse:
	{
		Variable.VarIntent = EVarIntent::Pulse;
		break;
	}
	}

	VariableList.Add(Variable);

	TERelease(&Info);
	return Result;
}


void UTouchEngine::Copy(UTouchEngine* Other)
{
	MyTEInstance = Other->MyTEInstance;
	MyTEContext = Other->MyTEContext;
	MyDevice = Other->MyDevice;
	MyImmediateContext = Other->MyImmediateContext;
	MyCHOPSingleOutputs = Other->MyCHOPSingleOutputs;
	MyTOPOutputs = Other->MyTOPOutputs;
	MyMessageLog = Other->MyMessageLog;
	MyLogOpened = Other->MyLogOpened;
	MyErrors = Other->MyErrors;
	MyWarnings = Other->MyWarnings;
	MyTexCleanups = Other->MyTexCleanups;
	MyRHIType = Other->MyRHIType;
}

void UTouchEngine::LoadTox(FString ToxPath)
{
	if (GIsCookerLoadingPackage)
		return;

	if (MyDevice)
		Clear();

	MyToxPath = ToxPath;
	MyDidLoad = false;

	if (ToxPath.IsEmpty() || !ToxPath.EndsWith(".tox"))
	{
		OutputError(FString::Printf(TEXT("loadTox(): Invalid file path - %s"), *ToxPath));
		MyFailedLoad = true;
		OnLoadFailed.Broadcast("Invalid file path");
		return;
	}

	FString rhiType = FApp::GetGraphicsRHI();
	if (rhiType == "DirectX 11")
	{
		MyDevice = (ID3D11Device*)GDynamicRHI->RHIGetNativeDevice();
		MyDevice->GetImmediateContext(&MyImmediateContext);
		if (!MyDevice || !MyImmediateContext)
		{
			OutputError(TEXT("loadTox(): Unable to obtain DX11 Device / Context."));
			MyFailedLoad = true;
			OnLoadFailed.Broadcast("Unable to obtain DX11 Device / Context.");
			return;
		}
		MyRHIType = RHIType::DirectX11;
	}
	else
	{
		OutputError(*FString::Printf(TEXT("loadTox(): Unsupported RHI active: %s"), *rhiType));
		MyFailedLoad = true;
		OnLoadFailed.Broadcast("Unsupported RHI active");
		return;
	}

	// TODO: need to make this work for all API options unreal works with
	TEResult Result = TED3D11ContextCreate(MyDevice, MyTEContext.take());
	TESeverity severity;

	if (Result != TEResultSuccess)
	{
		OutputResult(TEXT("loadTox(): Unable to create TouchEngine Context: "), Result);

		severity = TEResultGetSeverity(Result);

		if (severity == TESeverity::TESeverityError)
		{
			MyFailedLoad = true;
			OnLoadFailed.Broadcast("Unable to create TouchEngine Context");
			return;
		}
	}


	Result = TEInstanceCreate(EventCallback,
		LinkValueCallback,
		this,
		MyTEInstance.take());

	if (Result != TEResultSuccess)
	{
		OutputResult(TEXT("loadTox(): Unable to create TouchEngine Instance: "), Result);

		severity = TEResultGetSeverity(Result);

		if (severity == TESeverity::TESeverityError)
		{
			MyFailedLoad = true;
			OnLoadFailed.Broadcast("Unable to create TouchEngine Instance");
			return;
		}
	}

	Result = TEInstanceSetFrameRate(MyTEInstance, MyFrameRate, 1);

	if (Result != TEResultSuccess)
	{
		OutputResult(TEXT("loadTox(): Unable to set frame rate: "), Result);

		severity = TEResultGetSeverity(Result);

		if (severity == TESeverity::TESeverityError)
		{
			MyFailedLoad = true;
			OnLoadFailed.Broadcast("Unable to set frame rate");
			return;
		}
	}

	Result = TEInstanceLoad(MyTEInstance,
		TCHAR_TO_UTF8(*ToxPath),
		MyTimeMode
	);

	if (Result != TEResultSuccess)
	{
		OutputResult(TEXT("loadTox(): Unable to load tox file: "), Result);

		severity = TEResultGetSeverity(Result);

		if (severity == TESeverity::TESeverityError)
		{
			MyFailedLoad = true;
			OnLoadFailed.Broadcast("Unable to load tox file");
			return;
		}
	}

	Result = TEInstanceAssociateGraphicsContext(MyTEInstance, MyTEContext);

	if (Result != TEResultSuccess)
	{
		OutputResult(TEXT("loadTox(): Unable to associate graphics Context: "), Result);

		severity = TEResultGetSeverity(Result);

		if (severity == TESeverity::TESeverityError)
		{
			MyFailedLoad = true;
			OnLoadFailed.Broadcast("Unable to associate graphics Context");
			return;
		}
	}
	Result = TEInstanceResume(MyTEInstance);
}

void UTouchEngine::CookFrame(int64 FrameTime_Mill)
{
	OutputMessages();
	if (MyDidLoad && !MyCooking)
	{
		// If MyDidLoad is true, then we shouldn't have a null Instance
		assert(MyTEInstance);
		if (!MyTEInstance)
			return;

		TEResult Result = (TEResult)0;

		FlushRenderingCommands();
		switch (MyTimeMode)
		{
		case TETimeInternal:
		{
			Result = TEInstanceStartFrameAtTime(MyTEInstance, 0, 0, false);
			break;
		}
		case TETimeExternal:
		{
			MyTime += FrameTime_Mill;
			Result = TEInstanceStartFrameAtTime(MyTEInstance, MyTime, 10000, false);
			break;
		}
		}

		switch (Result)
		{
		case TEResult::TEResultSuccess:
		{
			MyCooking = true;
			break;
		}
		default:
		{
			OutputResult(FString("cookFrame(): Failed to cook frame: "), Result);
			break;
		}
		}
	}
}

bool UTouchEngine::SetCookMode(bool IsIndependent)
{
	if (IsIndependent)
		MyTimeMode = TETimeMode::TETimeInternal;
	else
		MyTimeMode = TETimeMode::TETimeExternal;

	return true;
}

bool UTouchEngine::SetFrameRate(int64 FrameRate)
{
	if (!MyTEInstance)
	{
		MyFrameRate = FrameRate;
		return true;
	}

	return false;
}

void UTouchEngine::AddWarning(const FString& s)
{
#ifdef WITH_EDITOR
	FScopeLock lock(&MyMessageLock);
	MyWarnings.Add(s);
#endif
}

void UTouchEngine::AddResult(const FString& s, TEResult Result)
{
	FString s2(s);
	s2 += TEResultGetDescription(Result);

	if (TEResultGetSeverity(Result) == TESeverityError)
		AddError(s2);
	else if (TEResultGetSeverity(Result) == TESeverityWarning)
		AddWarning(s2);
}

void UTouchEngine::AddError(const FString& s)
{
#ifdef WITH_EDITOR
	FScopeLock lock(&MyMessageLock);
	MyErrors.Add(s);
#endif
}

void UTouchEngine::OutputMessages()
{
#ifdef WITH_EDITOR
	FScopeLock lock(&MyMessageLock);
	for (FString& m : MyErrors)
	{
		OutputError(m);
	}
	for (FString& m : MyWarnings)
	{
		OutputWarning(m);
	}
#endif
	MyErrors.Empty();
	MyWarnings.Empty();
}

void UTouchEngine::OutputResult(const FString& s, TEResult Result)
{
#ifdef WITH_EDITOR
	FString s2(s);
	s2 += TEResultGetDescription(Result);

	if (TEResultGetSeverity(Result) == TESeverityError)
		OutputError(s2);
	else if (TEResultGetSeverity(Result) == TESeverityWarning)
		OutputWarning(s2);
#endif
}

void UTouchEngine::OutputError(const FString& s)
{
#ifdef WITH_EDITOR
	MyMessageLog.Error(FText::Format(LOCTEXT("TEErrorString", "TouchEngine error - %s"), FText::FromString(s)));
	if (!MyLogOpened)
	{
		MyMessageLog.Open(EMessageSeverity::Error, false);
		MyLogOpened = true;
	}
	else
	{
		MyMessageLog.Notify(LOCTEXT("TEError", "TouchEngine Error"), EMessageSeverity::Error);
	}
#endif
}

void UTouchEngine::OutputWarning(const FString& s)
{
#ifdef WITH_EDITOR
	MyMessageLog.Warning(FText::Format(LOCTEXT("TEWarningString", "TouchEngine warning - "), FText::FromString(s)));
	if (!MyLogOpened)
	{
		MyMessageLog.Open(EMessageSeverity::Warning, false);
		MyLogOpened = true;
	}
	else
	{
		MyMessageLog.Notify(LOCTEXT("TEWarning", "TouchEngine Warning"), EMessageSeverity::Warning);
	}
#endif
}

static bool isTypeless(DXGI_FORMAT Format)
{
	switch (Format)
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

static DXGI_FORMAT toTypedDXGIFormat(ETextureRenderTargetFormat Format)
{
	switch (Format)
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

FTouchTOP UTouchEngine::GetTOPOutput(const FString& Identifier)
{
	FTouchTOP c;
	if (!MyDidLoad)
	{
		return c;
	}
	assert(MyTEInstance);
	if (!MyTEInstance)
		return c;

	std::string fullId("");
	fullId += TCHAR_TO_UTF8(*Identifier);
	TELinkInfo* param = nullptr;
	TEResult Result = TEInstanceLinkGetInfo(MyTEInstance, fullId.c_str(), &param);

	if (Result != TEResultSuccess)
	{
		OutputError(FString(TEXT("getTOPOutput(): Unable to find output named: ")) + Identifier);
		return c;
	}
	else
	{
		TERelease(&param);
	}

	FScopeLock lock(&MyTOPLock);

	if (!MyTOPOutputs.Contains(Identifier))
	{
		MyTOPOutputs.Add(Identifier);
	}


	if (FTouchTOP* top = MyTOPOutputs.Find(Identifier))
	{
		return *top;
	}
	else
	{
		OutputError(FString(TEXT("getTOPOutput(): Unable to find output named: ")) + Identifier);
		return c;
	}
}

void UTouchEngine::SetTOPInput(const FString& Identifier, UTexture* Texture)
{
	if (!MyDidLoad)
	{
		return;
	}

	if (!Texture || !Texture->Resource)
	{
		OutputError(TEXT("setTOPInput(): Null or empty texture provided"));
		return;
	}

	std::string fullId("");
	fullId += TCHAR_TO_UTF8(*Identifier);

	TELinkInfo* param = nullptr;
	TEResult Result = TEInstanceLinkGetInfo(MyTEInstance, fullId.c_str(), &param);

	if (Result != TEResultSuccess)
	{
		OutputResult(TEXT("setTOPInput(): Unable to get input Info for: ") + Identifier + ". ", Result);
		return;
	}
	TERelease(&param);

	ENQUEUE_RENDER_COMMAND(void)(
		[this, fullId, Texture](FRHICommandListImmediate& RHICmdList)
		{
			UTexture2D* tex2d = dynamic_cast<UTexture2D*>(Texture);
			UTextureRenderTarget2D* rt = nullptr;
			if (!tex2d)
			{
				rt = dynamic_cast<UTextureRenderTarget2D*>(Texture);
			}

			if (!rt && !tex2d)
				return;

			TETexture* teTexture = nullptr;
			ID3D11Texture2D* wrappedResource = nullptr;
			if (MyRHIType == RHIType::DirectX11)
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
						AddError(TEXT("setTOPInput(): Unable to create TouchEngine copy of texture."));
					}
				}
				else
				{
					AddError(TEXT("setTOPInput(): Unsupported pixel format for texture input. Compressed textures are not supported."));
				}
			}
			else if (MyRHIType == RHIType::DirectX12)
			{
			}

			if (teTexture)
			{
				TEResult res = TEInstanceLinkSetTextureValue(MyTEInstance, fullId.c_str(), teTexture, MyTEContext);
				TERelease(&teTexture);
			}

			if (wrappedResource)
			{
				wrappedResource->Release();
			}
		});
}

FTouchCHOPFull UTouchEngine::GetCHOPOutputSingleSample(const FString& Identifier)
{
	if (!MyDidLoad)
	{
		return FTouchCHOPFull();
	}

	FTouchCHOPFull f;

	std::string fullId("");
	fullId += TCHAR_TO_UTF8(*Identifier);

	TELinkInfo* param = nullptr;
	TEResult Result = TEInstanceLinkGetInfo(MyTEInstance, fullId.c_str(), &param);
	if (Result == TEResultSuccess && param->scope == TEScopeOutput)
	{
		switch (param->type)
		{
		case TELinkTypeFloatBuffer:
		{

			TEFloatBuffer* buf = nullptr;
			Result = TEInstanceLinkGetFloatBufferValue(MyTEInstance, fullId.c_str(), TELinkValueDefault, &buf);

			if (Result == TEResultSuccess && buf != nullptr)
			{
				if (!MyCHOPSingleOutputs.Contains(Identifier))
				{
					MyCHOPSingleOutputs.Add(Identifier);
				}

				FTouchCHOPSingleSample& output = MyCHOPSingleOutputs[Identifier];

				int32_t channelCount = TEFloatBufferGetChannelCount(buf);
				int64_t maxSamples = TEFloatBufferGetValueCount(buf);

				int64_t length = maxSamples;

				double rate = TEFloatBufferGetRate(buf);
				if (!TEFloatBufferIsTimeDependent(buf))
				{
					const float* const* channels = TEFloatBufferGetValues(buf);
					const char* const* channelNames = TEFloatBufferGetChannelNames(buf);

					if (Result == TEResultSuccess)
					{
						// Use the channel data here
						if (length > 0 && channelCount > 0)
						{
							for (int i = 0; i < channelCount; i++)
							{
								f.SampleData.Add(FTouchCHOPSingleSample());
								f.SampleData[i].ChannelName = channelNames[i];

								for (int j = 0; j < length; j++)
								{
									f.SampleData[i].ChannelData.Add(channels[i][j]);
								}
							}
						}
					}
					// Suppress internal errors for now, some superfluous ones are occuring currently
					else if (Result != TEResultInternalError)
					{
						OutputResult(TEXT("getCHOPOutputSingleSample(): "), Result);
					}
					//c = output;
					TERelease(&buf);
				}
				else
				{
					//length /= rate / MyFrameRate;

					const float* const* channels = TEFloatBufferGetValues(buf);
					const char* const* channelNames = TEFloatBufferGetChannelNames(buf);

					if (Result == TEResultSuccess)
					{
						// Use the channel data here
						if (length > 0 && channelCount > 0)
						{
							output.ChannelData.SetNum(channelCount);

							for (int i = 0; i < channelCount; i++)
							{
								output.ChannelData[i] = channels[i][length - 1];
							}
							output.ChannelName = channelNames[0];
						}
					}
					// Suppress internal errors for now, some superfluous ones are occuring currently
					else if (Result != TEResultInternalError)
					{
						OutputResult(TEXT("getCHOPOutputSingleSample(): "), Result);
					}
					f.SampleData.Add(output);
					TERelease(&buf);

				}
			}
			break;
		}
		default:
		{
			OutputError(TEXT("getCHOPOutputSingleSample(): ") + Identifier + TEXT(" is not a CHOP output."));
			break;
		}
		}
	}
	else if (Result != TEResultSuccess)
	{
		OutputResult(TEXT("getCHOPOutputSingleSample(): "), Result);
	}
	else if (param->scope == TEScopeOutput)
	{
		OutputError(TEXT("getCHOPOutputSingleSample(): ") + Identifier + TEXT(" is not a CHOP output."));
	}
	TERelease(&param);

	return f;
}

void UTouchEngine::SetCHOPInputSingleSample(const FString& Identifier, const FTouchCHOPSingleSample& CHOP)
{
	if (!MyTEInstance)
		return;

	if (!MyDidLoad)
	{
		return;
	}

	if (!CHOP.ChannelData.Num())
		return;

	std::string fullId("");
	fullId += TCHAR_TO_UTF8(*Identifier);


	TEResult Result;
	TELinkInfo* Info;
	Result = TEInstanceLinkGetInfo(MyTEInstance, fullId.c_str(), &Info);

	if (Result != TEResultSuccess)
	{
		OutputResult(FString("setCHOPInputSingleSample(): Unable to get input Info, ") + FString(Identifier) + " may not exist. ", Result);
		return;
	}

	if (Info->type != TELinkTypeFloatBuffer)
	{
		OutputError(FString("setCHOPInputSingleSample(): Input named: ") + FString(Identifier) + " is not a CHOP input.");
		TERelease(&Info);
		return;
	}

	std::vector<float>	realData;
	std::vector<const float*>	dataPtrs;
	std::vector<std::string> names;
	std::vector<const char*> namesPtrs;
	for (int i = 0; i < CHOP.ChannelData.Num(); i++)
	{
		realData.push_back(CHOP.ChannelData[i]);
		std::string n("chan");
		n += '1' + i;
		names.push_back(std::move(n));
	}
	// Seperate loop since realData can reallocate a few times
	for (int i = 0; i < CHOP.ChannelData.Num(); i++)
	{
		dataPtrs.push_back(&realData[i]);
		namesPtrs.push_back(names[i].c_str());
	}

	TEFloatBuffer* buf = TEFloatBufferCreate(-1.f, CHOP.ChannelData.Num(), 1, namesPtrs.data());

	Result = TEFloatBufferSetValues(buf, dataPtrs.data(), 1);

	if (Result != TEResultSuccess)
	{
		OutputResult(FString("setCHOPInputSingleSample(): Failed to set buffer values: "), Result);
		TERelease(&Info);
		TERelease(&buf);
		return;
	}
	Result = TEInstanceLinkAddFloatBuffer(MyTEInstance, fullId.c_str(), buf);

	if (Result != TEResultSuccess)
	{
		OutputResult(FString("setCHOPInputSingleSample(): Unable to append buffer values: "), Result);
		TERelease(&Info);
		TERelease(&buf);
		return;
	}

	TERelease(&Info);
	TERelease(&buf);
}

FTouchCHOPFull UTouchEngine::GetCHOPOutputs(const FString& Identifier)
{
	if (!MyDidLoad)
	{
		return FTouchCHOPFull();
	}

	FTouchCHOPFull c;

	std::string fullId("");
	fullId += TCHAR_TO_UTF8(*Identifier);

	TELinkInfo* param = nullptr;
	TEResult Result = TEInstanceLinkGetInfo(MyTEInstance, fullId.c_str(), &param);
	if (Result == TEResultSuccess && param->scope == TEScopeOutput)
	{
		switch (param->type)
		{
		case TELinkTypeFloatBuffer:
		{

			TEFloatBuffer* buf = nullptr;
			Result = TEInstanceLinkGetFloatBufferValue(MyTEInstance, fullId.c_str(), TELinkValueDefault, &buf);

			if (Result == TEResultSuccess)
			{
				if (!MyCHOPFullOutputs.Contains(Identifier))
				{
					MyCHOPFullOutputs.Add(Identifier);
				}

				FTouchCHOPFull& output = MyCHOPFullOutputs[Identifier];

				int32_t channelCount = TEFloatBufferGetChannelCount(buf);
				int64_t maxSamples = TEFloatBufferGetValueCount(buf);
				const float* const* channels = TEFloatBufferGetValues(buf);
				const char* const* channelNames = TEFloatBufferGetChannelNames(buf);

				if (TEFloatBufferIsTimeDependent(buf))
				{

				}

				if (Result == TEResultSuccess)
				{
					// Use the channel data here
					if (maxSamples > 0 && channelCount > 0)
					{
						output.SampleData.SetNum(channelCount);
						for (int i = 0; i < channelCount; i++)
						{
							output.SampleData[i].ChannelData.SetNum(maxSamples);
							output.SampleData[i].ChannelName = channelNames[i];
							for (int j = 0; j < maxSamples; j++)
							{
								output.SampleData[i].ChannelData[j] = channels[i][j];
							}
						}
					}
				}
				// Suppress internal errors for now, some superfluous ones are occuring currently
				else if (Result != TEResultInternalError)
				{
					OutputResult(TEXT("getCHOPOutputs(): "), Result);
				}
				c = output;
				TERelease(&buf);
			}
			break;
		}
		default:
		{
			OutputError(TEXT("getCHOPOutputs(): ") + Identifier + TEXT(" is not a CHOP output."));
			break;
		}
		}
	}
	else if (Result != TEResultSuccess)
	{
		OutputResult(TEXT("getCHOPOutputs(): "), Result);
	}
	else if (param->scope == TEScopeOutput)
	{
		OutputError(TEXT("getCHOPOutputs(): ") + Identifier + TEXT(" is not a CHOP output."));
	}
	TERelease(&param);

	return c;
}

void UTouchEngine::SetCHOPInput(const FString& Identifier, const FTouchCHOPFull& CHOP)
{
}

FTouchVar<bool> UTouchEngine::GetBooleanOutput(const FString& Identifier)
{
	if (!MyDidLoad)
	{
		return FTouchVar<bool>();
	}

	FTouchVar<bool> c = FTouchVar<bool>();

	std::string fullId("");
	fullId += TCHAR_TO_UTF8(*Identifier);

	TELinkInfo* param = nullptr;
	TEResult Result = TEInstanceLinkGetInfo(MyTEInstance, fullId.c_str(), &param);
	if (Result == TEResultSuccess && param->scope == TEScopeOutput)
	{
		switch (param->type)
		{
		case TELinkTypeBoolean:
		{
			Result = TEInstanceLinkGetBooleanValue(MyTEInstance, fullId.c_str(), TELinkValueCurrent, &c.Data);

			if (Result == TEResultSuccess)
			{
				if (!MyCHOPSingleOutputs.Contains(Identifier))
				{
					MyCHOPSingleOutputs.Add(Identifier);
				}
			}
			break;
		}
		default:
		{
			OutputError(TEXT("getBooleanOutput(): ") + Identifier + TEXT(" is not a boolean output."));
			break;
		}
		}
	}
	else if (Result != TEResultSuccess)
	{
		OutputResult(TEXT("getBooleanOutput(): "), Result);
	}
	else if (param->scope == TEScopeOutput)
	{
		OutputError(TEXT("getBooleanOutput(): ") + Identifier + TEXT(" is not a boolean output."));
	}
	TERelease(&param);

	return c;
}

void UTouchEngine::SetBooleanInput(const FString& Identifier, FTouchVar<bool>& Op)
{
	if (!MyTEInstance)
		return;

	if (!MyDidLoad)
	{
		return;
	}

	std::string fullId("");
	fullId += TCHAR_TO_UTF8(*Identifier);

	TEResult Result;
	TELinkInfo* Info;
	Result = TEInstanceLinkGetInfo(MyTEInstance, fullId.c_str(), &Info);

	if (Result != TEResultSuccess)
	{
		OutputResult(FString("setBooleanInput(): Unable to get input Info, ") + FString(Identifier) + " may not exist. ", Result);
		return;
	}

	if (Info->type != TELinkTypeBoolean)
	{
		OutputError(FString("setBooleanInput(): Input named: ") + FString(Identifier) + " is not a boolean input.");
		TERelease(&Info);
		return;
	}

	Result = TEInstanceLinkSetBooleanValue(MyTEInstance, fullId.c_str(), Op.Data);

	if (Result != TEResultSuccess)
	{
		OutputResult(FString("setBooleanInput(): Unable to set boolean value: "), Result);
		TERelease(&Info);
		return;
	}

	TERelease(&Info);
}

FTouchVar<double> UTouchEngine::GetDoubleOutput(const FString& Identifier)
{
	if (!MyDidLoad)
	{
		return FTouchVar<double>();
	}

	FTouchVar<double> c = FTouchVar<double>();

	std::string fullId("");
	fullId += TCHAR_TO_UTF8(*Identifier);

	TELinkInfo* param = nullptr;
	TEResult Result = TEInstanceLinkGetInfo(MyTEInstance, fullId.c_str(), &param);
	if (Result == TEResultSuccess && param->scope == TEScopeOutput)
	{
		switch (param->type)
		{
		case TELinkTypeDouble:
		{
			Result = TEInstanceLinkGetDoubleValue(MyTEInstance, fullId.c_str(), TELinkValueCurrent, &c.Data, 1);

			if (Result == TEResultSuccess)
			{
				if (!MyCHOPSingleOutputs.Contains(Identifier))
				{
					MyCHOPSingleOutputs.Add(Identifier);
				}
			}
			break;
		}
		default:
		{
			OutputError(TEXT("getDoubleOutput(): ") + Identifier + TEXT(" is not a double output."));
			break;
		}
		}
	}
	else if (Result != TEResultSuccess)
	{
		OutputResult(TEXT("getDoubleOutput(): "), Result);
	}
	else if (param->scope == TEScopeOutput)
	{
		OutputError(TEXT("getDoubleOutput(): ") + Identifier + TEXT(" is not a double output."));
	}
	TERelease(&param);

	return c;
}

void UTouchEngine::SetDoubleInput(const FString& Identifier, FTouchVar<TArray<double>>& Op)
{
	if (!MyTEInstance)
		return;

	if (!MyDidLoad)
	{
		return;
	}

	std::string fullId("");
	fullId += TCHAR_TO_UTF8(*Identifier);

	TEResult Result;
	TELinkInfo* Info;
	Result = TEInstanceLinkGetInfo(MyTEInstance, fullId.c_str(), &Info);

	if (Result != TEResultSuccess)
	{
		OutputResult(FString("setDoubleInput(): Unable to get input Info, ") + FString(Identifier) + " may not exist. ", Result);
		return;
	}

	if (Info->type != TELinkTypeDouble)
	{
		OutputError(FString("setDoubleInput(): Input named: ") + FString(Identifier) + " is not a double input.");
		TERelease(&Info);
		return;
	}

	if (Op.Data.Num() != Info->count)
	{
		if (Op.Data.Num() > Info->count)
		{
			TArray<double> buffer;

			for (int i = 0; i < Info->count; i++)
			{
				buffer.Add(Op.Data[i]);
			}

			Result = TEInstanceLinkSetDoubleValue(MyTEInstance, fullId.c_str(), buffer.GetData(), Info->count);
		}
		else
		{
			OutputError(FString("setDoubleInput(): Unable to set double value: count mismatch"));
			TERelease(&Info);
			return;
		}
	}
	else
	{
		Result = TEInstanceLinkSetDoubleValue(MyTEInstance, fullId.c_str(), Op.Data.GetData(), Op.Data.Num());
	}

	if (Result != TEResultSuccess)
	{
		OutputResult(FString("setDoubleInput(): Unable to set double value: "), Result);
		TERelease(&Info);
		return;
	}

	TERelease(&Info);
}

FTouchVar<int32_t> UTouchEngine::GetIntegerOutput(const FString& Identifier)
{
	if (!MyDidLoad)
	{
		return FTouchVar<int32_t>();
	}

	FTouchVar<int32_t> c = FTouchVar<int32_t>();

	std::string fullId("");
	fullId += TCHAR_TO_UTF8(*Identifier);

	TELinkInfo* param = nullptr;
	TEResult Result = TEInstanceLinkGetInfo(MyTEInstance, fullId.c_str(), &param);
	if (Result == TEResultSuccess && param->scope == TEScopeOutput)
	{
		switch (param->type)
		{
		case TELinkTypeBoolean:
		{
			Result = TEInstanceLinkGetIntValue(MyTEInstance, fullId.c_str(), TELinkValueCurrent, &c.Data, 1);

			if (Result == TEResultSuccess)
			{
				if (!MyCHOPSingleOutputs.Contains(Identifier))
				{
					MyCHOPSingleOutputs.Add(Identifier);
				}
			}
			break;
		}
		default:
		{
			OutputError(TEXT("getIntegerOutput(): ") + Identifier + TEXT(" is not an integer output."));
			break;
		}
		}
	}
	else if (Result != TEResultSuccess)
	{
		OutputResult(TEXT("getIntegerOutput(): "), Result);
	}
	else if (param->scope == TEScopeOutput)
	{
		OutputError(TEXT("getIntegerOutput(): ") + Identifier + TEXT(" is not an integer output."));
	}
	TERelease(&param);

	return c;
}

void UTouchEngine::SetIntegerInput(const FString& Identifier, FTouchVar<TArray<int32_t>>& Op)
{
	if (!MyTEInstance)
		return;

	if (!MyDidLoad)
	{
		return;
	}

	std::string fullId("");
	fullId += TCHAR_TO_UTF8(*Identifier);

	TEResult Result;
	TELinkInfo* Info;
	Result = TEInstanceLinkGetInfo(MyTEInstance, fullId.c_str(), &Info);

	if (Result != TEResultSuccess)
	{
		OutputResult(FString("setIntegerInput(): Unable to get input Info, ") + FString(Identifier) + " may not exist. ", Result);
		return;
	}

	if (Info->type != TELinkTypeInt)
	{
		OutputError(FString("setIntegerInput(): Input named: ") + FString(Identifier) + " is not an integer input.");
		TERelease(&Info);
		return;
	}

	Result = TEInstanceLinkSetIntValue(MyTEInstance, fullId.c_str(), Op.Data.GetData(), Op.Data.Num());

	if (Result != TEResultSuccess)
	{
		OutputResult(FString("setIntegerInput(): Unable to set integer value: "), Result);
		TERelease(&Info);
		return;
	}

	TERelease(&Info);
}

FTouchVar<TEString*> UTouchEngine::GetStringOutput(const FString& Identifier)
{
	if (!MyDidLoad)
	{
		return FTouchVar<TEString*>();
	}

	FTouchVar<TEString*> c = FTouchVar<TEString*>();

	std::string fullId("");
	fullId += TCHAR_TO_UTF8(*Identifier);

	TELinkInfo* param = nullptr;
	TEResult Result = TEInstanceLinkGetInfo(MyTEInstance, fullId.c_str(), &param);
	if (Result == TEResultSuccess && param->scope == TEScopeOutput)
	{
		switch (param->type)
		{
		case TELinkTypeString:
		{
			Result = TEInstanceLinkGetStringValue(MyTEInstance, fullId.c_str(), TELinkValueCurrent, &c.Data);

			if (Result == TEResultSuccess)
			{
				if (!MyCHOPSingleOutputs.Contains(Identifier))
				{
					MyCHOPSingleOutputs.Add(Identifier);
				}
			}
			break;
		}
		default:
		{
			OutputError(TEXT("getStringOutput(): ") + Identifier + TEXT(" is not a string output."));
			break;
		}
		}
	}
	else if (Result != TEResultSuccess)
	{
		OutputResult(TEXT("getStringOutput(): "), Result);
	}
	else if (param->scope == TEScopeOutput)
	{
		OutputError(TEXT("getStringOutput(): ") + Identifier + TEXT(" is not a string output."));
	}
	TERelease(&param);

	return c;
}

void UTouchEngine::SetStringInput(const FString& Identifier, FTouchVar<char*>& Op)
{
	if (!MyTEInstance)
		return;

	if (!MyDidLoad)
	{
		return;
	}

	std::string fullId("");
	fullId += TCHAR_TO_UTF8(*Identifier);

	TEResult Result;
	TELinkInfo* Info;
	Result = TEInstanceLinkGetInfo(MyTEInstance, fullId.c_str(), &Info);

	if (Result != TEResultSuccess)
	{
		OutputResult(FString("setStringInput(): Unable to get input Info, ") + FString(Identifier) + " may not exist. ", Result);
		return;
	}

	if (Info->type == TELinkTypeString)
	{
		Result = TEInstanceLinkSetStringValue(MyTEInstance, fullId.c_str(), Op.Data);
	}
	else if (Info->type == TELinkTypeStringData)
	{
		TETable* table = TETableCreate();
		TETableResize(table, 1, 1);
		TETableSetStringValue(table, 0, 0, Op.Data);

		Result = TEInstanceLinkSetTableValue(MyTEInstance, fullId.c_str(), table);
		TERelease(&table);
	}
	else
	{
		OutputError(FString("setStringInput(): Input named: ") + FString(Identifier) + " is not a string input.");
		TERelease(&Info);
		return;
	}


	if (Result != TEResultSuccess)
	{
		OutputResult(FString("setStringInput(): Unable to set string value: "), Result);
		TERelease(&Info);
		return;
	}

	TERelease(&Info);
}

FTouchDATFull UTouchEngine::GetTableOutput(const FString& Identifier)
{
	if (!MyDidLoad)
		return FTouchDATFull();

	FTouchDATFull c = FTouchDATFull();

	std::string fullId("");
	fullId += TCHAR_TO_UTF8(*Identifier);

	TELinkInfo* param = nullptr;
	TEResult Result = TEInstanceLinkGetInfo(MyTEInstance, fullId.c_str(), &param);
	if (Result == TEResultSuccess && param->scope == TEScopeOutput)
	{
		switch (param->type)
		{
		case TELinkTypeStringData:
		{
			Result = TEInstanceLinkGetTableValue(MyTEInstance, fullId.c_str(), TELinkValue::TELinkValueCurrent, &c.ChannelData);
			break;
		}
		default:
		{
			OutputError(TEXT("getTableOutput(): ") + Identifier + TEXT(" is not a table output."));
			break;
		}
		}
	}
	else if (Result != TEResultSuccess)
	{
		OutputResult(TEXT("getTableOutput(): "), Result);
	}
	else if (param->scope == TEScopeOutput)
	{
		OutputError(TEXT("getTableOutput(): ") + Identifier + TEXT(" is not a table output."));
	}
	TERelease(&param);

	return c;
}

void UTouchEngine::SetTableInput(const FString& Identifier, FTouchDATFull& Op)
{
	if (!MyTEInstance)
		return;

	if (!MyDidLoad)
	{
		return;
	}

	std::string fullId("");
	fullId += TCHAR_TO_UTF8(*Identifier);

	TEResult Result;
	TELinkInfo* Info;
	Result = TEInstanceLinkGetInfo(MyTEInstance, fullId.c_str(), &Info);

	if (Result != TEResultSuccess)
	{
		OutputResult(FString("setTableInput(): Unable to get input Info, ") + FString(Identifier) + " may not exist. ", Result);
		return;
	}

	if (Info->type == TELinkTypeString)
	{
		const char* string = TETableGetStringValue(Op.ChannelData, 0, 0);
		Result = TEInstanceLinkSetStringValue(MyTEInstance, fullId.c_str(), string);
	}
	else if (Info->type == TELinkTypeStringData)
	{
		Result = TEInstanceLinkSetTableValue(MyTEInstance, fullId.c_str(), Op.ChannelData);
	}
	else
	{
		OutputError(FString("setTableInput(): Input named: ") + FString(Identifier) + " is not a table input.");
		TERelease(&Info);
		return;
	}


	if (Result != TEResultSuccess)
	{
		OutputResult(FString("setTableInput(): Unable to set table value: "), Result);
		TERelease(&Info);
		return;
	}

	TERelease(&Info);
}

#undef LOCTEXT_NAMESPACE