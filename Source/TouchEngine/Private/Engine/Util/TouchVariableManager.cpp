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

#include "Engine/Util/TouchVariableManager.h"

#include "Logging.h"
#include "Engine/Util/TouchErrorLog.h"
#include "Rendering/Exporting/TouchExportParams.h"
#include "Rendering/TouchResourceProvider.h"
#include "TouchEngineDynamicVariableStruct.h"

#include "Algo/IndexOf.h"

#include <vector>

namespace UE::TouchEngine
{
	FTouchVariableManager::FTouchVariableManager(
		TouchObject<TEInstance> TouchEngineInstance,
		TSharedPtr<FTouchResourceProvider> ResourceProvider,
		TSharedPtr<FTouchErrorLog> ErrorLog
		)
		: TouchEngineInstance(MoveTemp(TouchEngineInstance))
		, ResourceProvider(MoveTemp(ResourceProvider))
		, ErrorLog(ErrorLog)
	{}

	FTouchVariableManager::~FTouchVariableManager()
	{
		UE_LOG(LogTouchEngine, Verbose, TEXT("Shutting down ~FTouchVariableManager"));
		
		FScopeLock Lock (&TextureUpdateListenersLock);
		for (TPair<FInputTextureUpdateId, TArray<TPromise<FFinishTextureUpdateInfo>>>& Pair : TextureUpdateListeners)
		{
			for (TPromise<FFinishTextureUpdateInfo>& Promise : Pair.Value)
			{
				Promise.SetValue(FFinishTextureUpdateInfo{ ETextureUpdateErrorCode::Cancelled });
			}
		}
		SortedActiveTextureUpdates.Reset();
		TextureUpdateListeners.Reset();
		// ~FTouchResourceProvider will now proceed to cancel all pending tasks.
	}

	void FTouchVariableManager::AllocateLinkedTop(FName ParamName)
	{
		FScopeLock Lock(&TOPLock);
		TOPOutputs.FindOrAdd(ParamName);
	}

	void FTouchVariableManager::UpdateLinkedTOP(FName ParamName, UTexture2D* Texture)
	{
		FScopeLock Lock(&TOPLock);
		TOPOutputs.FindOrAdd(ParamName) = Texture;
	}

	TFuture<FFinishTextureUpdateInfo> FTouchVariableManager::OnFinishAllTextureUpdatesUpTo(const FInputTextureUpdateId TextureUpdateId)
	{
		// Is done already?
		{
			FScopeLock Lock(&ActiveTextureUpdatesLock);
			if (CanFinalizeTextureUpdateTask(TextureUpdateId))
			{
				return MakeFulfilledPromise<FFinishTextureUpdateInfo>(FFinishTextureUpdateInfo{ ETextureUpdateErrorCode::Success }).GetFuture();
			}
		}

		// Needs to wait...
		{
			FScopeLock Lock(&TextureUpdateListenersLock);
			TPromise<FFinishTextureUpdateInfo> Promise;
			TFuture<FFinishTextureUpdateInfo> Future = Promise.GetFuture();
			TextureUpdateListeners.FindOrAdd(TextureUpdateId).Emplace(MoveTemp(Promise));
			return Future;
		}
	}

	FTouchCHOPFull FTouchVariableManager::GetCHOPOutputSingleSample(const FString& Identifier)
	{
		check(IsInGameThread());
		FTouchCHOPFull Full;
		
		const auto AnsiString = StringCast<ANSICHAR>(*Identifier);
		const char* IdentifierAsCStr = AnsiString.Get();

		TELinkInfo* Param = nullptr;
		TEResult Result = TEInstanceLinkGetInfo(TouchEngineInstance, IdentifierAsCStr, &Param);
		if (Result == TEResultSuccess && Param->scope == TEScopeOutput)
		{
			switch (Param->type)
			{
			case TELinkTypeFloatBuffer:
			{

				TEFloatBuffer* Buf = nullptr;
				Result = TEInstanceLinkGetFloatBufferValue(TouchEngineInstance, IdentifierAsCStr, TELinkValueDefault, &Buf);

				if (Result == TEResultSuccess && Buf != nullptr)
				{
					if (!CHOPSingleOutputs.Contains(Identifier))
					{
						CHOPSingleOutputs.Add(Identifier);
					}

					FTouchCHOPSingleSample& Output = CHOPSingleOutputs[Identifier];

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
							ErrorLog->AddResult(TEXT("getCHOPOutputSingleSample(): "), Result);
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
							ErrorLog->AddResult(TEXT("getCHOPOutputSingleSample(): "), Result);
						}
						Full.SampleData.Add(Output);
						TERelease(&Buf);

					}
				}
				break;
			}
			default:
			{
				ErrorLog->AddError(TEXT("getCHOPOutputSingleSample(): ") + Identifier + TEXT(" is not a CHOP Output."));
				break;
			}
			}
		}
		else if (Result != TEResultSuccess)
		{
			ErrorLog->AddResult(TEXT("getCHOPOutputSingleSample(): "), Result);
		}
		else if (Param->scope == TEScopeOutput)
		{
			ErrorLog->AddError(TEXT("getCHOPOutputSingleSample(): ") + Identifier + TEXT(" is not a CHOP Output."));
		}
		TERelease(&Param);

		return Full;
	}

	FTouchCHOPFull FTouchVariableManager::GetCHOPOutputs(const FString& Identifier)
	{
		check(IsInGameThread());
		FTouchCHOPFull c;

		const auto AnsiString = StringCast<ANSICHAR>(*Identifier);
		const char* IdentifierAsCStr = AnsiString.Get();

		TELinkInfo* Param = nullptr;
		TEResult Result = TEInstanceLinkGetInfo(TouchEngineInstance, IdentifierAsCStr, &Param);
		if (Result == TEResultSuccess && Param->scope == TEScopeOutput)
		{
			switch (Param->type)
			{
			case TELinkTypeFloatBuffer:
			{

				TEFloatBuffer* Buf = nullptr;
				Result = TEInstanceLinkGetFloatBufferValue(TouchEngineInstance, IdentifierAsCStr, TELinkValueDefault, &Buf);

				if (Result == TEResultSuccess)
				{
					FTouchCHOPFull& Output = CHOPFullOutputs.FindOrAdd(Identifier);

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
						ErrorLog->AddResult(TEXT("getCHOPOutputs(): "), Result);
					}
					c = Output;
					TERelease(&Buf);
				}
				break;
			}
			default:
			{
				ErrorLog->AddError(TEXT("getCHOPOutputs(): ") + Identifier + TEXT(" is not a CHOP Output."));
				break;
			}
			}
		}
		else if (Result != TEResultSuccess)
		{
			ErrorLog->AddResult(TEXT("getCHOPOutputs(): "), Result);
		}
		else if (Param->scope == TEScopeOutput)
		{
			ErrorLog->AddError(TEXT("getCHOPOutputs(): ") + Identifier + TEXT(" is not a CHOP Output."));
		}
		TERelease(&Param);

		return c;
	}

	UTexture2D* FTouchVariableManager::GetTOPOutput(const FString& Identifier)
	{
		check(IsInGameThread());
		const auto AnsiString = StringCast<ANSICHAR>(*Identifier);
		const char* IdentifierAsCStr = AnsiString.Get();
		
		TELinkInfo* Param = nullptr;
		TEResult Result = TEInstanceLinkGetInfo(TouchEngineInstance, IdentifierAsCStr, &Param);

		if (Result != TEResultSuccess)
		{
			ErrorLog->AddError(FString(TEXT("getTOPOutput(): Unable to find Output named: ")) + Identifier);
			return nullptr;
		}
		
		TERelease(&Param);
		FScopeLock Lock(&TOPLock);

		const FName ParamName(Identifier);
		UTexture2D** Top = TOPOutputs.Find(ParamName);
		return Top
			? *Top
			: nullptr;
	}

	FTouchDATFull FTouchVariableManager::GetTableOutput(const FString& Identifier)
	{
		FTouchDATFull ChannelData;
		const auto AnsiString = StringCast<ANSICHAR>(*Identifier);
		const char* IdentifierAsCStr = AnsiString.Get();

		TELinkInfo* Param = nullptr;
		TEResult Result = TEInstanceLinkGetInfo(TouchEngineInstance, IdentifierAsCStr, &Param);
		if (Result == TEResultSuccess && Param->scope == TEScopeOutput)
		{
			switch (Param->type)
			{
			case TELinkTypeStringData:
				{
					Result = TEInstanceLinkGetTableValue(TouchEngineInstance, IdentifierAsCStr, TELinkValue::TELinkValueCurrent, &ChannelData.ChannelData);
					break;
				}
			default:
				{
					ErrorLog->AddError(TEXT("getTableOutput(): ") + Identifier + TEXT(" is not a table Output."));
					break;
				}
			}
		}
		else if (Result != TEResultSuccess)
		{
			ErrorLog->AddResult(TEXT("getTableOutput(): "), Result);
		}
		else if (Param->scope == TEScopeOutput)
		{
			ErrorLog->AddError(TEXT("getTableOutput(): ") + Identifier + TEXT(" is not a table Output."));
		}
		TERelease(&Param);

		return ChannelData;
	}

	TArray<FString> FTouchVariableManager::GetCHOPChannelNames(const FString& Identifier) const
	{
		if (const FTouchCHOPFull* FullChop = CHOPFullOutputs.Find(Identifier))
		{
			TArray<FString> RetVal;

			for (int32 i = 0; i < FullChop->SampleData.Num(); i++)
			{
				RetVal.Add(FullChop->SampleData[i].ChannelName);
			}
			return RetVal;
		}

		return TArray<FString>();
	}

	TTouchVar<bool> FTouchVariableManager::GetBooleanOutput(const FString& Identifier)
	{
		check(IsInGameThread());
		TTouchVar<bool> c = TTouchVar<bool>();

		const auto AnsiString = StringCast<ANSICHAR>(*Identifier);
		const char* IdentifierAsCStr = AnsiString.Get();

		TELinkInfo* Param = nullptr;
		TEResult Result = TEInstanceLinkGetInfo(TouchEngineInstance, IdentifierAsCStr, &Param);
		if (Result == TEResultSuccess && Param->scope == TEScopeOutput)
		{
			switch (Param->type)
			{
			case TELinkTypeBoolean:
			{
				Result = TEInstanceLinkGetBooleanValue(TouchEngineInstance, IdentifierAsCStr, TELinkValueCurrent, &c.Data);

				if (Result == TEResultSuccess)
				{
					if (!CHOPSingleOutputs.Contains(Identifier))
					{
						CHOPSingleOutputs.Add(Identifier);
					}
				}
				break;
			}
			default:
			{
				ErrorLog->AddError(TEXT("getBooleanOutput(): ") + Identifier + TEXT(" is not a boolean Output."));
				break;
			}
			}
		}
		else if (Result != TEResultSuccess)
		{
			ErrorLog->AddResult(TEXT("getBooleanOutput(): "), Result);
		}
		else if (Param->scope == TEScopeOutput)
		{
			ErrorLog->AddError(TEXT("getBooleanOutput(): ") + Identifier + TEXT(" is not a boolean Output."));
		}
		TERelease(&Param);

		return c;
	}

	TTouchVar<double> FTouchVariableManager::GetDoubleOutput(const FString& Identifier)
	{
		check(IsInGameThread());
		TTouchVar<double> c = TTouchVar<double>();

		const auto AnsiString = StringCast<ANSICHAR>(*Identifier);
		const char* IdentifierAsCStr = AnsiString.Get();

		TELinkInfo* Param = nullptr;
		TEResult Result = TEInstanceLinkGetInfo(TouchEngineInstance, IdentifierAsCStr, &Param);
		if (Result == TEResultSuccess && Param->scope == TEScopeOutput)
		{
			switch (Param->type)
			{
			case TELinkTypeDouble:
			{
				Result = TEInstanceLinkGetDoubleValue(TouchEngineInstance, IdentifierAsCStr, TELinkValueCurrent, &c.Data, 1);

				if (Result == TEResultSuccess)
				{
					if (!CHOPSingleOutputs.Contains(Identifier))
					{
						CHOPSingleOutputs.Add(Identifier);
					}
				}
				break;
			}
			default:
			{
				ErrorLog->AddError(TEXT("getDoubleOutput(): ") + Identifier + TEXT(" is not a double Output."));
				break;
			}
			}
		}
		else if (Result != TEResultSuccess)
		{
			ErrorLog->AddResult(TEXT("getDoubleOutput(): "), Result);
		}
		else if (Param->scope == TEScopeOutput)
		{
			ErrorLog->AddError(TEXT("getDoubleOutput(): ") + Identifier + TEXT(" is not a double Output."));
		}
		TERelease(&Param);

		return c;
	}

	TTouchVar<int32_t> FTouchVariableManager::GetIntegerOutput(const FString& Identifier)
	{
		check(IsInGameThread());
		TTouchVar<int32_t> c = TTouchVar<int32_t>();

		const auto AnsiString = StringCast<ANSICHAR>(*Identifier);
		const char* IdentifierAsCStr = AnsiString.Get();

		TELinkInfo* Param = nullptr;
		TEResult Result = TEInstanceLinkGetInfo(TouchEngineInstance, IdentifierAsCStr, &Param);
		if (Result == TEResultSuccess && Param->scope == TEScopeOutput)
		{
			switch (Param->type)
			{
			case TELinkTypeBoolean:
			{
				Result = TEInstanceLinkGetIntValue(TouchEngineInstance, IdentifierAsCStr, TELinkValueCurrent, &c.Data, 1);

				if (Result == TEResultSuccess)
				{
					if (!CHOPSingleOutputs.Contains(Identifier))
					{
						CHOPSingleOutputs.Add(Identifier);
					}
				}
				break;
			}
			default:
			{
				ErrorLog->AddError(TEXT("getIntegerOutput(): ") + Identifier + TEXT(" is not an integer Output."));
				break;
			}
			}
		}
		else if (Result != TEResultSuccess)
		{
			ErrorLog->AddResult(TEXT("getIntegerOutput(): "), Result);
		}
		else if (Param->scope == TEScopeOutput)
		{
			ErrorLog->AddError(TEXT("getIntegerOutput(): ") + Identifier + TEXT(" is not an integer Output."));
		}
		TERelease(&Param);

		return c;
	}

	TTouchVar<TEString*> FTouchVariableManager::GetStringOutput(const FString& Identifier)
	{
		check(IsInGameThread());
		TTouchVar<TEString*> c = TTouchVar<TEString*>();

		const auto AnsiString = StringCast<ANSICHAR>(*Identifier);
		const char* IdentifierAsCStr = AnsiString.Get();

		TELinkInfo* Param = nullptr;
		TEResult Result = TEInstanceLinkGetInfo(TouchEngineInstance, IdentifierAsCStr, &Param);
		if (Result == TEResultSuccess && Param->scope == TEScopeOutput)
		{
			switch (Param->type)
			{
			case TELinkTypeString:
				{
					Result = TEInstanceLinkGetStringValue(TouchEngineInstance, IdentifierAsCStr, TELinkValueCurrent, &c.Data);

					if (Result == TEResultSuccess)
					{
						if (!CHOPSingleOutputs.Contains(Identifier))
						{
							CHOPSingleOutputs.Add(Identifier);
						}
					}
					break;
				}
			default:
				{
					ErrorLog->AddError(TEXT("getStringOutput(): ") + Identifier + TEXT(" is not a string Output."));
					break;
				}
			}
		}
		else if (Result != TEResultSuccess)
		{
			ErrorLog->AddResult(TEXT("getStringOutput(): "), Result);
		}
		else if (Param->scope == TEScopeOutput)
		{
			ErrorLog->AddError(TEXT("getStringOutput(): ") + Identifier + TEXT(" is not a string Output."));
		}
		TERelease(&Param);

		return c;
	}

	void FTouchVariableManager::SetCHOPInputSingleSample(const FString& Identifier, const FTouchCHOPSingleSample& CHOP)
	{
		check(IsInGameThread());
		if (CHOP.ChannelData.Num() == 0)
		{
			return;
		}

		const auto AnsiString = StringCast<ANSICHAR>(*Identifier);
		const char* IdentifierAsCStr = AnsiString.Get();


		TEResult Result;
		TELinkInfo* Info;
		Result = TEInstanceLinkGetInfo(TouchEngineInstance, IdentifierAsCStr, &Info);

		if (Result != TEResultSuccess)
		{
			ErrorLog->AddResult(FString("setCHOPInputSingleSample(): Unable to get input Info, ") + FString(Identifier) + " may not exist. ", Result);
			return;
		}

		if (Info->type != TELinkTypeFloatBuffer)
		{
			ErrorLog->AddError(FString("setCHOPInputSingleSample(): Input named: ") + FString(Identifier) + " is not a CHOP input.");
			TERelease(&Info);
			return;
		}

		std::vector<const float*> DataPtrs;

		for (int i = 0; i < CHOP.ChannelData.Num(); i++)
		{
			DataPtrs.push_back(&CHOP.ChannelData[i]);
		}

		TEFloatBuffer* Buf = TEFloatBufferCreate(-1.f, CHOP.ChannelData.Num(), 1, nullptr);

		Result = TEFloatBufferSetValues(Buf, DataPtrs.data(), 1);

		if (Result != TEResultSuccess)
		{
			ErrorLog->AddResult(FString("setCHOPInputSingleSample(): Failed to set buffer values: "), Result);
			TERelease(&Info);
			TERelease(&Buf);
			return;
		}
		Result = TEInstanceLinkAddFloatBuffer(TouchEngineInstance, IdentifierAsCStr, Buf);

		if (Result != TEResultSuccess)
		{
			ErrorLog->AddResult(FString("setCHOPInputSingleSample(): Unable to append buffer values: "), Result);
			TERelease(&Info);
			TERelease(&Buf);
			return;
		}

		TERelease(&Info);
		TERelease(&Buf);
	}

	void FTouchVariableManager::SetCHOPInput(const FString& Identifier, const FTouchCHOPFull& CHOP)
	{
		check(IsInGameThread());
	}

	void FTouchVariableManager::SetTOPInput(const FString& Identifier, UTexture* Texture, bool bReuseExistingTexture)
	{
		check(IsInGameThread());
		
		// Fast path
		if (Texture == nullptr)
		{
			const auto AnsiString = StringCast<ANSICHAR>(*Identifier);
			const char* IdentifierAsCStr = AnsiString.Get();
			TEInstanceLinkSetTextureValue(TouchEngineInstance, IdentifierAsCStr, Texture, ResourceProvider->GetContext());
			return;
		}
		
		const int64 TextureUpdateId = NextTextureUpdateId++;
		const FTextureInputUpdateInfo UpdateInfo { *Identifier, TextureUpdateId };
		{
			FScopeLock Lock(&ActiveTextureUpdatesLock);
			SortedActiveTextureUpdates.Add({ TextureUpdateId });
		}
		
		ResourceProvider->ExportTextureToTouchEngine({ TouchEngineInstance, *Identifier, bReuseExistingTexture, Texture })
			.Next([WeakThis = TWeakPtr<FTouchVariableManager>(SharedThis(this)), UpdateInfo](FTouchExportResult Result)
			{
				TSharedPtr<FTouchVariableManager> ThisPin = WeakThis.Pin();
				if (!ThisPin || Result.ErrorCode == ETouchExportErrorCode::Cancelled)
				{
					return;
				}
				
				// The event needs to be executed after all work is done
				ON_SCOPE_EXIT
				{
					ThisPin->OnFinishInputTextureUpdate(UpdateInfo);
				};
				
				switch (Result.ErrorCode)
				{
				case ETouchExportErrorCode::UnsupportedPixelFormat:
					ThisPin->ErrorLog->AddError(TEXT("setTOPInput(): Unsupported pixel format for texture input. Compressed textures are not supported."));
					return;
				case ETouchExportErrorCode::UnsupportedTextureObject:
					ThisPin->ErrorLog->AddError(TEXT("setTOPInput(): Unsupported Unreal texture object."));
					return;
				case ETouchExportErrorCode::InternalGraphicsDriverError:
					ThisPin->ErrorLog->AddError(TEXT("setTOPInput(): Internal D3D12 error."));
					return;

				case ETouchExportErrorCode::FailedTextureTransfer:
					ThisPin->ErrorLog->AddError(TEXT("setTOPInput(): Failed to transfer texture to TE (TEInstanceAddTextureTransfer error)."));
					return;
					
				case ETouchExportErrorCode::UnsupportedOperation:
					ThisPin->ErrorLog->AddError(TEXT("setTOPInput(): This plugin does not implement functionality for input textures right now."));
					return;

				case ETouchExportErrorCode::UnknownFailure:
					ThisPin->ErrorLog->AddError(TEXT("setTOPInput(): Unknown failure condition - investigate."));
					return;
				
				default:
					static_assert(static_cast<int32>(ETouchExportErrorCode::Count) == 8, "Update this switch");
					break;
				}

				const auto AnsiString = StringCast<ANSICHAR>(*UpdateInfo.Texture.ToString());
				const char* IdentifierAsCStr = AnsiString.Get();
				TETexture* Texture = Result.ErrorCode == ETouchExportErrorCode::Success
					? Result.Texture
					: nullptr;
				TEInstanceLinkSetTextureValue(ThisPin->TouchEngineInstance, IdentifierAsCStr, Texture, ThisPin->ResourceProvider->GetContext());
			});
	}

	void FTouchVariableManager::SetBooleanInput(const FString& Identifier, TTouchVar<bool>& Op)
	{
		check(IsInGameThread());
		
		const auto AnsiString = StringCast<ANSICHAR>(*Identifier);
		const char* IdentifierAsCStr = AnsiString.Get();

		TEResult Result;
		TELinkInfo* Info;
		Result = TEInstanceLinkGetInfo(TouchEngineInstance, IdentifierAsCStr, &Info);

		if (Result != TEResultSuccess)
		{
			ErrorLog->AddResult(FString("setBooleanInput(): Unable to get input Info, ") + FString(Identifier) + " may not exist. ", Result);
			return;
		}

		if (Info->type != TELinkTypeBoolean)
		{
			ErrorLog->AddError(FString("setBooleanInput(): Input named: ") + FString(Identifier) + " is not a boolean input.");
			TERelease(&Info);
			return;
		}

		Result = TEInstanceLinkSetBooleanValue(TouchEngineInstance, IdentifierAsCStr, Op.Data);

		if (Result != TEResultSuccess)
		{
			ErrorLog->AddResult(FString("setBooleanInput(): Unable to set boolean value: "), Result);
			TERelease(&Info);
			return;
		}

		TERelease(&Info);
	}

	void FTouchVariableManager::SetDoubleInput(const FString& Identifier, TTouchVar<TArray<double>>& Op)
	{
		check(IsInGameThread());
		const auto AnsiString = StringCast<ANSICHAR>(*Identifier);
		const char* IdentifierAsCStr = AnsiString.Get();

		TEResult Result;
		TELinkInfo* Info;
		Result = TEInstanceLinkGetInfo(TouchEngineInstance, IdentifierAsCStr, &Info);

		if (Result != TEResultSuccess)
		{
			ErrorLog->AddResult(FString("setDoubleInput(): Unable to get input Info, ") + FString(Identifier) + " may not exist. ", Result);
			return;
		}

		if (Info->type != TELinkTypeDouble)
		{
			ErrorLog->AddError(FString("setDoubleInput(): Input named: ") + FString(Identifier) + " is not a double input.");
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

				Result = TEInstanceLinkSetDoubleValue(TouchEngineInstance, IdentifierAsCStr, buffer.GetData(), Info->count);
			}
			else
			{
				ErrorLog->AddError(FString("setDoubleInput(): Unable to set double value: count mismatch"));
				TERelease(&Info);
				return;
			}
		}
		else
		{
			Result = TEInstanceLinkSetDoubleValue(TouchEngineInstance, IdentifierAsCStr, Op.Data.GetData(), Op.Data.Num());
		}

		if (Result != TEResultSuccess)
		{
			ErrorLog->AddResult(FString("setDoubleInput(): Unable to set double value: "), Result);
			TERelease(&Info);
			return;
		}

		TERelease(&Info);
	}

	void FTouchVariableManager::SetIntegerInput(const FString& Identifier, TTouchVar<TArray<int32_t>>& Op)
	{
		check(IsInGameThread());
		const auto AnsiString = StringCast<ANSICHAR>(*Identifier);
		const char* IdentifierAsCStr = AnsiString.Get();

		TEResult Result;
		TELinkInfo* Info;
		Result = TEInstanceLinkGetInfo(TouchEngineInstance, IdentifierAsCStr, &Info);

		if (Result != TEResultSuccess)
		{
			ErrorLog->AddResult(FString("setIntegerInput(): Unable to get input Info, ") + FString(Identifier) + " may not exist. ", Result);
			return;
		}

		if (Info->type != TELinkTypeInt)
		{
			ErrorLog->AddError(FString("setIntegerInput(): Input named: ") + FString(Identifier) + " is not an integer input.");
			TERelease(&Info);
			return;
		}

		Result = TEInstanceLinkSetIntValue(TouchEngineInstance, IdentifierAsCStr, Op.Data.GetData(), Op.Data.Num());

		if (Result != TEResultSuccess)
		{
			ErrorLog->AddResult(FString("setIntegerInput(): Unable to set integer value: "), Result);
			TERelease(&Info);
			return;
		}

		TERelease(&Info);
	}

	void FTouchVariableManager::SetStringInput(const FString& Identifier, TTouchVar<const char*>& Op)
	{
		check(IsInGameThread());
		const auto AnsiString = StringCast<ANSICHAR>(*Identifier);
		const char* IdentifierAsCStr = AnsiString.Get();

		TEResult Result;
		TELinkInfo* Info;
		Result = TEInstanceLinkGetInfo(TouchEngineInstance, IdentifierAsCStr, &Info);

		if (Result != TEResultSuccess)
		{
			ErrorLog->AddResult(FString("setStringInput(): Unable to get input Info, ") + FString(Identifier) + " may not exist. ", Result);
			return;
		}

		if (Info->type == TELinkTypeString)
		{
			Result = TEInstanceLinkSetStringValue(TouchEngineInstance, IdentifierAsCStr, Op.Data);
		}
		else if (Info->type == TELinkTypeStringData)
		{
			TETable* Table = TETableCreate();
			TETableResize(Table, 1, 1);
			TETableSetStringValue(Table, 0, 0, Op.Data);

			Result = TEInstanceLinkSetTableValue(TouchEngineInstance, IdentifierAsCStr, Table);
			TERelease(&Table);
		}
		else
		{
			ErrorLog->AddError(FString("setStringInput(): Input named: ") + FString(Identifier) + " is not a string input.");
			TERelease(&Info);
			return;
		}


		if (Result != TEResultSuccess)
		{
			ErrorLog->AddResult(FString("setStringInput(): Unable to set string value: "), Result);
			TERelease(&Info);
			return;
		}

		TERelease(&Info);
	}

	void FTouchVariableManager::SetTableInput(const FString& Identifier, FTouchDATFull& Op)
	{
		check(IsInGameThread());
		const auto AnsiString = StringCast<ANSICHAR>(*Identifier);
		const char* IdentifierAsCStr = AnsiString.Get();

		TELinkInfo* Info;
		TEResult Result = TEInstanceLinkGetInfo(TouchEngineInstance, IdentifierAsCStr, &Info);
		if (Result != TEResultSuccess)
		{
			ErrorLog->AddResult(FString("setTableInput(): Unable to get input Info, ") + FString(Identifier) + " may not exist. ", Result);
			return;
		}

		if (Info->type == TELinkTypeString)
		{
			const char* string = TETableGetStringValue(Op.ChannelData, 0, 0);
			Result = TEInstanceLinkSetStringValue(TouchEngineInstance, IdentifierAsCStr, string);
		}
		else if (Info->type == TELinkTypeStringData)
		{
			Result = TEInstanceLinkSetTableValue(TouchEngineInstance, IdentifierAsCStr, Op.ChannelData);
		}
		else
		{
			ErrorLog->AddError(FString("setTableInput(): Input named: ") + FString(Identifier) + " is not a table input.");
			TERelease(&Info);
			return;
		}


		if (Result != TEResultSuccess)
		{
			ErrorLog->AddResult(FString("setTableInput(): Unable to set table value: "), Result);
			TERelease(&Info);
			return;
		}

		TERelease(&Info);
	}
	
	void FTouchVariableManager::OnFinishInputTextureUpdate(const FTextureInputUpdateInfo& UpdateInfo)
	{
		const FInputTextureUpdateId TaskId = UpdateInfo.TextureUpdateId;

		TArray<FInputTextureUpdateId> TexturesUpdatesToMarkCompleted;
		{
			FScopeLock Lock(&ActiveTextureUpdatesLock);
			const bool bAreAllPreviousUpdatesDone = CanFinalizeTextureUpdateTask(TaskId, true);
			
			const int32 Index = SortedActiveTextureUpdates.IndexOfByPredicate([TaskId](const FInputTextureUpdateTask& Task){ return Task.TaskId == TaskId; });
			check(Index != INDEX_NONE);
			if (bAreAllPreviousUpdatesDone)
			{
				TexturesUpdatesToMarkCompleted.Add(TaskId);
				SortedActiveTextureUpdates.RemoveAt(Index);
			}
			else
			{
				FInputTextureUpdateTask& Task = SortedActiveTextureUpdates[Index];
				Task.bIsAwaitingFinalisation = true;
				HighestTaskIdAwaitingFinalisation = FMath::Max(Task.TaskId, HighestTaskIdAwaitingFinalisation);
			}

			CollectAllDoneTexturesPendingFinalization(TexturesUpdatesToMarkCompleted);
		}
		
		TArray<TPromise<FFinishTextureUpdateInfo>> PromisesToExecute;
		{
			FScopeLock Lock(&TextureUpdateListenersLock);
			PromisesToExecute = RemoveAndGetListenersFor(TexturesUpdatesToMarkCompleted);
		}

		// Promises should be executed outside of the lock in case they themselves try to acquire it (deadlock)
		for (TPromise<FFinishTextureUpdateInfo>& Promise : PromisesToExecute)
		{
			Promise.SetValue(FFinishTextureUpdateInfo{ ETextureUpdateErrorCode::Success });
		}
	}

	bool FTouchVariableManager::CanFinalizeTextureUpdateTask(const FInputTextureUpdateId UpdateId, bool bJustFinishedTask) const
	{
		// Since SortedActiveTextureUpdates is sorted, if the first element is bigger everything after it is also bigger. 
		return SortedActiveTextureUpdates.Num() <= 0
			|| ((bJustFinishedTask && SortedActiveTextureUpdates[0].TaskId >= UpdateId)
				|| (!bJustFinishedTask && SortedActiveTextureUpdates[0].TaskId > UpdateId));
	}

	void FTouchVariableManager::CollectAllDoneTexturesPendingFinalization(TArray<FInputTextureUpdateId>& Result) const
	{
		// All tasks until the first bIsAwaitingFinalisation that has bIsAwaitingFinalisation == false are done.
		// Additionally we know we can stop at HighestTaskIdAwaitingFinalisation because that is the largest task with bIsAwaitingFinalisation == true.
		for (int32 i = 0; i < SortedActiveTextureUpdates.Num() && SortedActiveTextureUpdates[i].TaskId <= HighestTaskIdAwaitingFinalisation; ++i)
		{
			if (!SortedActiveTextureUpdates[i].bIsAwaitingFinalisation)
			{
				break;
			}

			Result.Add(SortedActiveTextureUpdates[i].TaskId);
		}
	}

	TArray<TPromise<FFinishTextureUpdateInfo>> FTouchVariableManager::RemoveAndGetListenersFor(const TArray<FInputTextureUpdateId>& UpdateIds)
	{
		TArray<TPromise<FFinishTextureUpdateInfo>> Result;
		for (const FInputTextureUpdateId& UpdateId : UpdateIds)
		{
			if (TArray<TPromise<FFinishTextureUpdateInfo>>* TextureUpdateData = TextureUpdateListeners.Find(UpdateId))
			{
				for (TPromise<FFinishTextureUpdateInfo>& Promise : *TextureUpdateData)
				{
					Result.Emplace(MoveTemp(Promise));
				}
			}
			
			TextureUpdateListeners.Remove(UpdateId);
		}
		return Result;
	}
}
