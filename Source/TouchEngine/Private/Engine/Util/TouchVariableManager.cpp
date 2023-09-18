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
#include "Engine/Util/TouchErrorLog.h"
#include "Rendering/TouchResourceProvider.h"
#include "Rendering/Exporting/TouchExportParams.h"

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
		FTouchEngineCHOP Chop;
		TouchObject<TELinkInfo> LinkInfo;
		if (GetLinkInfo(Identifier, LinkInfo, TEScopeOutput, TELinkTypeFloatBuffer, GET_FUNCTION_NAME_CHECKED(FTouchVariableManager, GetCHOPOutputSingleSample)))
		{
			const auto AnsiString = StringCast<ANSICHAR>(*Identifier);
			const char* IdentifierAsCStr = AnsiString.Get();
			
			TouchObject<TEFloatBuffer> Buf;
			const TEResult Result = TEInstanceLinkGetFloatBufferValue(TouchEngineInstance, IdentifierAsCStr, TELinkValueCurrent, Buf.take());
			if (Result == TEResultSuccess && Buf != nullptr)
			{
				FTouchEngineCHOPChannel& Output = CHOPChannelOutputs.FindOrAdd(Identifier); //todo: to be reviewed, why a FTouchEngineCHOPChannel?

				const int32 ChannelCount = TEFloatBufferGetChannelCount(Buf);
				const int64 NumSamples = TEFloatBufferGetValueCount(Buf);
				Chop.Channels.Reserve(ChannelCount);

				double rate = TEFloatBufferGetRate(Buf);
				if (!TEFloatBufferIsTimeDependent(Buf))
				{
					const float* const* Channels = TEFloatBufferGetValues(Buf);
					const char* const* ChannelNames = TEFloatBufferGetChannelNames(Buf);
					
					// Use the channel data here
					if (NumSamples > 0 && ChannelCount > 0)
					{
						for (int i = 0; i < ChannelCount; i++)
						{
							Chop.Channels.Add(FTouchEngineCHOPChannel {{Channels[i], static_cast<int>(NumSamples)}, ChannelNames[i]});
						}
					}
				}
				else //todo: to be reviewed
				{
					//length /= rate / MyFrameRate;
					const float* const* Channels = TEFloatBufferGetValues(Buf);
					const char* const* ChannelNames = TEFloatBufferGetChannelNames(Buf);

					// Use the channel data here
					if (NumSamples > 0 && ChannelCount > 0)
					{
						Output.Values.SetNum(ChannelCount);

						for (int32 i = 0; i < ChannelCount; i++)
						{
							Output.Values[i] = Channels[i][NumSamples - 1];
						}
						Output.Name = ChannelNames[0];
					}
					Chop.Channels.Add(Output);
				}
			}
			else if (Result != TEResultSuccess)
			{
				ErrorLog->AddResult(FTouchErrorLog::EErrorType::TEInstanceLinkGetValueError, Result, Identifier, GET_FUNCTION_NAME_CHECKED(FTouchVariableManager, GetCHOPOutputSingleSample));
			}
		}

		return FTouchEngineCHOP{};
	}

	FTouchEngineCHOP FTouchVariableManager::GetCHOPOutput(const FString& Identifier)
	{
		TouchObject<TELinkInfo> LinkInfo;
		if (GetLinkInfo(Identifier, LinkInfo, TEScopeOutput, TELinkTypeFloatBuffer, GET_FUNCTION_NAME_CHECKED(FTouchVariableManager, GetCHOPOutput)))
		{
			const auto AnsiString = StringCast<ANSICHAR>(*Identifier);
			const char* IdentifierAsCStr = AnsiString.Get();
			
			TouchObject<TEFloatBuffer> Buf = nullptr;
			const TEResult Result = TEInstanceLinkGetFloatBufferValue(TouchEngineInstance, IdentifierAsCStr, TELinkValueCurrent, Buf.take());
			UE_LOG(LogTouchEngineTECalls, Log, TEXT("  TEInstanceLinkGetFloatBufferValue[%s]  for '%s' => %s"), *GetCurrentThreadStr(), *Identifier, *TEResultToString(Result));
			if (Result == TEResultSuccess)
			{
				FTouchEngineCHOP& Output = CHOPOutputs.FindOrAdd(Identifier);

				const int32_t ChannelCount = TEFloatBufferGetChannelCount(Buf);
				const uint32_t NumSamples = TEFloatBufferGetValueCount(Buf);
				const float* const* Channels = TEFloatBufferGetValues(Buf);
				const char* const* ChannelNames = TEFloatBufferGetChannelNames(Buf);
				Output.Channels.SetNumUninitialized(ChannelCount);
				
				if (TEFloatBufferIsTimeDependent(Buf))
				{
				}

				Output.Channels.SetNum(ChannelCount);
				for (int i = 0; i < ChannelCount; i++)
				{
					Output.Channels[i] =FTouchEngineCHOPChannel{ {Channels[i], static_cast<int>(NumSamples)},ChannelNames[i] };
				}
				return Output;
			}
			else
			{
				ErrorLog->AddResult(FTouchErrorLog::EErrorType::TEInstanceLinkGetValueError, Result, Identifier, GET_FUNCTION_NAME_CHECKED(FTouchVariableManager, GetCHOPOutput));
			}
		}
		return FTouchEngineCHOP{};
	}

	UTexture2D* FTouchVariableManager::GetTOPOutput(const FString& Identifier)
	{
		TouchObject<TELinkInfo> LinkInfo;
		if (GetLinkInfo(Identifier, LinkInfo, TEScopeOutput, TELinkTypeTexture, GET_FUNCTION_NAME_CHECKED(FTouchVariableManager, GetTOPOutput)))
		{
			FScopeLock Lock(&TOPOutputsLock);

			const FName ParamName(Identifier);
			if (UTexture2D** Top = TOPOutputs.Find(ParamName))
			{
				return *Top;
			}
			ErrorLog->AddError(FTouchErrorLog::EErrorType::TEInstanceLinkGetValueError, Identifier, GET_FUNCTION_NAME_CHECKED(FTouchVariableManager, GetCHOPOutput),
				TEXT("No texture received for the variable."));
		}
		return nullptr;
	}

	FTouchDATFull FTouchVariableManager::GetTableOutput(const FString& Identifier) const
	{
		FTouchDATFull DATFull;
		TouchObject<TELinkInfo> LinkInfo;
		if (GetLinkInfo(Identifier, LinkInfo, TEScopeOutput, TELinkTypeStringData, GET_FUNCTION_NAME_CHECKED(FTouchVariableManager, GetTableOutput)))
		{
			const auto AnsiString = StringCast<ANSICHAR>(*Identifier);
			const char* IdentifierAsCStr = AnsiString.Get();
			
			const TEResult Result = TEInstanceLinkGetTableValue(TouchEngineInstance, IdentifierAsCStr, TELinkValueCurrent, DATFull.TableData.take());
			UE_LOG(LogTouchEngineTECalls, Log, TEXT("  TEInstanceLinkGetTableValue[%s]  for '%s' => %s"), *GetCurrentThreadStr(), *Identifier, *TEResultToString(Result));
			if (Result != TEResultSuccess)
			{
				ErrorLog->AddResult(FTouchErrorLog::EErrorType::TEInstanceLinkGetValueError, Result, Identifier, GET_FUNCTION_NAME_CHECKED(FTouchVariableManager, GetTableOutput));
			}
		}

		return DATFull;
	}

	TArray<FString> FTouchVariableManager::GetCHOPChannelNames(const FString& Identifier) const
	{
		if (const FTouchEngineCHOP* FullChop = CHOPOutputs.Find(Identifier))
		{
			TArray<FString> RetVal;
			RetVal.Reserve(FullChop->Channels.Num());
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
		bool c = {};
		TouchObject<TELinkInfo> LinkInfo;
		if (GetLinkInfo(Identifier, LinkInfo, TEScopeOutput, TELinkTypeBoolean, GET_FUNCTION_NAME_CHECKED(FTouchVariableManager, GetBooleanOutput)))
		{
			const auto AnsiString = StringCast<ANSICHAR>(*Identifier);
			const char* IdentifierAsCStr = AnsiString.Get();
			
			const TEResult Result = TEInstanceLinkGetBooleanValue(TouchEngineInstance, IdentifierAsCStr, TELinkValueCurrent, &c);
			UE_LOG(LogTouchEngineTECalls, Log, TEXT("  TEInstanceLinkGetBooleanValue[%s]  for '%s' => %s"), *GetCurrentThreadStr(), *Identifier, *TEResultToString(Result));
			if (Result != TEResultSuccess)
			{
				ErrorLog->AddResult(FTouchErrorLog::EErrorType::TEInstanceLinkGetValueError, Result, Identifier, GET_FUNCTION_NAME_CHECKED(FTouchVariableManager, GetBooleanOutput));
			}
		}

		return c;
	}

	double FTouchVariableManager::GetDoubleOutput(const FString& Identifier)
	{
		double c = {};
		TouchObject<TELinkInfo> LinkInfo;
		if (GetLinkInfo(Identifier, LinkInfo, TEScopeOutput, TELinkTypeDouble, GET_FUNCTION_NAME_CHECKED(FTouchVariableManager, GetDoubleOutput)))
		{
			const auto AnsiString = StringCast<ANSICHAR>(*Identifier);
			const char* IdentifierAsCStr = AnsiString.Get();
			
			const TEResult Result = TEInstanceLinkGetDoubleValue(TouchEngineInstance, IdentifierAsCStr, TELinkValueCurrent, &c, 1);
			UE_LOG(LogTouchEngineTECalls, Log, TEXT("  TEInstanceLinkGetDoubleValue[%s]  for '%s' => %s"), *GetCurrentThreadStr(), *Identifier, *TEResultToString(Result));
			if (Result != TEResultSuccess)
			{
				ErrorLog->AddResult(FTouchErrorLog::EErrorType::TEInstanceLinkGetValueError, Result, Identifier, GET_FUNCTION_NAME_CHECKED(FTouchVariableManager, GetDoubleOutput));
			}
		}

		return c;
	}

	int32_t FTouchVariableManager::GetIntegerOutput(const FString& Identifier)
	{
		int32_t c = {};
		TouchObject<TELinkInfo> LinkInfo;
		if (GetLinkInfo(Identifier, LinkInfo, TEScopeOutput, TELinkTypeInt, GET_FUNCTION_NAME_CHECKED(FTouchVariableManager, GetIntegerOutput)))
		{
			const auto AnsiString = StringCast<ANSICHAR>(*Identifier);
			const char* IdentifierAsCStr = AnsiString.Get();
			
			const TEResult Result = TEInstanceLinkGetIntValue(TouchEngineInstance, IdentifierAsCStr, TELinkValueCurrent, &c, 1);
			UE_LOG(LogTouchEngineTECalls, Log, TEXT("  TEInstanceLinkGetIntValue[%s]  for '%s' => %s"), *GetCurrentThreadStr(), *Identifier, *TEResultToString(Result));
			if (Result != TEResultSuccess)
			{
				ErrorLog->AddResult(FTouchErrorLog::EErrorType::TEInstanceLinkGetValueError, Result, Identifier, GET_FUNCTION_NAME_CHECKED(FTouchVariableManager, GetIntegerOutput));
			}
		}

		return c;
	}

	TouchObject<TEString> FTouchVariableManager::GetStringOutput(const FString& Identifier)
	{
		TouchObject<TEString> c = {};
		TouchObject<TELinkInfo> LinkInfo;
		if (GetLinkInfo(Identifier, LinkInfo, TEScopeOutput, TELinkTypeString, GET_FUNCTION_NAME_CHECKED(FTouchVariableManager, GetStringOutput)))
		{
			const auto AnsiString = StringCast<ANSICHAR>(*Identifier);
			const char* IdentifierAsCStr = AnsiString.Get();
			
			const TEResult Result = TEInstanceLinkGetStringValue(TouchEngineInstance, IdentifierAsCStr, TELinkValueCurrent, c.take());
			UE_LOG(LogTouchEngineTECalls, Log, TEXT("  TEInstanceLinkGetStringValue[%s]  for '%s' => %s"), *GetCurrentThreadStr(), *Identifier, *TEResultToString(Result));
			if (Result != TEResultSuccess)
			{
				ErrorLog->AddResult(FTouchErrorLog::EErrorType::TEInstanceLinkGetValueError, Result, Identifier, GET_FUNCTION_NAME_CHECKED(FTouchVariableManager, GetStringOutput));
			}
		}

		return c;
	}
	

	void FTouchVariableManager::SetCHOPInputSingleSample(const FString& Identifier, const FTouchEngineCHOPChannel& CHOPChannel)
	{
		TouchObject<TELinkInfo> LinkInfo;
		if (GetLinkInfo(Identifier, LinkInfo, TEScopeInput, TELinkTypeFloatBuffer, GET_FUNCTION_NAME_CHECKED(FTouchVariableManager, SetCHOPInputSingleSample)))
		{
			const auto AnsiString = StringCast<ANSICHAR>(*Identifier);
			const char* IdentifierAsCStr = AnsiString.Get();
			
			TArray<const float*> DataPointers {CHOPChannel.Values.GetData()};
			// DataPointers.Reserve(CHOPChannel.Values.Num());
			// for (int i = 0; i < CHOPChannel.Values.Num(); i++)
			// {
			// 	DataPointers.Add(&CHOPChannel.Values[i]);
			// }

			TouchObject<TEFloatBuffer> Buf;
			Buf.take(TEFloatBufferCreate(-1.f, CHOPChannel.Values.Num(), 1, nullptr));

			TEResult Result = TEFloatBufferSetValues(Buf, DataPointers.GetData(), 1);
			UE_LOG(LogTouchEngineTECalls, Log, TEXT("  TEFloatBufferSetValues[%s]  for '%s' => %s"), *GetCurrentThreadStr(), *Identifier, *TEResultToString(Result));
			if (Result != TEResultSuccess)
			{
				ErrorLog->AddResult(FTouchErrorLog::EErrorType::TEInstanceLinkSetValueError, Result, Identifier, GET_FUNCTION_NAME_CHECKED(FTouchVariableManager, SetCHOPInputSingleSample));
				return;
			}
			
			Result = TEInstanceLinkAddFloatBuffer(TouchEngineInstance, IdentifierAsCStr, Buf);
			UE_LOG(LogTouchEngineTECalls, Log, TEXT("  TEInstanceLinkAddFloatBuffer[%s]  for '%s' => %s"), *GetCurrentThreadStr(), *Identifier, *TEResultToString(Result));
			if (Result != TEResultSuccess)
			{
				ErrorLog->AddResult(FTouchErrorLog::EErrorType::TEInstanceLinkSetValueError, Result, Identifier, GET_FUNCTION_NAME_CHECKED(FTouchVariableManager, SetCHOPInputSingleSample),
					TEXT("Unable to append buffer values"));
			}
		}

	}

	void FTouchVariableManager::SetCHOPInput(const FString& Identifier, const FTouchEngineCHOP& CHOP)
	{
		TouchObject<TELinkInfo> LinkInfo;
		if (GetLinkInfo(Identifier, LinkInfo, TEScopeInput, TELinkTypeFloatBuffer, GET_FUNCTION_NAME_CHECKED(FTouchVariableManager, SetCHOPInput)))
		{
			const auto AnsiString = StringCast<ANSICHAR>(*Identifier);
			const char* IdentifierAsCStr = AnsiString.Get();
			
			int32 Capacity = CHOP.Channels.IsEmpty() ? 0 : CHOP.Channels[0].Values.Num();

			bool bAreAllChannelNamesEmpty = true;
			TArray<std::string> ChannelNamesANSI; // Store as temporary string to keep a reference until the buffer is created
			TArray<const char*> ChannelNames;
			TArray<const float*> DataPointers;
			ChannelNamesANSI.Reserve(CHOP.Channels.Num());
			ChannelNames.Reserve(CHOP.Channels.Num());
			DataPointers.Reserve(CHOP.Channels.Num());
			
			for (int i = 0; i < CHOP.Channels.Num(); i++)
			{
				if (CHOP.Channels[i].Values.Num() != Capacity) //CHOP is not valid
				{
					Capacity = -1;
					break;
				}
				const FString& ChannelName = CHOP.Channels[i].Name;
				bAreAllChannelNamesEmpty &= ChannelName.IsEmpty();
			
				auto ChannelNameANSI = StringCast<ANSICHAR>(*ChannelName);
				std::string ChannelNameString(ChannelNameANSI.Get());

				const int32 Index = ChannelNamesANSI.Emplace(ChannelNameString);
				ChannelNames.Emplace(ChannelName.IsEmpty() ? nullptr : ChannelNamesANSI[Index].c_str());

				DataPointers.Add(CHOP.Channels[i].Values.GetData());
			}

			if (Capacity == -1)
			{
				ErrorLog->AddError(FTouchErrorLog::EErrorType::TEInstanceLinkSetValueError, Identifier, GET_FUNCTION_NAME_CHECKED(FTouchVariableManager, SetCHOPInput),
						TEXT("The given CHOP is not valid."));
				return;
			}

			const TouchObject<TEFloatBuffer> Buffer = TouchObject<TEFloatBuffer>::make_take(TEFloatBufferCreate(-1.f, CHOP.Channels.Num(), Capacity, bAreAllChannelNamesEmpty ? nullptr : ChannelNames.GetData()));
			TEResult Result = TEFloatBufferSetValues(Buffer, DataPointers.GetData(), Capacity);
			UE_LOG(LogTouchEngineTECalls, Log, TEXT("  TEFloatBufferSetValues[%s]  for '%s' => %s"), *GetCurrentThreadStr(), *Identifier, *TEResultToString(Result));
			if (Result != TEResultSuccess)
			{
				ErrorLog->AddResult(FTouchErrorLog::EErrorType::TEInstanceLinkSetValueError, Result, Identifier, GET_FUNCTION_NAME_CHECKED(FTouchVariableManager, SetCHOPInput));
				return;
			}
			
			Result = TEInstanceLinkAddFloatBuffer(TouchEngineInstance, IdentifierAsCStr, Buffer);
			UE_LOG(LogTouchEngineTECalls, Log, TEXT("  TEInstanceLinkAddFloatBuffer[%s]  for '%s' => %s"), *GetCurrentThreadStr(), *Identifier, *TEResultToString(Result));
			if (Result != TEResultSuccess)
			{
				ErrorLog->AddResult(FTouchErrorLog::EErrorType::TEInstanceLinkSetValueError, Result, Identifier, GET_FUNCTION_NAME_CHECKED(FTouchVariableManager, SetCHOPInput),
					TEXT("Unable to append buffer values"));
			}
		}
	}

	void FTouchVariableManager::SetTOPInput(const FString& Identifier, UTexture* Texture, const FTouchEngineInputFrameData& FrameData)
	{
		TouchObject<TELinkInfo> LinkInfo;
		if (GetLinkInfo(Identifier, LinkInfo, TEScopeInput, TELinkTypeTexture, GET_FUNCTION_NAME_CHECKED(FTouchVariableManager, SetTOPInput)))
		{
			// Fast path
			const auto AnsiString = StringCast<ANSICHAR>(*Identifier);
			const char* IdentifierAsCStr = AnsiString.Get();
			if (!Texture)
			{
				const TEResult Result = TEInstanceLinkSetTextureValue(TouchEngineInstance, IdentifierAsCStr, Texture, ResourceProvider->GetContext());
				UE_LOG(LogTouchEngineTECalls, Log, TEXT("  TEInstanceLinkSetTextureValue[%s]  for '%s' => %s"), *GetCurrentThreadStr(), *Identifier, *TEResultToString(Result));
				return;
			}
		
			const FTouchExportParameters ExportParams {TouchEngineInstance, *Identifier, Texture, FrameData};
			const TouchObject<TETexture> ExportedTexture = ResourceProvider->ExportTextureToTouchEngine_AnyThread(ExportParams);
			
			{
				const TEResult Result = TEInstanceLinkSetTextureValue(TouchEngineInstance, IdentifierAsCStr, ExportedTexture, ResourceProvider->GetContext());
				UE_LOG(LogTouchEngineTECalls, Log, TEXT("  TEInstanceLinkSetTextureValue[%s]  for '%s' => %s"), *GetCurrentThreadStr(), *Identifier, *TEResultToString(Result));
				TEInstanceLinkSetInterest(TouchEngineInstance, IdentifierAsCStr, TELinkInterestNoValues);
				if (Result != TEResultSuccess)
				{
					ErrorLog->AddResult(FTouchErrorLog::EErrorType::TEInstanceLinkSetValueError, Result, Identifier, GET_FUNCTION_NAME_CHECKED(FTouchVariableManager, SetTOPInput));
					return;
				}
			}

			{
				FScopeLock Lock(&TOPInputsLock);
				const FName ParamName(Identifier);
				if (const TouchObject<TETexture>* Top = TOPInputs.Find(ParamName))
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
	}

	void FTouchVariableManager::SetBooleanInput(const FString& Identifier, const bool& Op)
	{
		TouchObject<TELinkInfo> LinkInfo;
		if (GetLinkInfo(Identifier, LinkInfo, TEScopeInput, TELinkTypeBoolean, GET_FUNCTION_NAME_CHECKED(FTouchVariableManager, SetBooleanInput)))
		{
			const auto AnsiString = StringCast<ANSICHAR>(*Identifier);
			const char* IdentifierAsCStr = AnsiString.Get();

			const TEResult Result = TEInstanceLinkSetBooleanValue(TouchEngineInstance, IdentifierAsCStr, Op);
			UE_LOG(LogTouchEngineTECalls, Log, TEXT("  TEInstanceLinkSetBooleanValue[%s]  for '%s' => %s"), *GetCurrentThreadStr(), *Identifier, *TEResultToString(Result));
			if (Result != TEResultSuccess)
			{
				ErrorLog->AddResult(FTouchErrorLog::EErrorType::TEInstanceLinkSetValueError, Result, Identifier, GET_FUNCTION_NAME_CHECKED(FTouchVariableManager, SetBooleanInput));
			}
		}
	}

	void FTouchVariableManager::SetDoubleInput(const FString& Identifier, const TArray<double>& Op)
	{
		TouchObject<TELinkInfo> LinkInfo;
		if (GetLinkInfo(Identifier, LinkInfo, TEScopeInput, TELinkTypeDouble, GET_FUNCTION_NAME_CHECKED(FTouchVariableManager, SetDoubleInput)))
		{
			const auto AnsiString = StringCast<ANSICHAR>(*Identifier);
			const char* IdentifierAsCStr = AnsiString.Get();
			TEResult Result;
			if (Op.Num() == LinkInfo->count)
			{
				Result = TEInstanceLinkSetDoubleValue(TouchEngineInstance, IdentifierAsCStr, Op.GetData(), LinkInfo->count);
				UE_LOG(LogTouchEngineTECalls, Log, TEXT("  TEInstanceLinkSetDoubleValue[%s]  for '%s' => %s"), *GetCurrentThreadStr(), *Identifier, *TEResultToString(Result));
			}
			else
			{
				if (Op.Num() > LinkInfo->count)
				{
					ErrorLog->AddCountMismatchWarning(LinkInfo, Op.Num(), Identifier, GET_FUNCTION_NAME_CHECKED(FTouchVariableManager, SetDoubleInput));
					// TArray<double> buffer {Op.GetData(), LinkInfo->count};
					Result = TEInstanceLinkSetDoubleValue(TouchEngineInstance, IdentifierAsCStr, Op.GetData(), LinkInfo->count);
					UE_LOG(LogTouchEngineTECalls, Log, TEXT("  TEInstanceLinkSetDoubleValue[%s]  for '%s' => %s"), *GetCurrentThreadStr(), *Identifier, *TEResultToString(Result));
				}
				else
				{
					ErrorLog->AddCountMismatchError(LinkInfo, Op.Num(), Identifier, GET_FUNCTION_NAME_CHECKED(FTouchVariableManager, SetDoubleInput));
					return;
				}
			}

			if (Result != TEResultSuccess)
			{
				ErrorLog->AddResult(FTouchErrorLog::EErrorType::TEInstanceLinkSetValueError, Result, Identifier, GET_FUNCTION_NAME_CHECKED(FTouchVariableManager, SetDoubleInput));
			}
		}
	}

	void FTouchVariableManager::SetIntegerInput(const FString& Identifier, const TArray<int32_t>& Op)
	{
		TouchObject<TELinkInfo> LinkInfo;
		if (GetLinkInfo(Identifier, LinkInfo, TEScopeInput, TELinkTypeInt, GET_FUNCTION_NAME_CHECKED(FTouchVariableManager, SetIntegerInput)))
		{
			const auto AnsiString = StringCast<ANSICHAR>(*Identifier);
			const char* IdentifierAsCStr = AnsiString.Get();

			TEResult Result;
			if (Op.Num() == LinkInfo->count)
			{
				Result = TEInstanceLinkSetIntValue(TouchEngineInstance, IdentifierAsCStr, Op.GetData(), LinkInfo->count);
				UE_LOG(LogTouchEngineTECalls, Log, TEXT("  TEInstanceLinkSetIntValue[%s]  for '%s' => %s"), *GetCurrentThreadStr(), *Identifier, *TEResultToString(Result));
			}
			else
			{
				if (Op.Num() > LinkInfo->count)
				{
					ErrorLog->AddCountMismatchWarning(LinkInfo, Op.Num(), Identifier, GET_FUNCTION_NAME_CHECKED(FTouchVariableManager, SetIntegerInput));
					// TArray<int> buffer {Op.GetData(), LinkInfo->count};
					Result = TEInstanceLinkSetIntValue(TouchEngineInstance, IdentifierAsCStr, Op.GetData(), LinkInfo->count);
					UE_LOG(LogTouchEngineTECalls, Log, TEXT("  TEInstanceLinkSetIntValue[%s]  for '%s' => %s"), *GetCurrentThreadStr(), *Identifier, *TEResultToString(Result));
				}
				else
				{
					ErrorLog->AddCountMismatchError(LinkInfo, Op.Num(), Identifier, GET_FUNCTION_NAME_CHECKED(FTouchVariableManager, SetIntegerInput));
					return;
				}
			}

			if (Result != TEResultSuccess)
			{
				ErrorLog->AddResult(FTouchErrorLog::EErrorType::TEInstanceLinkSetValueError, Result, Identifier, GET_FUNCTION_NAME_CHECKED(FTouchVariableManager, SetIntegerInput));
			}
		}
	}

	void FTouchVariableManager::SetStringInput(const FString& Identifier, const char*& Op)
	{
		TouchObject<TELinkInfo> LinkInfo;
		if (GetLinkInfo(Identifier, LinkInfo, TEScopeInput, GET_FUNCTION_NAME_CHECKED(FTouchVariableManager, SetStringInput)))
		{
			const auto AnsiString = StringCast<ANSICHAR>(*Identifier);
			const char* IdentifierAsCStr = AnsiString.Get();
			if (LinkInfo->type == TELinkTypeString)
			{
				const TEResult Result = TEInstanceLinkSetStringValue(TouchEngineInstance, IdentifierAsCStr, Op);
				UE_LOG(LogTouchEngineTECalls, Log, TEXT("  TEInstanceLinkSetStringValue[%s]  for '%s' => %s"), *GetCurrentThreadStr(), *Identifier, *TEResultToString(Result));
				if (Result != TEResultSuccess)
				{
					ErrorLog->AddResult(FTouchErrorLog::EErrorType::TEInstanceLinkSetValueError, Result, Identifier, GET_FUNCTION_NAME_CHECKED(FTouchVariableManager, SetStringInput),
						TEXT("Tried to set a String."));
				}
			}
			else if (LinkInfo->type == TELinkTypeStringData)
			{
				const TouchObject<TETable> Table = TouchObject<TETable>::make_take(TETableCreate());
				TETableResize(Table, 1, 1);
				TEResult Result = TETableSetStringValue(Table, 0, 0, Op);
				UE_LOG(LogTouchEngineTECalls, Log, TEXT("  TETableSetStringValue[%s]  for '%s' => %s"), *GetCurrentThreadStr(), *Identifier, *TEResultToString(Result));
				if (Result != TEResultSuccess)
				{
					ErrorLog->AddResult(FTouchErrorLog::EErrorType::TEInstanceLinkSetValueError, Result, Identifier, GET_FUNCTION_NAME_CHECKED(FTouchVariableManager, SetStringInput),
						TEXT("Tried to set a String value in a Table."));
				}
				
				Result = TEInstanceLinkSetTableValue(TouchEngineInstance, IdentifierAsCStr, Table);
				UE_LOG(LogTouchEngineTECalls, Log, TEXT("  TEInstanceLinkSetTableValue[%s]  for '%s' => %s"), *GetCurrentThreadStr(), *Identifier, *TEResultToString(Result));
				if (Result != TEResultSuccess)
				{
					ErrorLog->AddResult(FTouchErrorLog::EErrorType::TEInstanceLinkSetValueError, Result, Identifier, GET_FUNCTION_NAME_CHECKED(FTouchVariableManager, SetStringInput),
						TEXT("Tried to set a Table Value."));
				}
			}
			else
			{
				ErrorLog->AddTypeMismatchError(LinkInfo, TELinkTypeString, Identifier, GET_FUNCTION_NAME_CHECKED(FTouchVariableManager, SetStringInput));
			}
		}
	}

	void FTouchVariableManager::SetTableInput(const FString& Identifier, const FTouchDATFull& Op)
	{
		TouchObject<TELinkInfo> LinkInfo;
		if (GetLinkInfo(Identifier, LinkInfo, TEScopeInput, GET_FUNCTION_NAME_CHECKED(FTouchVariableManager, SetTableInput)))
		{
			const auto AnsiString = StringCast<ANSICHAR>(*Identifier);
			const char* IdentifierAsCStr = AnsiString.Get();
			if (LinkInfo->type == TELinkTypeString)
			{
				const char* String = TETableGetStringValue(Op.TableData, 0, 0);
				const TEResult Result = TEInstanceLinkSetStringValue(TouchEngineInstance, IdentifierAsCStr, String);
				UE_LOG(LogTouchEngineTECalls, Log, TEXT("  TEInstanceLinkSetStringValue[%s]  for '%s' => %s"), *GetCurrentThreadStr(), *Identifier, *TEResultToString(Result));
				if (Result != TEResultSuccess)
				{
					ErrorLog->AddResult(FTouchErrorLog::EErrorType::TEInstanceLinkSetValueError, Result, Identifier, GET_FUNCTION_NAME_CHECKED(FTouchVariableManager, SetTableInput),
						TEXT("Tried to set a String."));
				}
			}
			else if (LinkInfo->type == TELinkTypeStringData)
			{
				const TEResult Result = TEInstanceLinkSetTableValue(TouchEngineInstance, IdentifierAsCStr, Op.TableData);
				UE_LOG(LogTouchEngineTECalls, Log, TEXT("  TEInstanceLinkSetTableValue[%s]  for '%s' => %s"), *GetCurrentThreadStr(), *Identifier, *TEResultToString(Result));
				if (Result != TEResultSuccess)
				{
					ErrorLog->AddResult(FTouchErrorLog::EErrorType::TEInstanceLinkSetValueError, Result, Identifier, GET_FUNCTION_NAME_CHECKED(FTouchVariableManager, SetTableInput),
						TEXT("Tried to set a Table Value."));
				}
			}
			else
			{
				ErrorLog->AddError(FString("setTableInput(): Input named: ") + FString(Identifier) + " is not a table input.");
			}
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
		
		for (FName Identifier :InputKeys)
		{
			const auto AnsiString = StringCast<ANSICHAR>(*Identifier.ToString());
			const char* IdentifierAsCStr = AnsiString.Get();
			TEInstanceLinkSetTextureValue(TouchEngineInstance, IdentifierAsCStr, nullptr, ResourceProvider->GetContext()); // 
		}
	}

	bool FTouchVariableManager::GetLinkInfo(const FString& Identifier, TouchObject<TELinkInfo>& LinkInfo, TEScope ExpectedScope, TELinkType ExpectedType, const FName& FunctionName) const
	{
		check(IsInGameThread());
		const auto AnsiString = StringCast<ANSICHAR>(*Identifier);
		const char* IdentifierAsCStr = AnsiString.Get();
		const TEResult Result = TEInstanceLinkGetInfo(TouchEngineInstance, IdentifierAsCStr, LinkInfo.take());
		if (Result == TEResultSuccess && LinkInfo->scope == ExpectedScope && LinkInfo->type == ExpectedType)
		{
			return true;
		}
		else if (Result != TEResultSuccess)
		{
			ErrorLog->AddResult(FTouchErrorLog::EErrorType::TEInstanceLinkGetInfoError, Result, Identifier, FunctionName);
		}
		else if (LinkInfo->scope != ExpectedScope)
		{
			ErrorLog->AddScopeMismatchError(LinkInfo, TEScopeOutput, Identifier, FunctionName);
		}
		else if (LinkInfo->type != ExpectedType)
		{
			ErrorLog->AddTypeMismatchError(LinkInfo, ExpectedType, Identifier, FunctionName);
		}
		return false;
	}

	bool FTouchVariableManager::GetLinkInfo(const FString& Identifier, TouchObject<TELinkInfo>& LinkInfo, TEScope ExpectedScope, const FName& FunctionName) const
	{
		check(IsInGameThread());
		const auto AnsiString = StringCast<ANSICHAR>(*Identifier);
		const char* IdentifierAsCStr = AnsiString.Get();
		const TEResult Result = TEInstanceLinkGetInfo(TouchEngineInstance, IdentifierAsCStr, LinkInfo.take());
		if (Result == TEResultSuccess && LinkInfo->scope == ExpectedScope)
		{
			return true;
		}
		else if (Result != TEResultSuccess)
		{
			ErrorLog->AddResult(FTouchErrorLog::EErrorType::TEInstanceLinkGetInfoError, Result, Identifier, FunctionName);
		}
		else if (LinkInfo->scope != ExpectedScope)
		{
			ErrorLog->AddScopeMismatchError(LinkInfo, TEScopeOutput, Identifier, FunctionName);
		}
		return false;
	}
}
