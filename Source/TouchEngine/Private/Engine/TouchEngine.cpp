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

#include "Engine/Texture2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "ITouchEngineModule.h"
#include "TouchEngineDynamicVariableStruct.h"
#include "Rendering/Texture2DResource.h"
#include "Rendering/TouchResourceProvider.h"

#include <vector>

#include "Logging.h"
#include "Async/Async.h"
#include "TouchEngineParserUtils.h"

#define LOCTEXT_NAMESPACE "UTouchEngine"

void UTouchEngine::BeginDestroy()
{
	Clear();
	Super::BeginDestroy();
}

void UTouchEngine::LoadTox(const FString& ToxPath)
{
	if (GIsCookerLoadingPackage)
	{
		return;
	}

	MyDidLoad = false;

	if (ToxPath.IsEmpty())
	{
		const FString ErrMessage(FString::Printf(TEXT("%S: Tox file path is empty"), __FUNCTION__));
		OutputError(ErrMessage);
		MyFailedLoad = true;
		OnLoadFailed.Broadcast(ErrMessage);
		return;
	}

	if (!InstantiateEngineWithToxFile(ToxPath))
	{
		return;
	}

	MyLoadCalled = true;
	UE_LOG(LogTouchEngine, Log, TEXT("Loading %s"), *ToxPath);
	if (!OutputResultAndCheckForError(TEInstanceLoad(MyTouchEngineInstance), FString::Printf(TEXT("TouchEngine instance failed to load tox file '%s'"), *ToxPath)))
	{
		return;
	}

	OutputResultAndCheckForError(TEInstanceResume(MyTouchEngineInstance), TEXT("Unable to resume TouchEngine"));
}

void UTouchEngine::Unload()
{
	if (MyTouchEngineInstance)
	{
		ENQUEUE_RENDER_COMMAND(TouchEngine_Unload_CleanupTextures)(
			[this](FRHICommandList& CommandList)
			{
				if (!IsValid(this))
				{
					return;
				}

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

void UTouchEngine::CookFrame_GameThread(int64 FrameTime_Mill)
{
	check(IsInGameThread());
	
	OutputMessages();
	if (MyDidLoad && !MyCooking)
	{
		if (!ensureMsgf(MyTouchEngineInstance, TEXT("If MyDidLoad is true, then we shouldn't have a null Instance")))
		{
			return;
		}

		TEResult Result = (TEResult)0;

		FlushRenderingCommands();
		switch (MyTimeMode)
		{
			case TETimeInternal:
			{
				Result = TEInstanceStartFrameAtTime(MyTouchEngineInstance, 0, 0, false);
				break;
			}
			case TETimeExternal:
			{
				MyTime += FrameTime_Mill;
				Result = TEInstanceStartFrameAtTime(MyTouchEngineInstance, MyTime, 10000, false);
				break;
			}
		}

		const bool bSuccess = Result == TEResultSuccess;
		MyCooking = bSuccess;
		if (!bSuccess)
		{
			OutputResult(FString("cookFrame(): Failed to cook frame: "), Result);
		}
	}
}

bool UTouchEngine::SetCookMode(bool IsIndependent)
{
	if (IsIndependent)
	{
		MyTimeMode = TETimeMode::TETimeInternal;
	}
	else
	{
		MyTimeMode = TETimeMode::TETimeExternal;
	}
	return true;
}

bool UTouchEngine::SetFrameRate(int64 FrameRate)
{
	if (!MyTouchEngineInstance)
	{
		MyFrameRate = FrameRate;
		return true;
	}

	return false;
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
	TEResult Result = TEInstanceLinkGetInfo(MyTouchEngineInstance, FullID.c_str(), &Param);

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
				TEResult Res = TEInstanceLinkSetTextureValue(MyTouchEngineInstance, FullID.c_str(), nullptr, MyResourceProvider->GetContext());
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
					AddError_AnyThread(TEXT("setTOPInput(): Unable to create TouchEngine copy of texture."));
				}
			}
			else
			{
				AddError_AnyThread(TEXT("setTOPInput(): Unsupported pixel format for texture input. Compressed textures are not supported."));
			}

			if (TETexture)
			{
				TEResult res = TEInstanceLinkSetTextureValue(MyTouchEngineInstance, FullID.c_str(), TETexture, MyResourceProvider->GetContext());
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
	TEResult Result = TEInstanceLinkGetInfo(MyTouchEngineInstance, FullID.c_str(), &Param);
	if (Result == TEResultSuccess && Param->scope == TEScopeOutput)
	{
		switch (Param->type)
		{
		case TELinkTypeFloatBuffer:
		{

			TEFloatBuffer* Buf = nullptr;
			Result = TEInstanceLinkGetFloatBufferValue(MyTouchEngineInstance, FullID.c_str(), TELinkValueDefault, &Buf);

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
	TEResult Result = TEInstanceLinkGetInfo(MyTouchEngineInstance, FullID.c_str(), &Param);
	if (Result == TEResultSuccess && Param->scope == TEScopeOutput)
	{
		switch (Param->type)
		{
		case TELinkTypeFloatBuffer:
		{

			TEFloatBuffer* Buf = nullptr;
			Result = TEInstanceLinkGetFloatBufferValue(MyTouchEngineInstance, FullID.c_str(), TELinkValueDefault, &Buf);

			if (Result == TEResultSuccess)
			{
				FTouchCHOPFull& Output = MyCHOPFullOutputs.FindOrAdd(Identifier);

				const int32 ChannelCount = TEFloatBufferGetChannelCount(Buf);
				const int64 MaxSamples = TEFloatBufferGetValueCount(Buf);
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

FTouchTOP UTouchEngine::GetTOPOutput(const FString& Identifier)
{
	if (!MyDidLoad)
	{
		return FTouchTOP();
	}

	if (!ensure(MyTouchEngineInstance != nullptr))
	{
		return FTouchTOP();
	}

	std::string FullID("");
	FullID += TCHAR_TO_UTF8(*Identifier);
	TELinkInfo* Param = nullptr;
	TEResult Result = TEInstanceLinkGetInfo(MyTouchEngineInstance, FullID.c_str(), &Param);

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

FTouchDATFull UTouchEngine::GetTableOutput(const FString& Identifier)
{
	FTouchDATFull c = FTouchDATFull();
	if (!MyDidLoad)
	{
		return c;
	}

	std::string FullID("");
	FullID += TCHAR_TO_UTF8(*Identifier);

	TELinkInfo* Param = nullptr;
	TEResult Result = TEInstanceLinkGetInfo(MyTouchEngineInstance, FullID.c_str(), &Param);
	if (Result == TEResultSuccess && Param->scope == TEScopeOutput)
	{
		switch (Param->type)
		{
		case TELinkTypeStringData:
			{
				Result = TEInstanceLinkGetTableValue(MyTouchEngineInstance, FullID.c_str(), TELinkValue::TELinkValueCurrent, &c.ChannelData);
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
	TEResult Result = TEInstanceLinkGetInfo(MyTouchEngineInstance, FullID.c_str(), &Param);
	if (Result == TEResultSuccess && Param->scope == TEScopeOutput)
	{
		switch (Param->type)
		{
		case TELinkTypeBoolean:
		{
			Result = TEInstanceLinkGetBooleanValue(MyTouchEngineInstance, FullID.c_str(), TELinkValueCurrent, &c.Data);

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
	TEResult Result = TEInstanceLinkGetInfo(MyTouchEngineInstance, FullID.c_str(), &Param);
	if (Result == TEResultSuccess && Param->scope == TEScopeOutput)
	{
		switch (Param->type)
		{
		case TELinkTypeDouble:
		{
			Result = TEInstanceLinkGetDoubleValue(MyTouchEngineInstance, FullID.c_str(), TELinkValueCurrent, &c.Data, 1);

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
	TEResult Result = TEInstanceLinkGetInfo(MyTouchEngineInstance, FullID.c_str(), &Param);
	if (Result == TEResultSuccess && Param->scope == TEScopeOutput)
	{
		switch (Param->type)
		{
		case TELinkTypeBoolean:
		{
			Result = TEInstanceLinkGetIntValue(MyTouchEngineInstance, FullID.c_str(), TELinkValueCurrent, &c.Data, 1);

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
	TEResult Result = TEInstanceLinkGetInfo(MyTouchEngineInstance, FullID.c_str(), &Param);
	if (Result == TEResultSuccess && Param->scope == TEScopeOutput)
	{
		switch (Param->type)
		{
		case TELinkTypeString:
			{
				Result = TEInstanceLinkGetStringValue(MyTouchEngineInstance, FullID.c_str(), TELinkValueCurrent, &c.Data);

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

void UTouchEngine::SetCHOPInputSingleSample(const FString& Identifier, const FTouchCHOPSingleSample& CHOP)
{
	if (!MyTouchEngineInstance)
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
	Result = TEInstanceLinkGetInfo(MyTouchEngineInstance, FullID.c_str(), &Info);

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
	Result = TEInstanceLinkAddFloatBuffer(MyTouchEngineInstance, FullID.c_str(), Buf);

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

void UTouchEngine::SetCHOPInput(const FString& Identifier, const FTouchCHOPFull& CHOP)
{
}

void UTouchEngine::SetBooleanInput(const FString& Identifier, TTouchVar<bool>& Op)
{
	if (!MyTouchEngineInstance)
	{
		return;
	}

	if (!MyDidLoad)
	{
		return;
	}

	std::string FullID("");
	FullID += TCHAR_TO_UTF8(*Identifier);

	TEResult Result;
	TELinkInfo* Info;
	Result = TEInstanceLinkGetInfo(MyTouchEngineInstance, FullID.c_str(), &Info);

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

	Result = TEInstanceLinkSetBooleanValue(MyTouchEngineInstance, FullID.c_str(), Op.Data);

	if (Result != TEResultSuccess)
	{
		OutputResult(FString("setBooleanInput(): Unable to set boolean value: "), Result);
		TERelease(&Info);
		return;
	}

	TERelease(&Info);
}

void UTouchEngine::SetDoubleInput(const FString& Identifier, TTouchVar<TArray<double>>& Op)
{
	if (!MyTouchEngineInstance)
	{
		return;
	}

	if (!MyDidLoad)
	{
		return;
	}

	std::string FullID("");
	FullID += TCHAR_TO_UTF8(*Identifier);

	TEResult Result;
	TELinkInfo* Info;
	Result = TEInstanceLinkGetInfo(MyTouchEngineInstance, FullID.c_str(), &Info);

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

			Result = TEInstanceLinkSetDoubleValue(MyTouchEngineInstance, FullID.c_str(), buffer.GetData(), Info->count);
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
		Result = TEInstanceLinkSetDoubleValue(MyTouchEngineInstance, FullID.c_str(), Op.Data.GetData(), Op.Data.Num());
	}

	if (Result != TEResultSuccess)
	{
		OutputResult(FString("setDoubleInput(): Unable to set double value: "), Result);
		TERelease(&Info);
		return;
	}

	TERelease(&Info);
}

void UTouchEngine::SetIntegerInput(const FString& Identifier, TTouchVar<TArray<int32_t>>& Op)
{
	if (!MyTouchEngineInstance)
	{
		return;
	}

	if (!MyDidLoad)
	{
		return;
	}

	std::string FullID("");
	FullID += TCHAR_TO_UTF8(*Identifier);

	TEResult Result;
	TELinkInfo* Info;
	Result = TEInstanceLinkGetInfo(MyTouchEngineInstance, FullID.c_str(), &Info);

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

	Result = TEInstanceLinkSetIntValue(MyTouchEngineInstance, FullID.c_str(), Op.Data.GetData(), Op.Data.Num());

	if (Result != TEResultSuccess)
	{
		OutputResult(FString("setIntegerInput(): Unable to set integer value: "), Result);
		TERelease(&Info);
		return;
	}

	TERelease(&Info);
}

void UTouchEngine::SetStringInput(const FString& Identifier, TTouchVar<char*>& Op)
{
	if (!MyTouchEngineInstance)
	{
		return;
	}

	if (!MyDidLoad)
	{
		return;
	}

	std::string FullID("");
	FullID += TCHAR_TO_UTF8(*Identifier);

	TEResult Result;
	TELinkInfo* Info;
	Result = TEInstanceLinkGetInfo(MyTouchEngineInstance, FullID.c_str(), &Info);

	if (Result != TEResultSuccess)
	{
		OutputResult(FString("setStringInput(): Unable to get input Info, ") + FString(Identifier) + " may not exist. ", Result);
		return;
	}

	if (Info->type == TELinkTypeString)
	{
		Result = TEInstanceLinkSetStringValue(MyTouchEngineInstance, FullID.c_str(), Op.Data);
	}
	else if (Info->type == TELinkTypeStringData)
	{
		TETable* Table = TETableCreate();
		TETableResize(Table, 1, 1);
		TETableSetStringValue(Table, 0, 0, Op.Data);

		Result = TEInstanceLinkSetTableValue(MyTouchEngineInstance, FullID.c_str(), Table);
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

void UTouchEngine::SetTableInput(const FString& Identifier, FTouchDATFull& Op)
{
	if (!MyTouchEngineInstance || !MyDidLoad)
	{
		return;
	}

	std::string FullID("");
	FullID += TCHAR_TO_UTF8(*Identifier);

	TELinkInfo* Info;
	TEResult Result = TEInstanceLinkGetInfo(MyTouchEngineInstance, FullID.c_str(), &Info);
	if (Result != TEResultSuccess)
	{
		OutputResult(FString("setTableInput(): Unable to get input Info, ") + FString(Identifier) + " may not exist. ", Result);
		return;
	}

	if (Info->type == TELinkTypeString)
	{
		const char* string = TETableGetStringValue(Op.ChannelData, 0, 0);
		Result = TEInstanceLinkSetStringValue(MyTouchEngineInstance, FullID.c_str(), string);
	}
	else if (Info->type == TELinkTypeStringData)
	{
		Result = TEInstanceLinkSetTableValue(MyTouchEngineInstance, FullID.c_str(), Op.ChannelData);
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

bool UTouchEngine::GetIsLoading() const
{
	return MyLoadCalled && !MyDidLoad && !MyFailedLoad;
}

bool UTouchEngine::InstantiateEngineWithToxFile(const FString& ToxPath)
{
	if (!ToxPath.IsEmpty() && !ToxPath.EndsWith(".tox"))
	{
		MyFailedLoad = true;
		const FString FullMessage = FString::Printf(TEXT("Invalid file path - %s"), *ToxPath);
		OutputError(FullMessage);
		OnLoadFailed.Broadcast(FullMessage);
		return false;
	}

	// Causes old MyResourceProvider to be destroyed thus releasing its render resources
	MyResourceProvider = UE::TouchEngine::ITouchEngineModule::Get().CreateResourceProvider();
	
	if (!MyTouchEngineInstance)
	{
		const TEResult TouchEngineInstace = TEInstanceCreate(TouchEventCallback_GameOrTouchThread, LinkValueCallback_AnyThread, this, MyTouchEngineInstance.take());
		if (!OutputResultAndCheckForError(TouchEngineInstace, TEXT("Unable to create TouchEngine Instance")))
		{
			return false;
		}

		const TEResult SetFrameResult = TEInstanceSetFrameRate(MyTouchEngineInstance, MyFrameRate, 1);
		if (!OutputResultAndCheckForError(SetFrameResult, TEXT("Unable to set frame rate")))
		{
			return false;
		}

		const TEResult GraphicsContextResult = TEInstanceAssociateGraphicsContext(MyTouchEngineInstance, MyResourceProvider->GetContext());
		if (!OutputResultAndCheckForError(GraphicsContextResult, TEXT("Unable to associate graphics Context")))
		{
			return false;
		}
	}

	const bool bLoadTox = !ToxPath.IsEmpty();
	const char* Path = bLoadTox ? TCHAR_TO_UTF8(*ToxPath) : nullptr;
	// Set different error message depending on intent
	const FString ErrorMessage = bLoadTox 
		? FString::Printf(TEXT("Unable to configure TouchEngine with tox file '%s'"), *ToxPath)
		: TEXT("Unable to configure TouchEngine");
	const TEResult ConfigurationResult = TEInstanceConfigure(MyTouchEngineInstance, Path, MyTimeMode);
	if (!OutputResultAndCheckForError(ConfigurationResult, ErrorMessage))
	{
		MyConfiguredWithTox = false;
		MyToxPath = "";
		return false;
	}
	
	MyConfiguredWithTox = bLoadTox;
	MyToxPath = ToxPath;
	return true;
}

void UTouchEngine::TouchEventCallback_GameOrTouchThread(TEInstance* Instance, TEEvent Event, TEResult Result, int64_t StartTimeValue, int32_t StartTimeScale, int64_t EndTimeValue, int32_t EndTimeScale, void* Info)
{
	check(IsInGameOrTouchThread());
	
	UTouchEngine* Engine = static_cast<UTouchEngine*>(Info);
	if (!Engine)
	{
		return;
	}

	switch (Event)
	{
	case TEEventInstanceDidLoad:
	{
		Engine->OnInstancedLoaded_GameOrTouchThread(Instance, Result);
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

void UTouchEngine::OnInstancedLoaded_GameOrTouchThread(TEInstance* Instance, TEResult Result)
{
	check(IsInGameOrTouchThread());

	switch (Result)
	{
	case TEResultFileError:
		OnLoadError_GameOrTouchThread(Result, FString::Printf(TEXT("load() failed to load .tox \"%s\""), *MyToxPath));
		break;

	case TEResultIncompatibleEngineVersion:
		OnLoadError_GameOrTouchThread(Result);
		break;
		
	case TEResultSuccess:
		LoadInstance_GameOrTouchThread(Instance);
		break;
	default:
		TEResultGetSeverity(Result) == TESeverityError
			? OnLoadError_GameOrTouchThread(Result, TEXT("load() severe tox file error:"))
			: LoadInstance_GameOrTouchThread(Instance);
	}
}

void UTouchEngine::LoadInstance_GameOrTouchThread(TEInstance* Instance)
{
	check(IsInGameOrTouchThread());

	const TPair<TEResult, TArray<FTouchEngineDynamicVariableStruct>> VariablesIn = ProcessTouchVariables(Instance, TEScopeInput);
	const TPair<TEResult, TArray<FTouchEngineDynamicVariableStruct>> VariablesOut = ProcessTouchVariables(Instance, TEScopeOutput);

	const TEResult VarInResult = VariablesIn.Key;
	if (VarInResult != TEResultSuccess)
	{
		OnLoadError_GameOrTouchThread(VarInResult, TEXT("Failed to load input variables."));
		return;
	}

	const TEResult VarOutResult = VariablesIn.Key;
	if (VarOutResult != TEResultSuccess)
	{
		OnLoadError_GameOrTouchThread(VarOutResult, TEXT("Failed to load ouput variables."));
		return;
	}
	
	SetDidLoad();
	AsyncTask(ENamedThreads::GameThread,
		[this, VariablesIn, VariablesOut]()
		{
			UE_LOG(LogTouchEngine, Log, TEXT("Loaded %s"), *GetToxPath());
			OnParametersLoaded.Broadcast(VariablesIn.Value, VariablesOut.Value);
		}
	);
}

void UTouchEngine::OnLoadError_GameOrTouchThread(TEResult Result, const FString& BaseErrorMessage)
{
	check(IsInGameOrTouchThread());
	
	AsyncTask(ENamedThreads::GameThread,
		[this, BaseErrorMessage, Result]()
		{
			const FString FinalMessage = BaseErrorMessage.IsEmpty()
				? FString::Printf(TEXT("%s %hs"), *BaseErrorMessage, TEResultGetDescription(Result))
				: TEResultGetDescription(Result);
			OutputError(FinalMessage);
			
			MyFailedLoad = true;
			OnLoadFailed.Broadcast(TEResultGetDescription(Result));
		}
	);
}

TPair<TEResult, TArray<FTouchEngineDynamicVariableStruct>> UTouchEngine::ProcessTouchVariables(TEInstance* Instance, TEScope Scope)
{
	TArray<FTouchEngineDynamicVariableStruct> Variables;
	
	TEStringArray* Groups;
	const TEResult LinkResult = TEInstanceGetLinkGroups(Instance, Scope, &Groups);
	if (LinkResult != TEResultSuccess)
	{
		return { LinkResult, Variables };
	}

	for (int32_t i = 0; i < Groups->count; i++)
	{
		FTouchEngineParserUtils::ParseGroup(Instance, Groups->strings[i], Variables);
	}

	return { LinkResult, Variables };
}

void UTouchEngine::LinkValueCallback_AnyThread(TEInstance* Instance, TELinkEvent Event, const char* Identifier, void* Info)
{
	UTouchEngine* Doc = static_cast<UTouchEngine*>(Info);
	Doc->LinkValue_AnyThread(Instance, Event, Identifier);
}

void UTouchEngine::LinkValue_AnyThread(TEInstance* Instance, TELinkEvent Event, const char* Identifier)
{
	if (!ensure(Instance))
	{
		return;
	}

	TELinkInfo* Param = nullptr;
	TEResult Result = TEInstanceLinkGetInfo(Instance, Identifier, &Param);
	ON_SCOPE_EXIT
	{
		TERelease(&Param);
	};

	const bool bIsOutputValue = Result == TEResultSuccess && Param && Param->scope == TEScopeOutput;
	const bool bHasValueChanged = Event == TELinkEventValueChange;
	const bool bIsTextureValue = Param && Param->type == TELinkTypeTexture;
	if (bIsOutputValue && bHasValueChanged && bIsTextureValue)
	{
		ProcessLinkTextureValueChanged_AnyThread(Identifier);
	}
}

void UTouchEngine::ProcessLinkTextureValueChanged_AnyThread(const char* Identifier)
{
	// Stash the state, we don't do any actual renderer work from this thread
	TETexture* Texture = nullptr;
	const TEResult Result = TEInstanceLinkGetTextureValue(MyTouchEngineInstance, Identifier, TELinkValueCurrent, &Texture);
	// TODO DP: Check whether this is needed - it was already commented out
	// TEDXGITexture* DXGITexture = nullptr;
	// Result = TEInstanceLinkGetTextureValue(MyTEInstance, Identifier, TELinkValueCurrent, &DXGITexture);

	if (Result != TEResultSuccess)
	{
		return;
	}

	const FString Name(Identifier);
	MyNumOutputTexturesQueued++; // TODO DP: Data race with game thread
	ENQUEUE_RENDER_COMMAND(TouchEngine_LinkValueCallback_CleanupTextures)(
		[this, Name, Texture](FRHICommandListImmediate& RHICmdList)
		{
			// TODO DP: Won't this release the textures of the other params? Bug?
			MyResourceProvider->ReleaseTextures_RenderThread(false);

			TETexture* GraphicsApiTexture = nullptr;
			FScopeLock Lock(&MyTOPLock);
			MyResourceProvider->CreateTexture(Texture, GraphicsApiTexture);
			// TODO DP: CreateTexture replaces the following call. Verify and then delete this comment.
			// TED3D11ContextCreateTexture(MyTEContext, Texture, &TED3DTexture);
			if (!GraphicsApiTexture)
			{
				TERelease(&Texture);
				MyNumOutputTexturesQueued--;
				return;
			}

			FTexture2DResource* GraphicsApiTextureResource = MyResourceProvider->GetTexture(GraphicsApiTexture);
			// TODO DP: CreateTexture replaces the following call. Verify and then delete this comment.
			// ID3D11Texture2D* d3dSrcTexture = TED3D11TextureGetTexture(TED3DTexture);
			if (!GraphicsApiTextureResource)
			{
				TERelease(&GraphicsApiTexture);
				TERelease(&Texture);
				MyNumOutputTexturesQueued--;
				return;
			}

			const EPixelFormat PixelFormat = GraphicsApiTextureResource->GetPixelFormat();
			if (PixelFormat == PF_Unknown)
			{
				AddError_AnyThread(TEXT("Texture with unsupported pixel format being generated by TouchEngine."));
				MyNumOutputTexturesQueued--;
				return;
			}

			// We need this code to be reusable and the DestTexture property must be set later.
			// Right now we don't know if it will be called on this render thread call or on
			// a future one from another thread

			using namespace UE::TouchEngine; 
			const TWeakPtr<FTouchResourceProvider> WeakResourceProvider = TWeakPtr<FTouchResourceProvider>(MyResourceProvider);
			const auto CopyResource = [this, GraphicsApiTexture, Texture, GraphicsApiTextureResource, WeakResourceProvider](UTexture* DestTexture, FRHICommandListImmediate& RHICmdList)
			{
				if (!DestTexture->GetResource())
				{
					TERelease(&GraphicsApiTexture);
					TERelease(&Texture);
					MyNumOutputTexturesQueued--;
					return;
				}

				TSharedPtr<FTouchResourceProvider> ResourceProviderPin = WeakResourceProvider.Pin();
				if (ResourceProviderPin == nullptr
					// TODO DP: This happens if a new tox is requested in the mean time, e.g. by calling LoadTox. In that case, we should rather cancel this task instead...
					|| ResourceProviderPin != MyResourceProvider)
				{
					return;
				}
				
				FTexture2DResource* destResource = DestTexture->GetResource()->GetTexture2DResource();
				if (destResource)
				{
					FRHICopyTextureInfo CopyInfo;
					RHICmdList.CopyTexture(GraphicsApiTextureResource->TextureRHI, destResource->TextureRHI, CopyInfo);

					// TODO: We get a crash if we release the TED3DTexture here,
					// so we defer it until D3D tells us it's done with it.
					// Ideally we should be able to release it here and let the ref
					// counting cause it to be cleaned up on it's own later on
					ResourceProviderPin->QueueTextureRelease_RenderThread(Texture);
				}
				else
				{
					TERelease(&GraphicsApiTexture);
				}
				TERelease(&Texture);
				MyNumOutputTexturesQueued--;
			};

			// Do we need to resize the output texture?
			const FTouchTOP& Output = MyTOPOutputs.FindOrAdd(Name);
			if (!Output.Texture ||
				Output.Texture->GetSizeX() != GraphicsApiTextureResource->GetSizeX() ||
				Output.Texture->GetSizeY() != GraphicsApiTextureResource->GetSizeY() ||
				Output.Texture->GetPixelFormat() != PixelFormat)
			{
				// TODO DP: Why not just create the texture directly on the render thread? Btw async uobject creation is supported in UE5
				// Recreate the texture on the GameThread then copy the output to it on the render thread
				AsyncTask(ENamedThreads::AnyThread,
					[this, Name, PixelFormat, GraphicsApiTexture, Texture, GraphicsApiTextureResource, CopyResource]
					{
						// Scope tag for the Engine to know this is not a render thread.
				        // Important for the Texture->UpdateResource() call below.
				        FTaskTagScope TaskTagScope(ETaskTag::EParallelGameThread);
						FTouchTOP& TmpOutput = MyTOPOutputs[Name];
				        MyResourceProvider->ReleaseTexture(TmpOutput);

				        TmpOutput.Texture = UTexture2D::CreateTransient(GraphicsApiTextureResource->GetSizeX(), GraphicsApiTextureResource->GetSizeY(), PixelFormat);
				        TmpOutput.Texture->UpdateResource();

				        UTexture* DestTexture = TmpOutput.Texture;

				        // Copy the TouchEngine texture to the destination texture on the render thread
				        ENQUEUE_RENDER_COMMAND(TouchEngine_CopyResource)(
				        	[this, DestTexture, CopyResource](FRHICommandListImmediate& RHICommandList)
				        	{
				        		CopyResource(DestTexture, RHICommandList);
				        	});
			          });
			}
			else
			{
				// If we don't need to resize the texture, copy the resource right now on the render thread
				CopyResource(Output.Texture, RHICmdList);
			}
		}
	);
}

void UTouchEngine::Clear()
{
	MyResourceProvider.Reset(); // Release the rendering resources
	MyTouchEngineInstance.reset();
	
	MyCHOPSingleOutputs.Empty();

	MyDidLoad = false;
	MyFailedLoad = false;
	MyToxPath = "";
	MyConfiguredWithTox = false;
	MyLoadCalled = false;
}

void UTouchEngine::CleanupTextures_RenderThread(EFinalClean FC)
{
	MyResourceProvider->ReleaseTextures_RenderThread(FC == EFinalClean::True);
}

bool UTouchEngine::IsInGameOrTouchThread()
{
	return IsInGameThread() || !IsInRenderingThread();
}

bool UTouchEngine::OutputResultAndCheckForError(const TEResult Result, const FString& ErrMessage)
{
	if (Result != TEResultSuccess)
	{
		OutputResult(FString::Printf(TEXT("%s: "), *ErrMessage), Result);
		if (TEResultGetSeverity(Result) == TESeverity::TESeverityError)
		{
			MyFailedLoad = true;
			OnLoadFailed.Broadcast(ErrMessage);
			return false;
		}
	}
	return true;
}

void UTouchEngine::AddResult(const FString& ResultString, TEResult Result)
{
	FString Message = ResultString + TEResultGetDescription(Result);
	switch (TEResultGetSeverity(Result))
	{
	case TESeverityWarning: AddWarning_AnyThread(Message); break;
	case TESeverityError: AddError_AnyThread(Message); break;
	case TESeverityNone:
	default: ;
	}
}

void UTouchEngine::AddWarning_AnyThread(const FString& Str)
{
#if WITH_EDITOR
	FScopeLock Lock(&MyMessageLock);
	MyWarnings.Add(Str);
#endif
}

void UTouchEngine::AddError_AnyThread(const FString& Str)
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
#if WITH_EDITOR
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
	UE_LOG(LogTouchEngine, Error, TEXT("Failed to load %s: %s"), *GetToxPath(), *Str);

#if WITH_EDITOR
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
#if WITH_EDITOR
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

#undef LOCTEXT_NAMESPACE
