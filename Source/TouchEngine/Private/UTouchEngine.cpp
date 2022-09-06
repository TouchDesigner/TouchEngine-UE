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

/* Unfortunately, this section can not be alphabetically ordered.
 * D3D11Resources is not IWYU compliant and we must include the other files
 * before it to fix its incomplete type dependencies. */
#include "D3D11RHI.h"
#include "ShaderCore.h"
#include "D3D11State.h"
#include "D3D11Resources.h"

#include "TouchEngineDynamicVariableStruct.h"
#include "TouchEngineParserUtils.h"
#include "Async/Async.h"
#include "Engine/Texture2D.h"
#include "Engine/TextureRenderTarget2D.h"

#include <vector>
#include <mutex>

#define LOCTEXT_NAMESPACE "TouchEngine"

UTouchEngine::~UTouchEngine()
{
	Clear();
}

void UTouchEngine::BeginDestroy()
{
	Clear();

	Super::BeginDestroy();
}

void UTouchEngine::Clear()
{
	if (MyImmediateContext == nullptr)
	{
		return;
	}

	MyCHOPSingleOutputs.Empty();

	ENQUEUE_RENDER_COMMAND(TouchEngine_Clear_CleanupTextures)(
	[this](FRHICommandListImmediate& RHICmdList)
	{
		if (!IsValid(this))
		{
			return;
		}

		FScopeLock Lock(&MyTOPLock);

		CleanupTextures(MyImmediateContext, &MyTexCleanups, EFinalClean::True);
		if (MyImmediateContext)
			MyImmediateContext->Release();
		//MyTEContext.reset();
		if (MyTexCleanups.size())
			MyTexCleanups.clear();
		MyImmediateContext = nullptr;
		//	MyTEInstance.reset();
		MyDevice = nullptr;
		MyDidLoad = false;
		MyFailedLoad = false;
		MyToxPath = "";
		MyConfiguredWithTox = false;
		MyLoadCalled = false;
	});
}

const FString& UTouchEngine::GetToxPath() const
{
	return MyToxPath;
}

void UTouchEngine::EventCallback(TEInstance* Instance, TEEvent Event, TEResult Result, int64_t StartTimeValue, int32_t StartTimeScale, int64_t EndTimeValue, int32_t EndTimeScale, void* Info)
{
	UTouchEngine* Engine = static_cast<UTouchEngine*>(Info);
	if (!Engine)
		return;

	switch (Event)
	{
	case TEEventInstanceDidLoad:
	{
		if (Result == TEResultSuccess)
		{
			Engine->SetDidLoad();

			// Broadcast Links loaded Event
			TArray<FTouchEngineDynamicVariableStruct> VariablesIn, VariablesOut;

			for (TEScope Scope : { TEScopeInput, TEScopeOutput })
			{
				TEStringArray* Groups;
				Result = TEInstanceGetLinkGroups(Instance, Scope, &Groups);

				if (Result == TEResultSuccess)
				{
					for (int32_t i = 0; i < Groups->count; i++)
					{
						switch (Scope)
						{
						case TEScopeInput:
						{
							FTouchEngineParserUtils::ParseGroup(Instance, Groups->strings[i], VariablesIn);
							break;
						}
						case TEScopeOutput:
						{
							FTouchEngineParserUtils::ParseGroup(Instance, Groups->strings[i], VariablesOut);
							break;
						}
						}
					}
				}

			}

			UTouchEngine* SavedEngine = Engine;
			AsyncTask(ENamedThreads::GameThread, [SavedEngine, VariablesIn, VariablesOut]()
				{
					SavedEngine->OnParametersLoaded.Broadcast(VariablesIn, VariablesOut);
				}
			);
		}
		else if (Result == TEResultFileError)
		{
			UTouchEngine* SavedEngine = Engine;
			TEResult SavedResult = Result;
			AsyncTask(ENamedThreads::GameThread, [SavedEngine, SavedResult]()
				{
					SavedEngine->OutputError(FString("load() failed to load .tox \"") + SavedEngine->MyToxPath + "\" " + TEResultGetDescription(SavedResult));
					SavedEngine->MyFailedLoad = true;
					SavedEngine->OnLoadFailed.Broadcast(TEResultGetDescription(SavedResult));
				}
			);
		}
		else if (Result == TEResultIncompatibleEngineVersion)
		{
			UTouchEngine* SavedEngine = Engine;
			TEResult savedResult = Result;
			AsyncTask(ENamedThreads::GameThread, [SavedEngine, savedResult]()
				{
					SavedEngine->OutputError(TEResultGetDescription(savedResult));
					SavedEngine->MyFailedLoad = true;
					SavedEngine->OnLoadFailed.Broadcast(TEResultGetDescription(savedResult));
				}
			);
		}
		else
		{

			TESeverity Severity = TEResultGetSeverity(Result);

			if (Severity == TESeverity::TESeverityError)
			{
				UTouchEngine* SavedEngine = Engine;
				TEResult savedResult = Result;
				AsyncTask(ENamedThreads::GameThread, [SavedEngine, savedResult]()
					{
						SavedEngine->OutputError(FString("load(): tox file severe error: ") + TEResultGetDescription(savedResult));
						SavedEngine->MyFailedLoad = true;
						SavedEngine->OnLoadFailed.Broadcast(TEResultGetDescription(savedResult));
					}
				);
			}
			else
			{
				Engine->SetDidLoad();

				// Broadcast Links loaded Event
				TArray<FTouchEngineDynamicVariableStruct> VariablesIn, VariablesOut;

				for (TEScope Scope : { TEScopeInput, TEScopeOutput })
				{
					TEStringArray* Groups;
					Result = TEInstanceGetLinkGroups(Instance, Scope, &Groups);

					if (Result == TEResultSuccess)
					{
						for (int32_t i = 0; i < Groups->count; i++)
						{
							switch (Scope)
							{
							case TEScopeInput:
							{
								FTouchEngineParserUtils::ParseGroup(Instance, Groups->strings[i], VariablesIn);
								break;
							}
							case TEScopeOutput:
							{
								FTouchEngineParserUtils::ParseGroup(Instance, Groups->strings[i], VariablesOut);
								break;
							}
							}
						}
					}

				}

				UTouchEngine* SavedEngine = Engine;
				AsyncTask(ENamedThreads::GameThread, [SavedEngine, VariablesIn, VariablesOut]()
					{
						SavedEngine->OnParametersLoaded.Broadcast(VariablesIn, VariablesOut);
					}
				);
			}
		}
		break;
	}
	case TEEventFrameDidFinish:
	{
		Engine->MyCooking = false;
		Engine->OnCookFinished.Broadcast();
		break;
	}
	case TEEventGeneral:
	default:
		break;
	}
}

void UTouchEngine::LinkValueCallback(TEInstance* Instance, TELinkEvent Event, const char* Identifier, void* Info)
{
	UTouchEngine* Doc = static_cast<UTouchEngine*>(Info);
	Doc->LinkValueCallback(Instance, Event, Identifier);
}

void UTouchEngine::CleanupTextures(ID3D11DeviceContext* Context, std::deque<TexCleanup>* Cleanups, EFinalClean FC)
{
	if (Cleanups == nullptr)
		return;

	checkf(IsInRenderingThread(),
		TEXT("CleanupTextures must run on the rendering thread, otherwise"
			" the chance for data corruption due to the RenderThread and another"
			" one accessing the same textures is high."))

	while (!Cleanups->empty())
	{
		TexCleanup& Cleanup = Cleanups->front();

		if (!Cleanup.Query || !Cleanup.Texture)
		{
			Cleanups->pop_front();
			continue;
		}

		BOOL Result = false;

		Context->GetData(Cleanup.Query, &Result, sizeof(Result), 0);
		if (Result)
		{
			TERelease(&Cleanup.Texture);
			Cleanup.Query->Release();
			Cleanups->pop_front();
		}
		else
		{
			if (FC == EFinalClean::True)
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

	TELinkInfo* Param = nullptr;
	TEResult Result = TEInstanceLinkGetInfo(Instance, Identifier, &Param);

	if (Result == TEResultSuccess && Param && Param->scope == TEScopeOutput)
	{
		switch (Event)
		{
		case TELinkEventAdded:
		{
			// single Link added
			break;
		}
		case TELinkEventRemoved:
		{
			return;
		}
		case TELinkEventValueChange:
		{
			// current value of the callback
			if (!MyTEInstance)
				return;
			switch (Param->type)
			{
			case TELinkTypeTexture:
			{
				// Stash the state, we don't do any actual renderer work from this thread
				TEDXGITexture* DXGITexture = nullptr;
				Result = TEInstanceLinkGetTextureValue(MyTEInstance, Identifier, TELinkValueCurrent, &DXGITexture);

				if (Result != TEResultSuccess)
				{
					// possible crash without this check
					return;
				}

				MyNumOutputTexturesQueued++;

				FString Name(Identifier);
				ENQUEUE_RENDER_COMMAND(TouchEngine_LinkValueCallback_CleanupTextures)(
					[this, Name, DXGITexture](FRHICommandListImmediate& RHICmdList)
					{
						CleanupTextures(MyImmediateContext, &MyTexCleanups, EFinalClean::False);

						TED3D11Texture* TED3DTexture = nullptr;

						FScopeLock Lock(&MyTOPLock);

						TED3D11ContextCreateTexture(MyTEContext, DXGITexture, &TED3DTexture);

						if (!TED3DTexture)
						{
							TERelease(&DXGITexture);
							MyNumOutputTexturesQueued--;
							return;
						}

						ID3D11Texture2D* d3dSrcTexture = TED3D11TextureGetTexture(TED3DTexture);

						if (!d3dSrcTexture)
						{
							TERelease(&TED3DTexture);
							TERelease(&DXGITexture);
							MyNumOutputTexturesQueued--;
							return;
						}

						D3D11_TEXTURE2D_DESC Desc = { 0 };
						d3dSrcTexture->GetDesc(&Desc);


						if (!MyTOPOutputs.Contains(Name))
						{
							MyTOPOutputs.Add(Name);
						}

						EPixelFormat pixelFormat = toEPixelFormat(Desc.Format);

						UE_LOG(LogTemp, Warning, TEXT("descpixelformat = '%i'"), (int32)Desc.Format)
						UE_LOG(LogTemp, Warning, TEXT("pixelformat = '%i'"), (int32)pixelFormat)

						if (pixelFormat == PF_Unknown)
						{
							AddError(TEXT("Texture with unsupported pixel format being generated by TouchEngine."));
							MyNumOutputTexturesQueued--;
							return;
						}

						// We need this code to be reusable and the DestTexture property must be set later.
						// Right now we don't know if it will be called on this render thread call or on
						// a future one from another thread
						const auto CopyResource = [this, TED3DTexture, DXGITexture, d3dSrcTexture, &RHICmdList]
						(UTexture* DestTexture)
						{
							if (!DestTexture->GetResource())
							{
								TERelease(&TED3DTexture);
								TERelease(&DXGITexture);
								MyNumOutputTexturesQueued--;
								return;
							}

							ID3D11Resource* destResource = nullptr;
							if (MyRHIType == RHIType::DirectX11)
							{
								FD3D11TextureBase* D3D11Texture = GetD3D11TextureFromRHITexture(
									DestTexture->GetResource()->TextureRHI);
								destResource = D3D11Texture->GetResource();
							}
							// else if (MyRHIType == RHIType::DirectX12)
							// { }

							if (destResource)
							{
								if (!MyImmediateContext)
								{
									MyNumOutputTexturesQueued--;
									return;
								}

								MyImmediateContext->CopyResource(destResource, d3dSrcTexture);

								// if (MyRHIType == RHIType::DirectX12)
								// { }

								// TODO: We get a crash if we release the TED3DTexture here,
								// so we defer it until D3D tells us it's done with it.
								// Ideally we should be able to release it here and let the ref
								// counting cause it to be cleaned up on it's own later on
								TexCleanup Cleanup;
								D3D11_QUERY_DESC QueryDesc = {};
								QueryDesc.Query = D3D11_QUERY_EVENT;
								HRESULT hresult = MyDevice->CreateQuery(&QueryDesc, &Cleanup.Query);
								if (hresult == 0)
								{
									MyImmediateContext->End(Cleanup.Query);
									if (TED3DTexture)
									{
										Cleanup.Texture = TED3DTexture;

										MyTexCleanups.push_back(Cleanup);
									}
								}
							}
							else
							{
								TERelease(&TED3DTexture);
							}
							TERelease(&DXGITexture);
							MyNumOutputTexturesQueued--;
						}; // FCopyResource

						// Do we need to resize the output texture?
						const FTouchTOP& Output = MyTOPOutputs[Name];
						if (!Output.Texture ||
							Output.Texture->GetSizeX() != Desc.Width ||
							Output.Texture->GetSizeY() != Desc.Height ||
							Output.Texture->GetPixelFormat() != pixelFormat)
						{
							// Recreate the texture on the GameThread then copy the output to it on the render thread
							AsyncTask(ENamedThreads::AnyThread,
								[this, Name, Desc, pixelFormat, TED3DTexture, DXGITexture, d3dSrcTexture, CopyResource]
								{
									// Scope tag for the Engine to know this is not a render thread.
									// Important for the Texture->UpdateResource() call below.
									FTaskTagScope TaskTagScope(ETaskTag::EParallelGameThread);

									FTouchTOP& Output = MyTOPOutputs[Name];
									if (Output.WrappedResource)
									{
										Output.WrappedResource->Release();
										Output.WrappedResource = nullptr;
									}
									Output.Texture = nullptr;
									Output.Texture = UTexture2D::CreateTransient(Desc.Width, Desc.Height, pixelFormat);
									Output.Texture->UpdateResource();

									UTexture* DestTexture = Output.Texture;

									// Copy the TouchEngine texture to the destination texture on the render thread
									ENQUEUE_RENDER_COMMAND(TouchEngine_CopyResource)(
										[this, DestTexture, TED3DTexture, DXGITexture, d3dSrcTexture, CopyResource]
										(FRHICommandList& RHICommandList)
										{
											CopyResource(DestTexture);
										});
								});
							// Stop running commands on the render thread for now. We need to wait for the
							// texture to be resized on another thread.
							return;
						}

						// If we don't need to resize the texture, copy the resource right now on the render thread
						CopyResource(Output.Texture);
					}
				);
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

	TERelease(&Param);
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

bool UTouchEngine::InstantiateEngineWithToxFile(const FString& ToxPath, const char* Caller)
{
	const auto BroadcastLoadError = [this, &Caller](const FString& ErrMessage)
	{
		const FString FullMessage = FString::Printf(TEXT("%S: %s"), Caller, *ErrMessage);
		OutputError(FullMessage);
		MyFailedLoad = true;
		OnLoadFailed.Broadcast(FullMessage);
	};

	if (!ToxPath.IsEmpty() && !ToxPath.EndsWith(".tox"))
	{
		BroadcastLoadError(FString::Printf(TEXT("Invalid file path - %s"), *ToxPath));
		return false;
	}
	const bool bLoadTox = !ToxPath.IsEmpty();

	// Checks for result, outputs if not Success and returns true when the severity level is not Error
	const auto OutputResultAndCheckForError = [this, &Caller](const TEResult Result, const FString& ErrMessage)
	{
		if (Result != TEResultSuccess)
		{
			const FString FullMessage = FString::Printf(TEXT("%S: %s"), Caller, *ErrMessage);
			OutputResult(FString::Printf(TEXT("%s: "), *FullMessage), Result);

			if (TEResultGetSeverity(Result) == TESeverity::TESeverityError)
			{
				MyFailedLoad = true;
				OnLoadFailed.Broadcast(ErrMessage);
				return false;
			}
		}
		return true;
	};

	if (!MyTEInstance)
	{
		const FString RHIType = FApp::GetGraphicsRHI();
		if (RHIType == "DirectX 11")
		{
			MyRHIType = RHIType::DirectX11;
			MyDevice = (ID3D11Device*)GDynamicRHI->RHIGetNativeDevice();
			if (!MyDevice)
			{
				BroadcastLoadError(TEXT("Unable to obtain DX11 Device."));
				return false;
			}

			MyDevice->GetImmediateContext(&MyImmediateContext);
			if (!MyImmediateContext)
			{
				BroadcastLoadError(TEXT("Unable to obtain DX11 Context."));
				return false;
			}
		}
		else
		{
			BroadcastLoadError(FString::Printf(TEXT("Unsupported RHI active: %s"), *RHIType));
			return false;
		}

		if (!OutputResultAndCheckForError(
			TED3D11ContextCreate(MyDevice, MyTEContext.take()),
			TEXT("Unable to create TouchEngine Context")))
		{
			return false;
		}

		if (!OutputResultAndCheckForError(
			TEInstanceCreate(
				EventCallback,
				LinkValueCallback,
				this,
				MyTEInstance.take()),
			TEXT("Unable to create TouchEngine Instance")))
		{
			return false;
		}

		if (!OutputResultAndCheckForError(
			TEInstanceSetFrameRate(MyTEInstance, MyFrameRate, 1),
			TEXT("Unable to set frame rate")))
		{
			return false;
		}

		if (!OutputResultAndCheckForError(
			TEInstanceAssociateGraphicsContext(MyTEInstance, MyTEContext),
			TEXT("Unable to associate graphics Context")))
		{
			return false;
		}
	}

	if (!OutputResultAndCheckForError(
		TEInstanceConfigure(
			MyTEInstance,
			bLoadTox ? TCHAR_TO_UTF8(*ToxPath) : nullptr,
			MyTimeMode),
		bLoadTox // Set different error message depending on intent
			? FString::Printf(TEXT("Unable to configure TouchEngine with tox file '%s'"), *ToxPath)
			: TEXT("Unable to configure TouchEngine")))
	{
		MyConfiguredWithTox = false;
		MyToxPath = "";
		return false;
	}
	MyConfiguredWithTox = bLoadTox;
	MyToxPath = ToxPath;

	return true;
}

void UTouchEngine::PreLoad()
{
	// Prevent running in Commandlet mode
	if (IsRunningCommandlet())
	{
		return;
	}

	if (GIsCookerLoadingPackage)
		return;

	if (MyDevice)
		Clear();

	MyDidLoad = false;

	InstantiateEngineWithToxFile("", __FUNCTION__);
}

void UTouchEngine::PreLoad(const FString& ToxPath)
{
	if (GIsCookerLoadingPackage)
		return;

	if (MyDevice)
		Clear();

	MyDidLoad = false;

	if (ToxPath.IsEmpty())
	{
		const FString ErrMessage(FString::Printf(TEXT("%S: Tox file path is empty"), __FUNCTION__));
		OutputError(ErrMessage);
		MyFailedLoad = true;
		OnLoadFailed.Broadcast(ErrMessage);
		return;
	}

	InstantiateEngineWithToxFile(ToxPath, __FUNCTION__);
}

void UTouchEngine::LoadTox(FString ToxPath)
{
	if (GIsCookerLoadingPackage)
		return;

	MyDidLoad = false;

	if (ToxPath.IsEmpty())
	{
		const FString ErrMessage(FString::Printf(TEXT("%S: Tox file path is empty"), __FUNCTION__));
		OutputError(ErrMessage);
		MyFailedLoad = true;
		OnLoadFailed.Broadcast(ErrMessage);
		return;
	}

	if (!InstantiateEngineWithToxFile(ToxPath, __FUNCTION__))
	{
		return;
	}

	// Checks for result, outputs if not Success and returns true when the severity level is not Error
	const auto OutputResultAndCheckForError = [this](const TEResult Result, const FString& ErrMessage)
	{
		if (Result != TEResultSuccess)
		{
			const FString FullMessage = FString::Printf(TEXT("%S: %s"), __FUNCTION__, *ErrMessage);
			OutputResult(FString::Printf(TEXT("%s: "), *FullMessage), Result);

			if (TEResultGetSeverity(Result) == TESeverity::TESeverityError)
			{
				MyFailedLoad = true;
				OnLoadFailed.Broadcast(ErrMessage);
				return false;
			}
		}
		return true;
	};

	MyLoadCalled = true;
	if (!OutputResultAndCheckForError(
		TEInstanceLoad(MyTEInstance),
		FString::Printf(TEXT("TouchEngine instance failed to load tox file '%s'"), *ToxPath)))
	{
		return;
	}

	OutputResultAndCheckForError(
		TEInstanceResume(MyTEInstance),
		TEXT("Unable to resume TouchEngine"));
}

void UTouchEngine::Unload()
{
	if (MyTEInstance)
	{
		ENQUEUE_RENDER_COMMAND(TouchEngine_Unload_CleanupTextures)(
			[this](FRHICommandList& CommandList)
			{
				if (!IsValid(this))
					return;

				FScopeLock Lock(&MyTOPLock);

				CleanupTextures(MyImmediateContext, &MyTexCleanups, EFinalClean::True);
				MyConfiguredWithTox = false;
				MyDidLoad = false;
				MyFailedLoad = false;
				MyToxPath = "";
				MyLoadCalled = false;
			});
	}
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

void UTouchEngine::AddWarning(const FString& Str)
{
#ifdef WITH_EDITOR
	FScopeLock Lock(&MyMessageLock);
	MyWarnings.Add(Str);
#endif
}

void UTouchEngine::AddResult(const FString& Str, TEResult Result)
{
	FString Str2(Str);
	Str2 += TEResultGetDescription(Result);

	if (TEResultGetSeverity(Result) == TESeverityError)
		AddError(Str2);
	else if (TEResultGetSeverity(Result) == TESeverityWarning)
		AddWarning(Str2);
}

void UTouchEngine::AddError(const FString& Str)
{
#ifdef WITH_EDITOR
	FScopeLock Lock(&MyMessageLock);
	MyErrors.Add(Str);
#endif
}

void UTouchEngine::OutputMessages()
{
#ifdef WITH_EDITOR
	FScopeLock Lock(&MyMessageLock);
	for (FString& Message : MyErrors)
	{
		OutputError(Message);
	}
	for (FString& Message : MyWarnings)
	{
		OutputWarning(Message);
	}
#endif
	MyErrors.Empty();
	MyWarnings.Empty();
}

void UTouchEngine::OutputResult(const FString& Str, TEResult Result)
{
#ifdef WITH_EDITOR
	FString Str2(Str);
	Str2 += TEResultGetDescription(Result);

	if (TEResultGetSeverity(Result) == TESeverityError)
		OutputError(Str2);
	else if (TEResultGetSeverity(Result) == TESeverityWarning)
		OutputWarning(Str2);
#endif
}

void UTouchEngine::OutputError(const FString& Str)
{
#ifdef WITH_EDITOR
	MyMessageLog.Error(FText::Format(LOCTEXT("TEErrorString", "TouchEngine error - {0}"), FText::FromString(Str)));
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

void UTouchEngine::OutputWarning(const FString& Str)
{
#ifdef WITH_EDITOR
	MyMessageLog.Warning(FText::Format(LOCTEXT("TEWarningString", "TouchEngine warning - {0}"), FText::FromString(Str)));
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

static bool IsTypeless(DXGI_FORMAT Format)
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
	if (!MyDidLoad)
	{
		return FTouchTOP();
	}

	if (!ensure(MyTEInstance != nullptr))
	{
		return FTouchTOP();
	}

	std::string FullID("");
	FullID += TCHAR_TO_UTF8(*Identifier);
	TELinkInfo* Param = nullptr;
	TEResult Result = TEInstanceLinkGetInfo(MyTEInstance, FullID.c_str(), &Param);

	if (Result != TEResultSuccess)
	{
		OutputError(FString(TEXT("getTOPOutput(): Unable to find Output named: ")) + Identifier);
		return FTouchTOP();
	}
	else
	{
		TERelease(&Param);
	}

	FScopeLock Lock(&MyTOPLock);

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
		OutputError(FString(TEXT("getTOPOutput(): Unable to find Output named: ")) + Identifier);
		return FTouchTOP();
	}
}

void UTouchEngine::SetTOPInput(const FString& Identifier, UTexture* Texture)
{
	if (!MyDidLoad)
	{
		return;
	}

	std::string FullID("");
	FullID += TCHAR_TO_UTF8(*Identifier);

	TELinkInfo* Param = nullptr;
	TEResult Result = TEInstanceLinkGetInfo(MyTEInstance, FullID.c_str(), &Param);

	if (Result != TEResultSuccess)
	{
		OutputResult(TEXT("setTOPInput(): Unable to get input Info for: ") + Identifier + ". ", Result);
		return;
	}
	TERelease(&Param);

	MyNumInputTexturesQueued++;

	ENQUEUE_RENDER_COMMAND(SetTOPOutput)(
		[this, FullID, Texture](FRHICommandListImmediate& RHICmdList)
		{
			UTexture2D* Tex2D = dynamic_cast<UTexture2D*>(Texture);
			UTextureRenderTarget2D* RT = nullptr;
			if (!Tex2D)
			{
				RT = dynamic_cast<UTextureRenderTarget2D*>(Texture);
			}

			TETexture* TETexture = nullptr;
			ID3D11Texture2D* WrappedResource = nullptr;
			if (MyRHIType == RHIType::DirectX11)
			{
				FD3D11Texture2D* D3D11Texture = nullptr;
				DXGI_FORMAT TypedDXGIFormat = DXGI_FORMAT_UNKNOWN;

				if (Tex2D)
				{
					D3D11Texture = (FD3D11Texture2D*)GetD3D11TextureFromRHITexture(Tex2D->GetResource()->TextureRHI);
					TypedDXGIFormat = toTypedDXGIFormat(Tex2D->GetPixelFormat());
				}
				else if (RT)
				{
					D3D11Texture = (FD3D11Texture2D*)GetD3D11TextureFromRHITexture(RT->GetResource()->TextureRHI);
					TypedDXGIFormat = toTypedDXGIFormat(RT->RenderTargetFormat);
				}
				else
				{
					TEResult res = TEInstanceLinkSetTextureValue(MyTEInstance, FullID.c_str(), nullptr, MyTEContext);
					MyNumInputTexturesQueued--;
					return;
				}

				if (TypedDXGIFormat != DXGI_FORMAT_UNKNOWN)
				{
					if (D3D11Texture)
					{
						D3D11_TEXTURE2D_DESC Desc;
						D3D11Texture->GetResource()->GetDesc(&Desc);
						if (IsTypeless(Desc.Format))
						{
							TETexture = TED3D11TextureCreateTypeless(D3D11Texture->GetResource(), false, kTETextureComponentMapIdentity, TypedDXGIFormat);
						}
						else
						{
							TETexture = TED3D11TextureCreate(D3D11Texture->GetResource(), false, kTETextureComponentMapIdentity);
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

			if (TETexture)
			{
				TEResult res = TEInstanceLinkSetTextureValue(MyTEInstance, FullID.c_str(), TETexture, MyTEContext);
				TERelease(&TETexture);
			}

			if (WrappedResource)
			{
				WrappedResource->Release();
			}

			MyNumInputTexturesQueued--;
		});
}

FTouchCHOPFull UTouchEngine::GetCHOPOutputSingleSample(const FString& Identifier)
{
	if (!MyDidLoad)
	{
		return FTouchCHOPFull();
	}

	FTouchCHOPFull Full;

	std::string FullID("");
	FullID += TCHAR_TO_UTF8(*Identifier);

	TELinkInfo* Param = nullptr;
	TEResult Result = TEInstanceLinkGetInfo(MyTEInstance, FullID.c_str(), &Param);
	if (Result == TEResultSuccess && Param->scope == TEScopeOutput)
	{
		switch (Param->type)
		{
		case TELinkTypeFloatBuffer:
		{

			TEFloatBuffer* Buf = nullptr;
			Result = TEInstanceLinkGetFloatBufferValue(MyTEInstance, FullID.c_str(), TELinkValueDefault, &Buf);

			if (Result == TEResultSuccess && Buf != nullptr)
			{
				if (!MyCHOPSingleOutputs.Contains(Identifier))
				{
					MyCHOPSingleOutputs.Add(Identifier);
				}

				FTouchCHOPSingleSample& Output = MyCHOPSingleOutputs[Identifier];

				int32 ChannelCount = TEFloatBufferGetChannelCount(Buf);
				int64 MaxSamples = TEFloatBufferGetValueCount(Buf);

				int64 Length = MaxSamples;

				double rate = TEFloatBufferGetRate(Buf);
				if (!TEFloatBufferIsTimeDependent(Buf))
				{
					const float* const* Channels = TEFloatBufferGetValues(Buf);
					const char* const* ChannelNames = TEFloatBufferGetChannelNames(Buf);

					if (Result == TEResultSuccess)
					{
						// Use the channel data here
						if (Length > 0 && ChannelCount > 0)
						{
							for (int i = 0; i < ChannelCount; i++)
							{
								Full.SampleData.Add(FTouchCHOPSingleSample());
								Full.SampleData[i].ChannelName = ChannelNames[i];

								for (int j = 0; j < Length; j++)
								{
									Full.SampleData[i].ChannelData.Add(Channels[i][j]);
								}
							}
						}
					}
					// Suppress internal errors for now, some superfluous ones are occuring currently
					else if (Result != TEResultInternalError)
					{
						OutputResult(TEXT("getCHOPOutputSingleSample(): "), Result);
					}
					//c = Output;
					TERelease(&Buf);
				}
				else
				{
					//length /= rate / MyFrameRate;

					const float* const* Channels = TEFloatBufferGetValues(Buf);
					const char* const* ChannelNames = TEFloatBufferGetChannelNames(Buf);

					if (Result == TEResultSuccess)
					{
						// Use the channel data here
						if (Length > 0 && ChannelCount > 0)
						{
							Output.ChannelData.SetNum(ChannelCount);

							for (int32 i = 0; i < ChannelCount; i++)
							{
								Output.ChannelData[i] = Channels[i][Length - 1];
							}
							Output.ChannelName = ChannelNames[0];
						}
					}
					// Suppress internal errors for now, some superfluous ones are occuring currently
					else if (Result != TEResultInternalError)
					{
						OutputResult(TEXT("getCHOPOutputSingleSample(): "), Result);
					}
					Full.SampleData.Add(Output);
					TERelease(&Buf);

				}
			}
			break;
		}
		default:
		{
			OutputError(TEXT("getCHOPOutputSingleSample(): ") + Identifier + TEXT(" is not a CHOP Output."));
			break;
		}
		}
	}
	else if (Result != TEResultSuccess)
	{
		OutputResult(TEXT("getCHOPOutputSingleSample(): "), Result);
	}
	else if (Param->scope == TEScopeOutput)
	{
		OutputError(TEXT("getCHOPOutputSingleSample(): ") + Identifier + TEXT(" is not a CHOP Output."));
	}
	TERelease(&Param);

	return Full;
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

	std::string FullID("");
	FullID += TCHAR_TO_UTF8(*Identifier);


	TEResult Result;
	TELinkInfo* Info;
	Result = TEInstanceLinkGetInfo(MyTEInstance, FullID.c_str(), &Info);

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

	std::vector<float> RealData;
	std::vector<const float*> DataPtrs;
	std::vector<std::string> Names;
	std::vector<const char*> NamesPtrs;
	for (int i = 0; i < CHOP.ChannelData.Num(); i++)
	{
		RealData.push_back(CHOP.ChannelData[i]);
		std::string n("chan");
		n += '1' + i;
		Names.push_back(std::move(n));
	}
	// Seperate loop since realData can reallocate a few times
	for (int i = 0; i < CHOP.ChannelData.Num(); i++)
	{
		DataPtrs.push_back(&RealData[i]);
		NamesPtrs.push_back(Names[i].c_str());
	}

	TEFloatBuffer* Buf = TEFloatBufferCreate(-1.f, CHOP.ChannelData.Num(), 1, NamesPtrs.data());

	Result = TEFloatBufferSetValues(Buf, DataPtrs.data(), 1);

	if (Result != TEResultSuccess)
	{
		OutputResult(FString("setCHOPInputSingleSample(): Failed to set buffer values: "), Result);
		TERelease(&Info);
		TERelease(&Buf);
		return;
	}
	Result = TEInstanceLinkAddFloatBuffer(MyTEInstance, FullID.c_str(), Buf);

	if (Result != TEResultSuccess)
	{
		OutputResult(FString("setCHOPInputSingleSample(): Unable to append buffer values: "), Result);
		TERelease(&Info);
		TERelease(&Buf);
		return;
	}

	TERelease(&Info);
	TERelease(&Buf);
}

FTouchCHOPFull UTouchEngine::GetCHOPOutputs(const FString& Identifier)
{
	if (!MyDidLoad)
	{
		return FTouchCHOPFull();
	}

	FTouchCHOPFull c;

	std::string FullID("");
	FullID += TCHAR_TO_UTF8(*Identifier);

	TELinkInfo* Param = nullptr;
	TEResult Result = TEInstanceLinkGetInfo(MyTEInstance, FullID.c_str(), &Param);
	if (Result == TEResultSuccess && Param->scope == TEScopeOutput)
	{
		switch (Param->type)
		{
		case TELinkTypeFloatBuffer:
		{

			TEFloatBuffer* Buf = nullptr;
			Result = TEInstanceLinkGetFloatBufferValue(MyTEInstance, FullID.c_str(), TELinkValueDefault, &Buf);

			if (Result == TEResultSuccess)
			{
				if (!MyCHOPFullOutputs.Contains(Identifier))
				{
					MyCHOPFullOutputs.Add(Identifier);
				}

				FTouchCHOPFull& Output = MyCHOPFullOutputs[Identifier];

				int32_t ChannelCount = TEFloatBufferGetChannelCount(Buf);
				int64_t MaxSamples = TEFloatBufferGetValueCount(Buf);
				const float* const* Channels = TEFloatBufferGetValues(Buf);
				const char* const* ChannelNames = TEFloatBufferGetChannelNames(Buf);

				if (TEFloatBufferIsTimeDependent(Buf))
				{

				}

				if (Result == TEResultSuccess)
				{
					// Use the channel data here
					if (MaxSamples > 0 && ChannelCount > 0)
					{
						Output.SampleData.SetNum(ChannelCount);
						for (int i = 0; i < ChannelCount; i++)
						{
							Output.SampleData[i].ChannelData.SetNum(MaxSamples);
							Output.SampleData[i].ChannelName = ChannelNames[i];
							for (int j = 0; j < MaxSamples; j++)
							{
								Output.SampleData[i].ChannelData[j] = Channels[i][j];
							}
						}
					}
				}
				// Suppress internal errors for now, some superfluous ones are occuring currently
				else if (Result != TEResultInternalError)
				{
					OutputResult(TEXT("getCHOPOutputs(): "), Result);
				}
				c = Output;
				TERelease(&Buf);
			}
			break;
		}
		default:
		{
			OutputError(TEXT("getCHOPOutputs(): ") + Identifier + TEXT(" is not a CHOP Output."));
			break;
		}
		}
	}
	else if (Result != TEResultSuccess)
	{
		OutputResult(TEXT("getCHOPOutputs(): "), Result);
	}
	else if (Param->scope == TEScopeOutput)
	{
		OutputError(TEXT("getCHOPOutputs(): ") + Identifier + TEXT(" is not a CHOP Output."));
	}
	TERelease(&Param);

	return c;
}

void UTouchEngine::SetCHOPInput(const FString& Identifier, const FTouchCHOPFull& CHOP)
{
}

TTouchVar<bool> UTouchEngine::GetBooleanOutput(const FString& Identifier)
{
	if (!MyDidLoad)
	{
		return TTouchVar<bool>();
	}

	TTouchVar<bool> c = TTouchVar<bool>();

	std::string FullID("");
	FullID += TCHAR_TO_UTF8(*Identifier);

	TELinkInfo* Param = nullptr;
	TEResult Result = TEInstanceLinkGetInfo(MyTEInstance, FullID.c_str(), &Param);
	if (Result == TEResultSuccess && Param->scope == TEScopeOutput)
	{
		switch (Param->type)
		{
		case TELinkTypeBoolean:
		{
			Result = TEInstanceLinkGetBooleanValue(MyTEInstance, FullID.c_str(), TELinkValueCurrent, &c.Data);

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
			OutputError(TEXT("getBooleanOutput(): ") + Identifier + TEXT(" is not a boolean Output."));
			break;
		}
		}
	}
	else if (Result != TEResultSuccess)
	{
		OutputResult(TEXT("getBooleanOutput(): "), Result);
	}
	else if (Param->scope == TEScopeOutput)
	{
		OutputError(TEXT("getBooleanOutput(): ") + Identifier + TEXT(" is not a boolean Output."));
	}
	TERelease(&Param);

	return c;
}

void UTouchEngine::SetBooleanInput(const FString& Identifier, TTouchVar<bool>& Op)
{
	if (!MyTEInstance)
		return;

	if (!MyDidLoad)
	{
		return;
	}

	std::string FullID("");
	FullID += TCHAR_TO_UTF8(*Identifier);

	TEResult Result;
	TELinkInfo* Info;
	Result = TEInstanceLinkGetInfo(MyTEInstance, FullID.c_str(), &Info);

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

	Result = TEInstanceLinkSetBooleanValue(MyTEInstance, FullID.c_str(), Op.Data);

	if (Result != TEResultSuccess)
	{
		OutputResult(FString("setBooleanInput(): Unable to set boolean value: "), Result);
		TERelease(&Info);
		return;
	}

	TERelease(&Info);
}

TTouchVar<double> UTouchEngine::GetDoubleOutput(const FString& Identifier)
{
	if (!MyDidLoad)
	{
		return TTouchVar<double>();
	}

	TTouchVar<double> c = TTouchVar<double>();

	std::string FullID("");
	FullID += TCHAR_TO_UTF8(*Identifier);

	TELinkInfo* Param = nullptr;
	TEResult Result = TEInstanceLinkGetInfo(MyTEInstance, FullID.c_str(), &Param);
	if (Result == TEResultSuccess && Param->scope == TEScopeOutput)
	{
		switch (Param->type)
		{
		case TELinkTypeDouble:
		{
			Result = TEInstanceLinkGetDoubleValue(MyTEInstance, FullID.c_str(), TELinkValueCurrent, &c.Data, 1);

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
			OutputError(TEXT("getDoubleOutput(): ") + Identifier + TEXT(" is not a double Output."));
			break;
		}
		}
	}
	else if (Result != TEResultSuccess)
	{
		OutputResult(TEXT("getDoubleOutput(): "), Result);
	}
	else if (Param->scope == TEScopeOutput)
	{
		OutputError(TEXT("getDoubleOutput(): ") + Identifier + TEXT(" is not a double Output."));
	}
	TERelease(&Param);

	return c;
}

void UTouchEngine::SetDoubleInput(const FString& Identifier, TTouchVar<TArray<double>>& Op)
{
	if (!MyTEInstance)
		return;

	if (!MyDidLoad)
	{
		return;
	}

	std::string FullID("");
	FullID += TCHAR_TO_UTF8(*Identifier);

	TEResult Result;
	TELinkInfo* Info;
	Result = TEInstanceLinkGetInfo(MyTEInstance, FullID.c_str(), &Info);

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

			Result = TEInstanceLinkSetDoubleValue(MyTEInstance, FullID.c_str(), buffer.GetData(), Info->count);
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
		Result = TEInstanceLinkSetDoubleValue(MyTEInstance, FullID.c_str(), Op.Data.GetData(), Op.Data.Num());
	}

	if (Result != TEResultSuccess)
	{
		OutputResult(FString("setDoubleInput(): Unable to set double value: "), Result);
		TERelease(&Info);
		return;
	}

	TERelease(&Info);
}

TTouchVar<int32_t> UTouchEngine::GetIntegerOutput(const FString& Identifier)
{
	if (!MyDidLoad)
	{
		return TTouchVar<int32_t>();
	}

	TTouchVar<int32_t> c = TTouchVar<int32_t>();

	std::string FullID("");
	FullID += TCHAR_TO_UTF8(*Identifier);

	TELinkInfo* Param = nullptr;
	TEResult Result = TEInstanceLinkGetInfo(MyTEInstance, FullID.c_str(), &Param);
	if (Result == TEResultSuccess && Param->scope == TEScopeOutput)
	{
		switch (Param->type)
		{
		case TELinkTypeBoolean:
		{
			Result = TEInstanceLinkGetIntValue(MyTEInstance, FullID.c_str(), TELinkValueCurrent, &c.Data, 1);

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
			OutputError(TEXT("getIntegerOutput(): ") + Identifier + TEXT(" is not an integer Output."));
			break;
		}
		}
	}
	else if (Result != TEResultSuccess)
	{
		OutputResult(TEXT("getIntegerOutput(): "), Result);
	}
	else if (Param->scope == TEScopeOutput)
	{
		OutputError(TEXT("getIntegerOutput(): ") + Identifier + TEXT(" is not an integer Output."));
	}
	TERelease(&Param);

	return c;
}

void UTouchEngine::SetIntegerInput(const FString& Identifier, TTouchVar<TArray<int32_t>>& Op)
{
	if (!MyTEInstance)
		return;

	if (!MyDidLoad)
	{
		return;
	}

	std::string FullID("");
	FullID += TCHAR_TO_UTF8(*Identifier);

	TEResult Result;
	TELinkInfo* Info;
	Result = TEInstanceLinkGetInfo(MyTEInstance, FullID.c_str(), &Info);

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

	Result = TEInstanceLinkSetIntValue(MyTEInstance, FullID.c_str(), Op.Data.GetData(), Op.Data.Num());

	if (Result != TEResultSuccess)
	{
		OutputResult(FString("setIntegerInput(): Unable to set integer value: "), Result);
		TERelease(&Info);
		return;
	}

	TERelease(&Info);
}

TTouchVar<TEString*> UTouchEngine::GetStringOutput(const FString& Identifier)
{
	if (!MyDidLoad)
	{
		return TTouchVar<TEString*>();
	}

	TTouchVar<TEString*> c = TTouchVar<TEString*>();

	std::string FullID("");
	FullID += TCHAR_TO_UTF8(*Identifier);

	TELinkInfo* Param = nullptr;
	TEResult Result = TEInstanceLinkGetInfo(MyTEInstance, FullID.c_str(), &Param);
	if (Result == TEResultSuccess && Param->scope == TEScopeOutput)
	{
		switch (Param->type)
		{
		case TELinkTypeString:
		{
			Result = TEInstanceLinkGetStringValue(MyTEInstance, FullID.c_str(), TELinkValueCurrent, &c.Data);

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
			OutputError(TEXT("getStringOutput(): ") + Identifier + TEXT(" is not a string Output."));
			break;
		}
		}
	}
	else if (Result != TEResultSuccess)
	{
		OutputResult(TEXT("getStringOutput(): "), Result);
	}
	else if (Param->scope == TEScopeOutput)
	{
		OutputError(TEXT("getStringOutput(): ") + Identifier + TEXT(" is not a string Output."));
	}
	TERelease(&Param);

	return c;
}

void UTouchEngine::SetStringInput(const FString& Identifier, TTouchVar<char*>& Op)
{
	if (!MyTEInstance)
		return;

	if (!MyDidLoad)
	{
		return;
	}

	std::string FullID("");
	FullID += TCHAR_TO_UTF8(*Identifier);

	TEResult Result;
	TELinkInfo* Info;
	Result = TEInstanceLinkGetInfo(MyTEInstance, FullID.c_str(), &Info);

	if (Result != TEResultSuccess)
	{
		OutputResult(FString("setStringInput(): Unable to get input Info, ") + FString(Identifier) + " may not exist. ", Result);
		return;
	}

	if (Info->type == TELinkTypeString)
	{
		Result = TEInstanceLinkSetStringValue(MyTEInstance, FullID.c_str(), Op.Data);
	}
	else if (Info->type == TELinkTypeStringData)
	{
		TETable* Table = TETableCreate();
		TETableResize(Table, 1, 1);
		TETableSetStringValue(Table, 0, 0, Op.Data);

		Result = TEInstanceLinkSetTableValue(MyTEInstance, FullID.c_str(), Table);
		TERelease(&Table);
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

	std::string FullID("");
	FullID += TCHAR_TO_UTF8(*Identifier);

	TELinkInfo* Param = nullptr;
	TEResult Result = TEInstanceLinkGetInfo(MyTEInstance, FullID.c_str(), &Param);
	if (Result == TEResultSuccess && Param->scope == TEScopeOutput)
	{
		switch (Param->type)
		{
		case TELinkTypeStringData:
		{
			Result = TEInstanceLinkGetTableValue(MyTEInstance, FullID.c_str(), TELinkValue::TELinkValueCurrent, &c.ChannelData);
			break;
		}
		default:
		{
			OutputError(TEXT("getTableOutput(): ") + Identifier + TEXT(" is not a table Output."));
			break;
		}
		}
	}
	else if (Result != TEResultSuccess)
	{
		OutputResult(TEXT("getTableOutput(): "), Result);
	}
	else if (Param->scope == TEScopeOutput)
	{
		OutputError(TEXT("getTableOutput(): ") + Identifier + TEXT(" is not a table Output."));
	}
	TERelease(&Param);

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

	std::string FullID("");
	FullID += TCHAR_TO_UTF8(*Identifier);

	TEResult Result;
	TELinkInfo* Info;
	Result = TEInstanceLinkGetInfo(MyTEInstance, FullID.c_str(), &Info);

	if (Result != TEResultSuccess)
	{
		OutputResult(FString("setTableInput(): Unable to get input Info, ") + FString(Identifier) + " may not exist. ", Result);
		return;
	}

	if (Info->type == TELinkTypeString)
	{
		const char* string = TETableGetStringValue(Op.ChannelData, 0, 0);
		Result = TEInstanceLinkSetStringValue(MyTEInstance, FullID.c_str(), string);
	}
	else if (Info->type == TELinkTypeStringData)
	{
		Result = TEInstanceLinkSetTableValue(MyTEInstance, FullID.c_str(), Op.ChannelData);
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

void UTouchEngine::SetDidLoad()
{
	MyDidLoad = true;
}

bool UTouchEngine::GetIsLoading() const
{
	return MyLoadCalled && !MyDidLoad && !MyFailedLoad;
}

#undef LOCTEXT_NAMESPACE
