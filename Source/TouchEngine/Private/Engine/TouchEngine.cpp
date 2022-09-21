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

#include "Engine/TouchEngine.h"

#include "TouchEngineDynamicVariableStruct.h"
#include "Async/Async.h"
#include "Engine/Texture2D.h"
#include "Engine/TextureRenderTarget2D.h"

#include <vector>

#include "ITouchEngineModule.h"
#include "TouchEngineParserUtils.h"
#include "TouchEngineResourceProvider.h"
#include "Rendering/Texture2DResource.h"


#define LOCTEXT_NAMESPACE "UTouchEngine"

void UTouchEngine::BeginDestroy()
{
	Clear();

	Super::BeginDestroy();
}

void UTouchEngine::Clear()
{
	// if (MyImmediateContext == nullptr)
	// {
	// 	return;
	// }

	MyCHOPSingleOutputs.Empty();

	/**
	ENQUEUE_RENDER_COMMAND(TouchEngine_Clear_CleanupTextures)(
	[this](FRHICommandListImmediate& RHICmdList)
	{
		if (!IsValid(this))
		{
			return;
		}

		FScopeLock Lock(&MyTOPLock);


		// The ResourceProvider is a singleton currently, so this is probably not a good idea
		if (MyResourceProvider)
			MyResourceProvider->Release();
	});
	**/

	MyDidLoad = false;
	MyFailedLoad = false;
	MyToxPath = "";
	MyConfiguredWithTox = false;
	MyLoadCalled = false;
}

const FString& UTouchEngine::GetToxPath() const
{
	return MyToxPath;
}

void UTouchEngine::EventCallback(TEInstance* Instance, TEEvent Event, TEResult Result, int64_t StartTimeValue, int32_t StartTimeScale, int64_t EndTimeValue, int32_t EndTimeScale, void* Info)
{
	UTouchEngine* Engine = static_cast<UTouchEngine*>(Info);
	if (!Engine)
	{
		return;
	}

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

void UTouchEngine::CleanupTextures_RenderThread(EFinalClean FC)
{
	GetResourceProvider()->ReleaseTextures_RenderThread(FC == EFinalClean::True);
}

void UTouchEngine::LinkValueCallback(TEInstance* Instance, TELinkEvent Event, const char* Identifier, void* Info)
{
	UTouchEngine* Doc = static_cast<UTouchEngine*>(Info);
	Doc->LinkValueCallback(Instance, Event, Identifier);
}

void UTouchEngine::LinkValueCallback(TEInstance* Instance, TELinkEvent Event, const char* Identifier)
{
	if (!ensure(Instance))
	{
		return;
	}

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
				TETexture* Texture = nullptr;
				Result = TEInstanceLinkGetTextureValue(MyTEInstance, Identifier, TELinkValueCurrent, &Texture);

				// TEDXGITexture* DXGITexture = nullptr;
				// Result = TEInstanceLinkGetTextureValue(MyTEInstance, Identifier, TELinkValueCurrent, &DXGITexture);

				if (Result != TEResultSuccess)
				{
					// possible crash without this check
					return;
				}

				MyNumOutputTexturesQueued++;

				FString Name(Identifier);
				ENQUEUE_RENDER_COMMAND(TouchEngine_LinkValueCallback_CleanupTextures)(
					[this, Name, Texture](FRHICommandListImmediate& RHICmdList)
					{
						MyResourceProvider->ReleaseTextures_RenderThread(false);

						TETexture* TED3DTexture = nullptr;

						FScopeLock Lock(&MyTOPLock);

						MyResourceProvider->CreateTexture(Texture, TED3DTexture);
						// TED3D11ContextCreateTexture(MyTEContext, Texture, &TED3DTexture);

						if (!TED3DTexture)
						{
							TERelease(&Texture);
							MyNumOutputTexturesQueued--;
							return;
						}

						FTexture2DResource* d3dSrcTexture = MyResourceProvider->GetTexture(TED3DTexture);
						// ID3D11Texture2D* d3dSrcTexture = TED3D11TextureGetTexture(TED3DTexture);

						if (!d3dSrcTexture)
						{
							TERelease(&TED3DTexture);
							TERelease(&Texture);
							MyNumOutputTexturesQueued--;
							return;
						}

						if (!MyTOPOutputs.Contains(Name))
						{
							MyTOPOutputs.Add(Name);
						}

						EPixelFormat PixelFormat = d3dSrcTexture->GetPixelFormat();

						if (PixelFormat == PF_Unknown)
						{
							AddError(TEXT("Texture with unsupported pixel format being generated by TouchEngine."));
							MyNumOutputTexturesQueued--;
							return;
						}

						// We need this code to be reusable and the DestTexture property must be set later.
						// Right now we don't know if it will be called on this render thread call or on
						// a future one from another thread
						const auto CopyResource = [this, TED3DTexture, Texture, d3dSrcTexture, &RHICmdList](UTexture* DestTexture)
						{
							if (!DestTexture->GetResource())
							{
								TERelease(&TED3DTexture);
								TERelease(&Texture);
								MyNumOutputTexturesQueued--;
								return;
							}

							FTexture2DResource* destResource =  DestTexture->GetResource()->GetTexture2DResource();
							if (destResource)
							{
								/**
								 *　ORIGINAL CODE:
								if (!MyImmediateContext)
								{
									MyNumOutputTexturesQueued--;
									return;
								}
								 *
								 * @todo Is this a meaningful piece of logic, or just a failsafe? -Drakynfly
								 */
								if (!MyResourceProvider)
								{
									MyNumOutputTexturesQueued--;
									return;
								}

								FRHICopyTextureInfo CopyInfo;
								RHICmdList.CopyTexture(d3dSrcTexture->TextureRHI, destResource->TextureRHI, CopyInfo);
								// MyImmediateContext->CopyResource(destResource, d3dSrcTexture);

								// if (MyRHIType == RHIType::DirectX12)
								// { }

								// TODO: We get a crash if we release the TED3DTexture here,
								// so we defer it until D3D tells us it's done with it.
								// Ideally we should be able to release it here and let the ref
								// counting cause it to be cleaned up on it's own later on
								MyResourceProvider->QueueTextureRelease(Texture);
							}
							else
							{
								TERelease(&TED3DTexture);
							}
							TERelease(&Texture);
							MyNumOutputTexturesQueued--;
						}; // FCopyResource

						// Do we need to resize the output texture?
						const FTouchTOP& Output = MyTOPOutputs[Name];
						if (!Output.Texture ||
							Output.Texture->GetSizeX() != d3dSrcTexture->GetSizeX() ||
							Output.Texture->GetSizeY() != d3dSrcTexture->GetSizeY() ||
							Output.Texture->GetPixelFormat() != PixelFormat)
						{
							// Recreate the texture on the GameThread then copy the output to it on the render thread
							AsyncTask(ENamedThreads::AnyThread,
								[this, Name, PixelFormat, TED3DTexture, Texture, d3dSrcTexture, CopyResource]
								{
									// Scope tag for the Engine to know this is not a render thread.
									// Important for the Texture->UpdateResource() call below.
									FTaskTagScope TaskTagScope(ETaskTag::EParallelGameThread);

									FTouchTOP& TmpOutput = MyTOPOutputs[Name];

									MyResourceProvider->ReleaseTexture(TmpOutput);

									/*
									if (Output.WrappedResource)
									{
										Output.WrappedResource->Release();
										Output.WrappedResource = nullptr;
									}

									Output.Texture = nullptr;
									*/

									TmpOutput.Texture = UTexture2D::CreateTransient(d3dSrcTexture->GetSizeX(), d3dSrcTexture->GetSizeY(), PixelFormat);
									TmpOutput.Texture->UpdateResource();

									UTexture* DestTexture = TmpOutput.Texture;

									// Copy the TouchEngine texture to the destination texture on the render thread
									ENQUEUE_RENDER_COMMAND(TouchEngine_CopyResource)(
										[this, DestTexture, TED3DTexture, Texture, d3dSrcTexture, CopyResource]
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

TSharedPtr<UE::TouchEngine::FTouchEngineResourceProvider> UTouchEngine::GetResourceProvider()
{
	return UE::TouchEngine::ITouchEngineModule::Get().GetResourceProvider();
}

void UTouchEngine::Copy(UTouchEngine* Other)
{
	MyTEInstance = Other->MyTEInstance;
	// MyTEContext = Other->MyTEContext;
	// MyDevice = Other->MyDevice;
	// MyImmediateContext = Other->MyImmediateContext;
	MyCHOPSingleOutputs = Other->MyCHOPSingleOutputs;
	MyTOPOutputs = Other->MyTOPOutputs;
	MyMessageLog = Other->MyMessageLog;
	MyLogOpened = Other->MyLogOpened;
	MyErrors = Other->MyErrors;
	MyWarnings = Other->MyWarnings;
	// MyTexCleanups = Other->MyTexCleanups;
	// MyRHIType = Other->MyRHIType;
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
	
	if (!MyResourceProvider)
	{
		MyResourceProvider = GetResourceProvider();
	}

	if (!MyTEInstance)
	{
		TEResult TouchEngineInstace = TEInstanceCreate(EventCallback, LinkValueCallback, this, MyTEInstance.take());
		if (!OutputResultAndCheckForError(TouchEngineInstace, TEXT("Unable to create TouchEngine Instance")))
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
			TEInstanceAssociateGraphicsContext(MyTEInstance, MyResourceProvider->GetContext()),
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
	if (IsRunningCommandlet() || GIsCookerLoadingPackage)
	{
		return;
	}

	if (MyTEInstance)
	{
		Clear();
	}

	MyDidLoad = false;
	InstantiateEngineWithToxFile("", __FUNCTION__);
}

void UTouchEngine::PreLoad(const FString& ToxPath)
{
	if (GIsCookerLoadingPackage)
		return;

	//if (MyDevice)
	if (MyTEInstance)
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

				CleanupTextures_RenderThread(EFinalClean::True);
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

void UTouchEngine::AddResult(const FString& ResultString, TEResult Result)
{
	FString Message = ResultString + TEResultGetDescription(Result);
	switch (TEResultGetSeverity(Result))
	{
	case TESeverityWarning: AddWarning(Message); break;
	case TESeverityError: AddError(Message); break;
	case TESeverityNone:
	default: ;
	}
}

void UTouchEngine::AddWarning(const FString& Str)
{
#if WITH_EDITOR
	FScopeLock Lock(&MyMessageLock);
	MyWarnings.Add(Str);
#endif
}

void UTouchEngine::AddError(const FString& Str)
{
#if WITH_EDITOR
	FScopeLock Lock(&MyMessageLock);
	MyErrors.Add(Str);
#endif
}

void UTouchEngine::OutputMessages()
{
#if WITH_EDITOR
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

void UTouchEngine::OutputResult(const FString& ResultString, TEResult Result)
{
#ifdef WITH_EDITOR
	FString Message = ResultString + TEResultGetDescription(Result);
	switch (TEResultGetSeverity(Result))
	{
	case TESeverityWarning: OutputError(Message); break;
	case TESeverityError: OutputWarning(Message); break;
	case TESeverityNone:
	default: ;
	}
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
			TETexture* TETexture = nullptr;

			//FD3D11Texture2D* D3D11Texture = nullptr;
			//DXGI_FORMAT TypedDXGIFormat = DXGI_FORMAT_UNKNOWN;
			FRHITexture2D* RHI_Texture = nullptr;
			EPixelFormat Format = PF_Unknown;

			if (UTexture2D* Tex2D = Cast<UTexture2D>(Texture))
			{
				RHI_Texture = Tex2D->GetResource()->TextureRHI->GetTexture2D();
				Format = Tex2D->GetPixelFormat();

				//D3D11Texture = (FD3D11Texture2D*)GetD3D11TextureFromRHITexture(Tex2D->GetResource()->TextureRHI);
				//TypedDXGIFormat = toTypedDXGIFormat(Tex2D->GetPixelFormat());
			}
			else if (UTextureRenderTarget2D* RT = Cast<UTextureRenderTarget2D>(Texture))
			{
				RHI_Texture = RT->GetResource()->TextureRHI->GetTexture2D();
				Format = GetPixelFormatFromRenderTargetFormat(RT->RenderTargetFormat);

				//D3D11Texture = (FD3D11Texture2D*)GetD3D11TextureFromRHITexture(RT->GetResource()->TextureRHI);
				//TypedDXGIFormat = toTypedDXGIFormat(RT->RenderTargetFormat);
			}
			else
			{
				TEResult Res = TEInstanceLinkSetTextureValue(MyTEInstance, FullID.c_str(), nullptr, MyResourceProvider->GetContext());
				MyNumInputTexturesQueued--;
				return;
			}

			if (MyResourceProvider->IsSupportedFormat(Format))
			{
				if (RHI_Texture)
				{
					TETexture = MyResourceProvider->CreateTextureWithFormat(RHI_Texture, TETextureOriginTopLeft, kTETextureComponentMapIdentity, Format);
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

			if (TETexture)
			{
				TEResult res = TEInstanceLinkSetTextureValue(MyTEInstance, FullID.c_str(), TETexture, MyResourceProvider->GetContext());
				TERelease(&TETexture);
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
