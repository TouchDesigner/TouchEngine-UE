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

#include <string>

#include "Logging.h"
#include "TouchEngineDynamicVariableStruct.h"
#include "Engine/Util/TouchErrorLog.h"
#include "Rendering/TouchResourceProvider.h"
#include "Rendering/Exporting/TouchExportParams.h"

#include <vector> //todo: there shouldn't be the need to use vector

#include "Engine/TEDebug.h"
#include "Util/TouchHelpers.h"
#include "Engine/Texture.h"

namespace UE::TouchEngine
{
	FTouchVariableManager::FTouchVariableManager(
		TouchObject<TEInstance> TouchEngineInstance,
		TSharedPtr<FTouchResourceProvider> ResourceProvider,
		const TSharedPtr<FTouchErrorLog>& ErrorLog
	)
		: TouchEngineInstance(MoveTemp(TouchEngineInstance))
		  , ResourceProvider(MoveTemp(ResourceProvider))
		  , ErrorLog(ErrorLog)
	{
	}

	FTouchVariableManager::~FTouchVariableManager()
	{
		UE_LOG(LogTouchEngine, Verbose, TEXT("Shutting down ~FTouchVariableManager"));
		// ~FTouchResourceProvider will now proceed to cancel all pending tasks.
	}

	void FTouchVariableManager::AllocateLinkedTop(const FName ParamName)
	{
		FScopeLock Lock(&TOPOutputsLock);
		TOPOutputs.FindOrAdd(ParamName);
	}

	UTexture2D* FTouchVariableManager::UpdateLinkedTOP(const FName ParamName, UTexture2D* Texture)
	{
		FScopeLock Lock(&TOPOutputsLock);
		UTexture2D* ExistingTextureToBePooled = nullptr;
		if (UTexture2D** ExistingTexturePtr = TOPOutputs.Find(ParamName))
		{
			ExistingTextureToBePooled = *ExistingTexturePtr;
		}
		TOPOutputs.FindOrAdd(ParamName) = Texture;
		return ExistingTextureToBePooled;
	}
	
	FTouchEngineCHOP FTouchVariableManager::GetCHOPOutputSingleSample(const FString& Identifier)
	{
		check(IsInGameThread());
		FTouchEngineCHOP Full;

		const auto AnsiString = StringCast<ANSICHAR>(*Identifier);
		const char* IdentifierAsCStr = AnsiString.Get();

		TouchObject<TELinkInfo> Param = nullptr;
		TEResult Result = TEInstanceLinkGetInfo(TouchEngineInstance, IdentifierAsCStr, Param.take());

		if (Result == TEResultSuccess && Param->scope == TEScopeOutput)
		{
			switch (Param->type)
			{
			case TELinkTypeFloatBuffer:
				{
					TouchObject<TEFloatBuffer> Buf;
					Result = TEInstanceLinkGetFloatBufferValue(TouchEngineInstance, IdentifierAsCStr, TELinkValueDefault, Buf.take());

					if (Result == TEResultSuccess && Buf != nullptr)
					{
						if (!CHOPChannelOutputs.Contains(Identifier))
						{
							CHOPChannelOutputs.Add(Identifier);
						}

						FTouchEngineCHOPChannel& Output = CHOPChannelOutputs[Identifier];

						const int32 ChannelCount = TEFloatBufferGetChannelCount(Buf);
						const int64 MaxSamples = TEFloatBufferGetValueCount(Buf);

						const int64 Length = MaxSamples;

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
										Full.Channels.Add(FTouchEngineCHOPChannel());
										Full.Channels[i].Name = ChannelNames[i];

										for (int j = 0; j < Length; j++)
										{
											Full.Channels[i].Values.Add(Channels[i][j]);
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
									Output.Values.SetNum(ChannelCount);

									for (int32 i = 0; i < ChannelCount; i++)
									{
										Output.Values[i] = Channels[i][Length - 1];
									}
									Output.Name = ChannelNames[0];
								}
							}
							// Suppress internal errors for now, some superfluous ones are occuring currently
							else if (Result != TEResultInternalError)
							{
								ErrorLog->AddResult(TEXT("getCHOPOutputSingleSample(): "), Result);
							}
							Full.Channels.Add(Output);
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

		return Full;
	}

	FTouchEngineCHOP FTouchVariableManager::GetCHOPOutput(const FString& Identifier)
	{
		check(IsInGameThread());
		FTouchEngineCHOP Chop;

		const auto AnsiString = StringCast<ANSICHAR>(*Identifier);
		const char* IdentifierAsCStr = AnsiString.Get();

		TouchObject<TELinkInfo> Param;
		TEResult Result = TEInstanceLinkGetInfo(TouchEngineInstance, IdentifierAsCStr, Param.take());

		if (Result == TEResultSuccess && Param->scope == TEScopeOutput)
		{
			switch (Param->type)
			{
			case TELinkTypeFloatBuffer:
				{
					TouchObject<TEFloatBuffer> Buf = nullptr;
					Result = TEInstanceLinkGetFloatBufferValue(TouchEngineInstance, IdentifierAsCStr, TELinkValueDefault, Buf.take());

					if (Result == TEResultSuccess)
					{
						FTouchEngineCHOP& Output = CHOPOutputs.FindOrAdd(Identifier);

						const int32 ChannelCount = TEFloatBufferGetChannelCount(Buf);
						const int64 MaxSamples = TEFloatBufferGetValueCount(Buf);
						const float* const* Channels = TEFloatBufferGetValues(Buf);
						const char* const* ChannelNames = TEFloatBufferGetChannelNames(Buf);

						if (TEFloatBufferIsTimeDependent(Buf))
						{
						}

						// Use the channel data here
						if (MaxSamples > 0 && ChannelCount > 0)
						{
							Output.Channels.SetNum(ChannelCount);
							for (int i = 0; i < ChannelCount; i++)
							{
								Output.Channels[i].Values.SetNum(MaxSamples);
								Output.Channels[i].Name = ChannelNames[i];
								for (int j = 0; j < MaxSamples; j++)
								{
									Output.Channels[i].Values[j] = Channels[i][j];
								}
							}
						}
						Chop = Output;
					}
					// Suppress internal errors for now, some superfluous ones are occuring currently
					else if (Result != TEResultInternalError)
					{
						ErrorLog->AddResult(TEXT("getCHOPOutputs(): "), Result);
					}
					
					break;
				} //~case TELinkTypeFloatBuffer
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

		return Chop;
	}

	UTexture2D* FTouchVariableManager::GetTOPOutput(const FString& Identifier)
	{
		check(IsInGameThread());
		const auto AnsiString = StringCast<ANSICHAR>(*Identifier);
		const char* IdentifierAsCStr = AnsiString.Get();

		TouchObject<TELinkInfo> Param;
		const TEResult Result = TEInstanceLinkGetInfo(TouchEngineInstance, IdentifierAsCStr, Param.take());
		

		if (Result != TEResultSuccess)
		{
			ErrorLog->AddError(FString(TEXT("getTOPOutput(): Unable to find Output named: ")) + Identifier);
			return nullptr;
		}

		FScopeLock Lock(&TOPOutputsLock);

		const FName ParamName(Identifier);
		UTexture2D** Top = TOPOutputs.Find(ParamName);
		return Top
			       ? *Top
			       : nullptr;
	}

	FTouchDATFull FTouchVariableManager::GetTableOutput(const FString& Identifier) const
	{
		FTouchDATFull ChannelData;
		const auto AnsiString = StringCast<ANSICHAR>(*Identifier);
		const char* IdentifierAsCStr = AnsiString.Get();

		TouchObject<TELinkInfo> Param;
		TEResult Result = TEInstanceLinkGetInfo(TouchEngineInstance, IdentifierAsCStr, Param.take());
		if (Result == TEResultSuccess && Param->scope == TEScopeOutput)
		{
			switch (Param->type)
			{
			case TELinkTypeStringData:
				{
					Result = TEInstanceLinkGetTableValue(TouchEngineInstance, IdentifierAsCStr, TELinkValue::TELinkValueCurrent, ChannelData.ChannelData.take());
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

		return ChannelData;
	}

	TArray<FString> FTouchVariableManager::GetCHOPChannelNames(const FString& Identifier) const
	{
		if (const FTouchEngineCHOP* FullChop = CHOPOutputs.Find(Identifier))
		{
			TArray<FString> RetVal;

			for (int32 i = 0; i < FullChop->Channels.Num(); i++)
			{
				RetVal.Add(FullChop->Channels[i].Name);
			}
			return RetVal;
		}

		return TArray<FString>();
	}

	bool FTouchVariableManager::GetBooleanOutput(const FString& Identifier)
	{
		check(IsInGameThread());
		bool c = {};

		const auto AnsiString = StringCast<ANSICHAR>(*Identifier);
		const char* IdentifierAsCStr = AnsiString.Get();

		TouchObject<TELinkInfo> Param;
		TEResult Result = TEInstanceLinkGetInfo(TouchEngineInstance, IdentifierAsCStr, Param.take());

		if (Result == TEResultSuccess && Param->scope == TEScopeOutput)
		{
			switch (Param->type)
			{
			case TELinkTypeBoolean:
				{
					Result = TEInstanceLinkGetBooleanValue(TouchEngineInstance, IdentifierAsCStr, TELinkValueCurrent, &c);

					if (Result == TEResultSuccess)
					{
						if (!CHOPChannelOutputs.Contains(Identifier))
						{
							CHOPChannelOutputs.Add(Identifier);
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

		return c;
	}

	double FTouchVariableManager::GetDoubleOutput(const FString& Identifier)
	{
		check(IsInGameThread());
		double c = {};

		const auto AnsiString = StringCast<ANSICHAR>(*Identifier);
		const char* IdentifierAsCStr = AnsiString.Get();

		TouchObject<TELinkInfo> Param;
		TEResult Result = TEInstanceLinkGetInfo(TouchEngineInstance, IdentifierAsCStr, Param.take());

		if (Result == TEResultSuccess && Param->scope == TEScopeOutput)
		{
			switch (Param->type)
			{
			case TELinkTypeDouble:
				{
					Result = TEInstanceLinkGetDoubleValue(TouchEngineInstance, IdentifierAsCStr, TELinkValueCurrent, &c, 1);

					if (Result == TEResultSuccess)
					{
						if (!CHOPChannelOutputs.Contains(Identifier))
						{
							CHOPChannelOutputs.Add(Identifier);
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

		return c;
	}

	int32_t FTouchVariableManager::GetIntegerOutput(const FString& Identifier)
	{
		check(IsInGameThread());
		int32_t c = {};

		const auto AnsiString = StringCast<ANSICHAR>(*Identifier);
		const char* IdentifierAsCStr = AnsiString.Get();

		TouchObject<TELinkInfo> Param;
		TEResult Result = TEInstanceLinkGetInfo(TouchEngineInstance, IdentifierAsCStr, Param.take());

		if (Result == TEResultSuccess && Param->scope == TEScopeOutput)
		{
			switch (Param->type)
			{
			case TELinkTypeBoolean:
				{
					Result = TEInstanceLinkGetIntValue(TouchEngineInstance, IdentifierAsCStr, TELinkValueCurrent, &c, 1);

					if (Result == TEResultSuccess)
					{
						if (!CHOPChannelOutputs.Contains(Identifier))
						{
							CHOPChannelOutputs.Add(Identifier);
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

		return c;
	}

	TouchObject<TEString> FTouchVariableManager::GetStringOutput(const FString& Identifier)
	{
		check(IsInGameThread());
		TouchObject<TEString> c = {};

		const auto AnsiString = StringCast<ANSICHAR>(*Identifier);
		const char* IdentifierAsCStr = AnsiString.Get();

		TouchObject<TELinkInfo> Param;
		TEResult Result = TEInstanceLinkGetInfo(TouchEngineInstance, IdentifierAsCStr, Param.take());

		if (Result == TEResultSuccess && Param->scope == TEScopeOutput)
		{
			switch (Param->type)
			{
			case TELinkTypeString:
				{
					Result = TEInstanceLinkGetStringValue(TouchEngineInstance, IdentifierAsCStr, TELinkValueCurrent, c.take());

					if (Result == TEResultSuccess)
					{
						if (!CHOPChannelOutputs.Contains(Identifier))
						{
							CHOPChannelOutputs.Add(Identifier);
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

		return c;
	}
	

	void FTouchVariableManager::SetCHOPInputSingleSample(const FString& Identifier, const FTouchEngineCHOPChannel& CHOP)
	{
		check(IsInGameThread());
		if (CHOP.Values.Num() == 0)
		{
			return;
		}

		const auto AnsiString = StringCast<ANSICHAR>(*Identifier);
		const char* IdentifierAsCStr = AnsiString.Get();

		TouchObject<TELinkInfo> Info;
		TEResult Result = TEInstanceLinkGetInfo(TouchEngineInstance, IdentifierAsCStr, Info.take());

		if (Result != TEResultSuccess)
		{
			ErrorLog->AddResult(FString("setCHOPInputSingleSample(): Unable to get input Info, ") + FString(Identifier) + " may not exist. ", Result);
			return;
		}

		if (Info->type != TELinkTypeFloatBuffer)
		{
			ErrorLog->AddError(FString("setCHOPInputSingleSample(): Input named: ") + FString(Identifier) + " is not a CHOP input.");
			return;
		}

		std::vector<const float*> DataPointers;

		for (int i = 0; i < CHOP.Values.Num(); i++)
		{
			DataPointers.push_back(&CHOP.Values[i]);
		}

		TouchObject<TEFloatBuffer> Buf;
		Buf.take(TEFloatBufferCreate(-1.f, CHOP.Values.Num(), 1, nullptr));

		Result = TEFloatBufferSetValues(Buf, DataPointers.data(), 1);

		if (Result != TEResultSuccess)
		{
			ErrorLog->AddResult(FString("setCHOPInputSingleSample(): Failed to set buffer values: "), Result);
			return;
		}
		Result = TEInstanceLinkAddFloatBuffer(TouchEngineInstance, IdentifierAsCStr, Buf);

		if (Result != TEResultSuccess)
		{
			ErrorLog->AddResult(FString("setCHOPInputSingleSample(): Unable to append buffer values: "), Result);
			return;
		}
	}

	void FTouchVariableManager::SetCHOPInput(const FString& Identifier, const FTouchEngineCHOP& CHOP)
	{
		check(IsInGameThread());
		if (!CHOP.IsValid()) //todo: look at merging with the loop below
		{
			ErrorLog->AddError(FString::Printf(TEXT("SetCHOPInput(): The CHOP given for the input `%s` is not valid "), *Identifier));
			return;
		}

		const auto AnsiString = StringCast<ANSICHAR>(*Identifier);
		const char* IdentifierAsCStr = AnsiString.Get();


		TouchObject<TELinkInfo> Info;
		TEResult Result = TEInstanceLinkGetInfo(TouchEngineInstance, IdentifierAsCStr, Info.take());

		if (Result != TEResultSuccess)
		{
			ErrorLog->AddResult(FString(TEXT("SetCHOPInput(): Unable to get input Info, ")) + FString(Identifier) + TEXT(" may not exist. "), Result);
			return;
		}

		if (Info->type != TELinkTypeFloatBuffer)
		{
			ErrorLog->AddError(FString(TEXT("setCHOPInputSingleSample(): Input named: ")) + FString(Identifier) + TEXT(" is not a CHOP input."));
			return;
		}

		const int32 Capacity = CHOP.Channels.IsEmpty() ? 0 : CHOP.Channels[0].Values.Num();

		bool bAreAllChannelNamesEmpty = true;
		TArray<std::string> ChannelNamesANSI; // Store as temporary string to keep a reference until the buffer is created
		TArray<const char*> ChannelNames;
		std::vector<const float*> DataPointers;
		for (int i = 0; i < CHOP.Channels.Num(); i++)
		{
			const FString& ChannelName = CHOP.Channels[i].Name;
			bAreAllChannelNamesEmpty &= ChannelName.IsEmpty();
			
			auto ChannelNameANSI = StringCast<ANSICHAR>(*ChannelName);
			std::string ChannelNameString(ChannelNameANSI.Get());

			const auto Index = ChannelNamesANSI.Emplace(ChannelNameString);
			ChannelNames.Emplace(ChannelName.IsEmpty() ? nullptr : ChannelNamesANSI[Index].c_str());

			DataPointers.push_back(CHOP.Channels[i].Values.GetData());
		}

		const TouchObject<TEFloatBuffer> Buffer = TouchObject<TEFloatBuffer>::make_take(TEFloatBufferCreate(-1.f, CHOP.Channels.Num(), Capacity, bAreAllChannelNamesEmpty ? nullptr : ChannelNames.GetData()));
		Result = TEFloatBufferSetValues(Buffer, DataPointers.data(), Capacity);

		if (Result != TEResultSuccess)
		{
			ErrorLog->AddResult(FString(TEXT("setCHOPInputSingleSample(): Failed to set buffer values: ")), Result);
			return;
		}
		Result = TEInstanceLinkAddFloatBuffer(TouchEngineInstance, IdentifierAsCStr, Buffer);
		UE_LOG(LogTouchEngine, Verbose, TEXT("  TEInstanceLinkAddFloatBuffer[%s] :  %s [CHOP]"), *GetCurrentThreadStr(), *Identifier);

		if (Result != TEResultSuccess)
		{
			ErrorLog->AddResult(FString(TEXT("setCHOPInputSingleSample(): Unable to append buffer values: ")), Result);
			return;
		}
	}

	void FTouchVariableManager::SetTOPInput(const FString& Identifier, UTexture* Texture, const FTouchEngineInputFrameData& FrameData)
	{
		check(IsInGameThread());

		// Fast path
		const auto AnsiString = StringCast<ANSICHAR>(*Identifier);
		const char* IdentifierAsCStr = AnsiString.Get();
		if (!Texture)
		{
			TEInstanceLinkSetTextureValue(TouchEngineInstance, IdentifierAsCStr, Texture, ResourceProvider->GetContext());
			return;
		}
		
		const FTouchExportParameters ExportParams {TouchEngineInstance, *Identifier, Texture, FrameData};
		const TouchObject<TETexture> ExportedTexture = ResourceProvider->ExportTextureToTouchEngine_AnyThread(ExportParams);
		{
			const TEResult SetTextureResult = TEInstanceLinkSetTextureValue(TouchEngineInstance, IdentifierAsCStr, ExportedTexture, ResourceProvider->GetContext());
			UE_LOG(LogTouchEngineTECalls, Log, TEXT(" [FTouchVariableManager::SetTOPInput[%s]] `TEInstanceLinkSetTextureValue` returned `%s`. %s"),
				*GetCurrentThreadStr(), *TEResultToString(SetTextureResult), *ExportParams.GetDebugDescription());
			TEInstanceLinkSetInterest(TouchEngineInstance, IdentifierAsCStr, TELinkInterestNoValues);
		}

		{
			FScopeLock Lock(&TOPInputsLock);

			const FName ParamName(Identifier);
			const TouchObject<TETexture>* Top = TOPInputs.Find(ParamName);
			if (Top)
			{
				if (Top->get() != ExportedTexture.get())
				{
					TOPInputs.Add(ParamName, ExportedTexture);
				}
			}
			else
			{
				TOPInputs.Add(ParamName, ExportedTexture);
			}
		}
	}

	void FTouchVariableManager::SetBooleanInput(const FString& Identifier, const bool& Op)
	{
		check(IsInGameThread());

		const auto AnsiString = StringCast<ANSICHAR>(*Identifier);
		const char* IdentifierAsCStr = AnsiString.Get();

		TouchObject<TELinkInfo> Info;
		TEResult Result = TEInstanceLinkGetInfo(TouchEngineInstance, IdentifierAsCStr, Info.take());
		
		if (Result != TEResultSuccess)
		{
			ErrorLog->AddResult(FString("setBooleanInput(): Unable to get input Info, ") + FString(Identifier) + " may not exist. ", Result);
			return;
		}

		if (Info->type != TELinkTypeBoolean)
		{
			ErrorLog->AddError(FString("setBooleanInput(): Input named: ") + FString(Identifier) + " is not a boolean input.");
			return;
		}

		Result = TEInstanceLinkSetBooleanValue(TouchEngineInstance, IdentifierAsCStr, Op);
		UE_LOG(LogTouchEngine, Verbose, TEXT("  TEInstanceLinkSetBooleanValue[%s]:  %s"), *GetCurrentThreadStr(), *Identifier);

		if (Result != TEResultSuccess)
		{
			ErrorLog->AddResult(FString("setBooleanInput(): Unable to set boolean value: "), Result);
			return;
		}
	}

	void FTouchVariableManager::SetDoubleInput(const FString& Identifier, const TArray<double>& Op)
	{
		check(IsInGameThread());
		const auto AnsiString = StringCast<ANSICHAR>(*Identifier);
		const char* IdentifierAsCStr = AnsiString.Get();

		TouchObject<TELinkInfo> Info;
		TEResult Result = TEInstanceLinkGetInfo(TouchEngineInstance, IdentifierAsCStr, Info.take());

		if (Result != TEResultSuccess)
		{
			ErrorLog->AddResult(FString("setDoubleInput(): Unable to get input Info, ") + FString(Identifier) + " may not exist. ", Result);
			return;
		}

		if (Info->type != TELinkTypeDouble)
		{
			ErrorLog->AddError(FString("setDoubleInput(): Input named: ") + FString(Identifier) + " is not a double input.");
			return;
		}

		if (Op.Num() != Info->count)
		{
			if (Op.Num() > Info->count)
			{
				TArray<double> buffer;

				for (int i = 0; i < Info->count; i++)
				{
					buffer.Add(Op[i]);
				}

				Result = TEInstanceLinkSetDoubleValue(TouchEngineInstance, IdentifierAsCStr, buffer.GetData(), Info->count);
				UE_LOG(LogTouchEngine, Verbose, TEXT("  TEInstanceLinkSetDoubleValue[%s]:  %s"), *GetCurrentThreadStr(), *Identifier);
			}
			else
			{
				ErrorLog->AddError(FString("setDoubleInput(): Unable to set double value: count mismatch"));
				return;
			}
		}
		else
		{
			Result = TEInstanceLinkSetDoubleValue(TouchEngineInstance, IdentifierAsCStr, Op.GetData(), Op.Num());
			UE_LOG(LogTouchEngine, Verbose, TEXT("  TEInstanceLinkSetDoubleValue[%s]:  %s"), *GetCurrentThreadStr(), *Identifier);
		}

		if (Result != TEResultSuccess)
		{
			ErrorLog->AddResult(FString("setDoubleInput(): Unable to set double value: "), Result);
			return;
		}
	}

	void FTouchVariableManager::SetIntegerInput(const FString& Identifier, const TArray<int32_t>& Op)
	{
		check(IsInGameThread());
		const auto AnsiString = StringCast<ANSICHAR>(*Identifier);
		const char* IdentifierAsCStr = AnsiString.Get();

		TouchObject<TELinkInfo> Info;
		TEResult Result = TEInstanceLinkGetInfo(TouchEngineInstance, IdentifierAsCStr, Info.take());

		if (Result != TEResultSuccess)
		{
			ErrorLog->AddResult(FString("setIntegerInput(): Unable to get input Info, ") + FString(Identifier) + " may not exist. ", Result);
			return;
		}

		if (Info->type != TELinkTypeInt)
		{
			ErrorLog->AddError(FString("setIntegerInput(): Input named: ") + FString(Identifier) + " is not an integer input.");
			return;
		}

		Result = TEInstanceLinkSetIntValue(TouchEngineInstance, IdentifierAsCStr, Op.GetData(), Op.Num());
		UE_LOG(LogTouchEngine, Verbose, TEXT("  TEInstanceLinkSetIntValue[%s]:  %s"), *GetCurrentThreadStr(), *Identifier);

		if (Result != TEResultSuccess)
		{
			ErrorLog->AddResult(FString("setIntegerInput(): Unable to set integer value: "), Result);
			return;
		}
	}

	void FTouchVariableManager::SetStringInput(const FString& Identifier, const char*& Op)
	{
		check(IsInGameThread());
		const auto AnsiString = StringCast<ANSICHAR>(*Identifier);
		const char* IdentifierAsCStr = AnsiString.Get();

		TouchObject<TELinkInfo> Info;
		TEResult Result = TEInstanceLinkGetInfo(TouchEngineInstance, IdentifierAsCStr, Info.take());

		if (Result != TEResultSuccess)
		{
			ErrorLog->AddResult(FString("setStringInput(): Unable to get input Info, ") + FString(Identifier) + " may not exist. ", Result);
			return;
		}

		if (Info->type == TELinkTypeString)
		{
			Result = TEInstanceLinkSetStringValue(TouchEngineInstance, IdentifierAsCStr, Op);
			UE_LOG(LogTouchEngine, Verbose, TEXT("  TEInstanceLinkSetStringValue[%s]:  %s"), *GetCurrentThreadStr(), *Identifier);
		}
		else if (Info->type == TELinkTypeStringData)
		{
			TETable* Table = TETableCreate();
			TETableResize(Table, 1, 1);
			TETableSetStringValue(Table, 0, 0, Op);

			Result = TEInstanceLinkSetTableValue(TouchEngineInstance, IdentifierAsCStr, Table);
			UE_LOG(LogTouchEngine, Verbose, TEXT("  TEInstanceLinkSetTableValue[%s]:  %s"), *GetCurrentThreadStr(), *Identifier);
		}
		else
		{
			ErrorLog->AddError(FString("setStringInput(): Input named: ") + FString(Identifier) + " is not a string input.");
			return;
		}


		if (Result != TEResultSuccess)
		{
			ErrorLog->AddResult(FString("setStringInput(): Unable to set string value: "), Result);
			return;
		}
	}

	void FTouchVariableManager::SetTableInput(const FString& Identifier, const FTouchDATFull& Op)
	{
		check(IsInGameThread());
		const auto AnsiString = StringCast<ANSICHAR>(*Identifier);
		const char* IdentifierAsCStr = AnsiString.Get();

		TouchObject<TELinkInfo> Info;
		TEResult Result = TEInstanceLinkGetInfo(TouchEngineInstance, IdentifierAsCStr, Info.take());
		
		if (Result != TEResultSuccess)
		{
			ErrorLog->AddResult(FString("setTableInput(): Unable to get input Info, ") + FString(Identifier) + " may not exist. ", Result);
			return;
		}

		if (Info->type == TELinkTypeString)
		{
			const char* string = TETableGetStringValue(Op.ChannelData, 0, 0);
			Result = TEInstanceLinkSetStringValue(TouchEngineInstance, IdentifierAsCStr, string);
			UE_LOG(LogTouchEngine, Verbose, TEXT("  TEInstanceLinkSetStringValue[%s]:  %s"), *GetCurrentThreadStr(), *Identifier);
		}
		else if (Info->type == TELinkTypeStringData)
		{
			Result = TEInstanceLinkSetTableValue(TouchEngineInstance, IdentifierAsCStr, Op.ChannelData);
			UE_LOG(LogTouchEngine, Verbose, TEXT("  TEInstanceLinkSetStringValue[%s]:  %s"), *GetCurrentThreadStr(), *Identifier);
		}
		else
		{
			ErrorLog->AddError(FString("setTableInput(): Input named: ") + FString(Identifier) + " is not a table input.");
			return;
		}


		if (Result != TEResultSuccess)
		{
			ErrorLog->AddResult(FString("setTableInput(): Unable to set table value: "), Result);
			return;
		}
	}
	

	void FTouchVariableManager::SetFrameLastUpdatedForParameter(const FString& Identifier, int64 FrameID)
	{
		LastFrameParameterUpdated.Add(Identifier, FrameID);
	}

	int64 FTouchVariableManager::GetFrameLastUpdatedForParameter(const FString& Identifier)
	{
		return LastFrameParameterUpdated.FindOrAdd(Identifier, -1);
	}

	void FTouchVariableManager::ClearSavedData()
	{
		TArray<FName> InputKeys;
		{
			FScopeLock ILock(&TOPInputsLock);
			TOPInputs.GenerateKeyArray(InputKeys);
			TOPInputs.Empty(); // we need to make sure we do not hold TETextures references which would stop them from being released by TouchEngine
		}
		
		for (auto Identifier:InputKeys)
		{
			const auto AnsiString = StringCast<ANSICHAR>(*Identifier.ToString());
			const char* IdentifierAsCStr = AnsiString.Get();
			TEInstanceLinkSetTextureValue(TouchEngineInstance, IdentifierAsCStr, nullptr, ResourceProvider->GetContext()); // 
		}
		
		// FScopeLock OLock(&TOPOutputsLock);
		// TOPOutputs.Empty(); //todo: check that the UTexture are properly destroyed
	}
}
