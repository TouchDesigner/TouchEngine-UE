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
						Output.Values.Reserve(ChannelCount);

						for (int32 i = 0; i < ChannelCount; i++)
						{
							Output.Values.Add(Channels[i][NumSamples - 1]);
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
			UE_LOG(LogTouchEngineTECalls, Log, TEXT("  TEInstanceLinkGetFloatBufferValue(TEInstance: '%p', identifier: '%hs', which: 'TELinkValueCurrent', value: '%p') [Thread: '%s'] => Returned: '%s'"),
				TouchEngineInstance.get(),
				IdentifierAsCStr,
				Buf.get(),
				*GetCurrentThreadStr(),
				*TEResultToString(Result)
			);
			
			if (Result == TEResultSuccess)
			{
				FTouchEngineCHOP& Output = CHOPOutputs.FindOrAdd(Identifier);

				const int32_t ChannelCount = TEFloatBufferGetChannelCount(Buf);
				const uint32_t NumSamples = TEFloatBufferGetValueCount(Buf);
				const float* const* Channels = TEFloatBufferGetValues(Buf);
				const char* const* ChannelNames = TEFloatBufferGetChannelNames(Buf);
				Output.Channels.Empty(ChannelCount);
				
				if (TEFloatBufferIsTimeDependent(Buf))
				{
				}

				for (int i = 0; i < ChannelCount; i++)
				{
					Output.Channels.Add(FTouchEngineCHOPChannel{{Channels[i], static_cast<int>(NumSamples)},  ChannelNames[i]});
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
			UE_LOG(LogTouchEngineTECalls, Log, TEXT("  TEInstanceLinkGetTableValue(TEInstance: '%p', identifier: '%hs', which: 'TELinkValueCurrent', value: '%p') [Thread: '%s'] => Returned: '%s'"),
				TouchEngineInstance.get(),
				IdentifierAsCStr,
				DATFull.TableData.get(),
				*GetCurrentThreadStr(),
				*TEResultToString(Result)
			);

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
			UE_LOG(LogTouchEngineTECalls, Log, TEXT("  TEInstanceLinkGetBooleanValue(TEInstance: '%p', identifier: '%hs', which: 'TELinkValueCurrent', value: '%p') [Thread: '%s'] => Returned: '%s'"),
				TouchEngineInstance.get(),
				IdentifierAsCStr,
				&c,
				*GetCurrentThreadStr(),
				*TEResultToString(Result)
			);
			
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
			UE_LOG(LogTouchEngineTECalls, Log, TEXT("  TEInstanceLinkGetDoubleValue(TEInstance: '%p', identifier: '%hs', which: 'TELinkValueCurrent', value: '%p', count: '1') [Thread: '%s'] => Returned: '%s'"),
				TouchEngineInstance.get(),
				IdentifierAsCStr,
				&c,
				*GetCurrentThreadStr(),
				*TEResultToString(Result)
			);

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
			UE_LOG(LogTouchEngineTECalls, Log, TEXT("  TEInstanceLinkGetIntValue(TEInstance: '%p', identifier: '%hs', which: 'TELinkValueCurrent', value: '%p', count: '1') [Thread: '%s'] => Returned: '%s'"),
				TouchEngineInstance.get(),
				IdentifierAsCStr,
				&c,
				*GetCurrentThreadStr(),
				*TEResultToString(Result)
			);

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
			UE_LOG(LogTouchEngineTECalls, Log, TEXT("  TEInstanceLinkGetStringValue(TEInstance: '%p', identifier: '%hs', which: 'TELinkValueCurrent', string: '%p') [Thread: '%s'] => Returned: '%s'"),
				TouchEngineInstance.get(),
				IdentifierAsCStr,
				c.get(),
				*GetCurrentThreadStr(),
				*TEResultToString(Result)
			);

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

			TouchObject<TEFloatBuffer> Buf;
			Buf.take(TEFloatBufferCreate(-1.f, CHOPChannel.Values.Num(), 1, nullptr));
			const void* NullPointer = nullptr;
			UE_LOG(LogTouchEngineTECalls, Log, TEXT("  TEFloatBufferCreate(rate: '%f', channels: '%d', capacity: '%d', names: '%p') [Thread: '%s'] => Returned: '%p'"),
				-1.f,
				CHOPChannel.Values.Num(),
				1,
				NullPointer,
				*GetCurrentThreadStr(),
				Buf.get()
			);

			TEResult Result = TEFloatBufferSetValues(Buf, DataPointers.GetData(), 1);
			UE_LOG(LogTouchEngineTECalls, Log, TEXT("  TEFloatBufferSetValues(TEFloatBuffer: '%p', values: '%p', count: '1') [Thread: '%s'] => Returned: '%s'"),
				Buf.get(),
				DataPointers.GetData(),
				*GetCurrentThreadStr(),
				*TEResultToString(Result)
			);
			
			if (Result != TEResultSuccess)
			{
				ErrorLog->AddResult(FTouchErrorLog::EErrorType::TEInstanceLinkSetValueError, Result, Identifier, GET_FUNCTION_NAME_CHECKED(FTouchVariableManager, SetCHOPInputSingleSample));
				return;
			}
			
			Result = TEInstanceLinkAddFloatBuffer(TouchEngineInstance, IdentifierAsCStr, Buf);
			UE_LOG(LogTouchEngineTECalls, Log, TEXT("  TEInstanceLinkAddFloatBuffer(TEInstance: '%p', identifier: '%hs', TEFloatBuffer: '%p') [Thread: '%s'] => Returned: '%s'"),
				TouchEngineInstance.get(),
				IdentifierAsCStr,
				Buf.get(),
				*GetCurrentThreadStr(),
				*TEResultToString(Result)
			);

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
			UE_LOG(LogTouchEngineTECalls, Log, TEXT("  TEFloatBufferCreate(rate: '%f', channels: '%d', capacity: '%d', names: '%p') [Thread: '%s'] => Returned: '%p'"),
				-1.f,
				CHOP.Channels.Num(),
				Capacity,
				bAreAllChannelNamesEmpty ? nullptr : ChannelNames.GetData(),
				*GetCurrentThreadStr(),
				Buffer.get()
			);
			TEResult Result = TEFloatBufferSetValues(Buffer, DataPointers.GetData(), Capacity);
			UE_LOG(LogTouchEngineTECalls, Log, TEXT("  TEFloatBufferSetValues(TEFloatBuffer: '%p', values: '%p', count: '%d') [Thread: '%s'] => Returned: '%s'"),
				Buffer.get(),
				DataPointers.GetData(),
				Capacity,
				*GetCurrentThreadStr(),
				*TEResultToString(Result)
			);

			if (Result != TEResultSuccess)
			{
				ErrorLog->AddResult(FTouchErrorLog::EErrorType::TEInstanceLinkSetValueError, Result, Identifier, GET_FUNCTION_NAME_CHECKED(FTouchVariableManager, SetCHOPInput));
				return;
			}
			
			Result = TEInstanceLinkAddFloatBuffer(TouchEngineInstance, IdentifierAsCStr, Buffer);
			UE_LOG(LogTouchEngineTECalls, Log, TEXT("  TEInstanceLinkAddFloatBuffer(TEInstance: '%p', identifier: '%hs', TEFloatBuffer: '%p') [Thread: '%s'] => Returned: '%s'"),
				TouchEngineInstance.get(),
				IdentifierAsCStr,
				Buffer.get(),
				*GetCurrentThreadStr(),
				*TEResultToString(Result)
			);

			if (Result != TEResultSuccess)
			{
				ErrorLog->AddResult(FTouchErrorLog::EErrorType::TEInstanceLinkSetValueError, Result, Identifier, GET_FUNCTION_NAME_CHECKED(FTouchVariableManager, SetCHOPInput),
					TEXT("Unable to append buffer values"));
			}
		}
	}

	TFuture<bool> FTouchVariableManager::SetTOPInput(const FString& Identifier, const TSharedPtr<FExportedTouchTexture>& Texture, const FTouchEngineInputFrameData& FrameData)
	{
		TouchObject<TELinkInfo> LinkInfo;
		if (!GetLinkInfo(Identifier, LinkInfo, TEScopeInput, TELinkTypeTexture, GET_FUNCTION_NAME_CHECKED(FTouchVariableManager, SetTOPInput)))
		{
			return MakeFulfilledPromise<bool>(false).GetFuture();
		}
		
		// Fast path
		const auto AnsiString = StringCast<ANSICHAR>(*Identifier);
		const char* IdentifierAsCStr = AnsiString.Get();
		if (!Texture)
		{
			const void* NullPointer = nullptr;
			const TEResult Result = TEInstanceLinkSetTextureValue(TouchEngineInstance, IdentifierAsCStr, nullptr, ResourceProvider->GetContext());
			UE_LOG(LogTouchEngineTECalls, Log, TEXT("  TEInstanceLinkSetTextureValue(TEInstance: '%p', identifier: '%hs', texture: '%p' ['%s'], context: '%p') [Thread: '%s', Frame: '%lld']  =>  '%s'"),
				TouchEngineInstance.get(),
				IdentifierAsCStr,
				NullPointer,
				*Texture->DebugName,
				ResourceProvider->GetContext(),
				*GetCurrentThreadStr(),
				FrameData.FrameID,
				*TEResultToString(Result)
			)
			return MakeFulfilledPromise<bool>(false).GetFuture();
		}

		TPromise<bool> Promise;
		TFuture<bool> Future = Promise.GetFuture();

		const FTouchExportParameters ExportParams {TouchEngineInstance, *Identifier, Texture.ToSharedRef(), FrameData};
		ResourceProvider->ExportTextureToTouchEngine_AnyThread(ExportParams)
			.Next([Promise = MoveTemp(Promise), WeakThis = AsWeak(), Identifier, ExportParams](TouchObject<TETexture> ExportedTexture) mutable
			{
				TSharedPtr<FTouchVariableManager> This = WeakThis.Pin();
				if (!This)
				{
					Promise.SetValue(false);
					return;
				}

				UE_LOG(LogTouchEngine, Verbose, TEXT("[SetTOPInput[%s]] ResourceProvider->ExportTextureToTouchEngine_AnyThread.Next => returned texture '%s' for input '%s' on frame %lld"), *GetCurrentThreadStr(), *ExportParams.TextureToBeExported->DebugName, *Identifier, ExportParams.FrameData.FrameID)

				const auto AnsiString = StringCast<ANSICHAR>(*Identifier);
				const char* IdentifierAsCStr = AnsiString.Get();
				
				{
					// Logging before the call as the call will generate some texture callbacks and we want to keep the log consistent with the code
					UE_LOG(LogTouchEngineTECalls, Log, TEXT("  TEInstanceLinkSetTextureValue(TEInstance: '%p', identifier: '%hs', texture: '%p' ['%s'], context: '%p') [Thread: '%s', Frame: '%lld']"),
						This->TouchEngineInstance.get(),
						IdentifierAsCStr,
						ExportedTexture.get(),
						*ExportParams.TextureToBeExported->DebugName ,
						This->ResourceProvider->GetContext(),
						*GetCurrentThreadStr(),
						ExportParams.FrameData.FrameID
					)
					const TEResult Result = TEInstanceLinkSetTextureValue(This->TouchEngineInstance, IdentifierAsCStr, ExportedTexture, This->ResourceProvider->GetContext());
					
					const TEResult SetInterestResult = TEInstanceLinkSetInterest(This->TouchEngineInstance, IdentifierAsCStr, TELinkInterestNoValues);
					UE_LOG(LogTouchEngineTECalls, Log, TEXT("  TEInstanceLinkSetInterest(TEInstance: '%p', identifier: '%hs', interest: 'TELinkInterestNoValues') [Thread: '%s', Frame: '%lld']  =>  '%s'"),
						This->TouchEngineInstance.get(),
						IdentifierAsCStr,
						*GetCurrentThreadStr(),
						ExportParams.FrameData.FrameID,
						*TEResultToString(SetInterestResult)
					)
					
					if (Result != TEResultSuccess)
					{
						UE_LOG(LogTouchEngineTECalls, Error, TEXT("  TEInstanceLinkSetTextureValue(TEInstance: '%p', identifier: '%hs', texture: '%p' ['%s'], context: '%p') [Thread: '%s', Frame: '%lld']  =>  returned '%s'"),
							This->TouchEngineInstance.get(),
							IdentifierAsCStr,
							ExportedTexture.get(),
							*ExportParams.TextureToBeExported->DebugName ,
							This->ResourceProvider->GetContext(),
							*GetCurrentThreadStr(),
							ExportParams.FrameData.FrameID,
							*TEResultToString(Result)
						)
						This->ErrorLog->AddResult(FTouchErrorLog::EErrorType::TEInstanceLinkSetValueError, Result, Identifier, GET_FUNCTION_NAME_CHECKED(FTouchVariableManager, SetTOPInput));
						Promise.SetValue(false);
						return;
					}
				}

				{
					// This array is used to clear the input texture values when cancelling. See ClearSavedData
					FScopeLock Lock(&This->TOPInputsLock); //todo: do we actually need to keep the TETexture all this time?
					const FName ParamName(Identifier);
					if (const TouchObject<TETexture>* Top = This->TOPInputs.Find(ParamName))
					{
						if (Top->get() != ExportedTexture.get())
						{
							This->TOPInputs.Add(ParamName, ExportedTexture);
						}
					}
					else
					{
						This->TOPInputs.Add(ParamName, ExportedTexture);
					}
				}
				Promise.SetValue(true);
			});
		
		return Future;
	}

	void FTouchVariableManager::SetBooleanInput(const FString& Identifier, const bool& Op)
	{
		TouchObject<TELinkInfo> LinkInfo;
		if (GetLinkInfo(Identifier, LinkInfo, TEScopeInput, TELinkTypeBoolean, GET_FUNCTION_NAME_CHECKED(FTouchVariableManager, SetBooleanInput)))
		{
			const auto AnsiString = StringCast<ANSICHAR>(*Identifier);
			const char* IdentifierAsCStr = AnsiString.Get();

			const TEResult Result = TEInstanceLinkSetBooleanValue(TouchEngineInstance, IdentifierAsCStr, Op);
			UE_LOG(LogTouchEngineTECalls, Log, TEXT("  TEInstanceLinkSetBooleanValue(TEInstance: '%p', identifier: '%hs', value: '%s') [Thread: '%s'] => Returned: '%s'"),
				TouchEngineInstance.get(),
				IdentifierAsCStr,
				Op ? TEXT("True") : TEXT("False"),
				*GetCurrentThreadStr(),
				*TEResultToString(Result)
			);

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
			
			if (Op.Num() > LinkInfo->count)
			{
				ErrorLog->AddCountMismatchWarning(LinkInfo, Op.Num(), Identifier, GET_FUNCTION_NAME_CHECKED(FTouchVariableManager, SetDoubleInput));
			}
			else if (Op.Num() < LinkInfo->count)
			{
				ErrorLog->AddCountMismatchError(LinkInfo, Op.Num(), Identifier, GET_FUNCTION_NAME_CHECKED(FTouchVariableManager, SetDoubleInput));
				return;
			}
			
			const TEResult Result = TEInstanceLinkSetDoubleValue(TouchEngineInstance, IdentifierAsCStr, Op.GetData(), LinkInfo->count);
			UE_LOG(LogTouchEngineTECalls, Log, TEXT("  TEInstanceLinkSetDoubleValue(TEInstance: '%p', identifier: '%hs', value: '%p', count: '%d') [Thread: '%s'] => Returned: '%s'"),
				TouchEngineInstance.get(),
				IdentifierAsCStr,
				Op.GetData(),
				LinkInfo->count,
				*GetCurrentThreadStr(),
				*TEResultToString(Result)
			);

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

			if (Op.Num() > LinkInfo->count)
			{
				ErrorLog->AddCountMismatchWarning(LinkInfo, Op.Num(), Identifier, GET_FUNCTION_NAME_CHECKED(FTouchVariableManager, SetIntegerInput));
			}
			else if (Op.Num() < LinkInfo->count)
			{
				ErrorLog->AddCountMismatchError(LinkInfo, Op.Num(), Identifier, GET_FUNCTION_NAME_CHECKED(FTouchVariableManager, SetIntegerInput));
				return;
			}

			const TEResult Result = TEInstanceLinkSetIntValue(TouchEngineInstance, IdentifierAsCStr, Op.GetData(), LinkInfo->count);
			UE_LOG(LogTouchEngineTECalls, Log, TEXT("  TEInstanceLinkSetIntValue(TEInstance: '%p', identifier: '%hs', value: '%p', count: '%d') [Thread: '%s'] => Returned: '%s'"),
				TouchEngineInstance.get(),
				IdentifierAsCStr,
				Op.GetData(),
				LinkInfo->count,
				*GetCurrentThreadStr(),
				*TEResultToString(Result)
			);

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
				UE_LOG(LogTouchEngineTECalls, Log, TEXT("  TEInstanceLinkSetStringValue(TEInstance: '%p', identifier: '%hs', value: '%p') [Thread: '%s'] => Returned: '%s'"),
					TouchEngineInstance.get(),
					IdentifierAsCStr,
					Op,
					*GetCurrentThreadStr(),
					*TEResultToString(Result)
				);

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
				UE_LOG(LogTouchEngineTECalls, Log, TEXT("  TETableResize(TETable: '%p', rows: '1', columns: '1') [Thread: '%s']"),
					Table.get(),
					*GetCurrentThreadStr()
				);
				TEResult Result = TETableSetStringValue(Table, 0, 0, Op);
				UE_LOG(LogTouchEngineTECalls, Log, TEXT("  TETableSetStringValue(TETable: '%p', row: '0', column: '0', value: '%p') [Thread: '%s'] => Returned: '%s'"),
					Table.get(),
					Op,
					*GetCurrentThreadStr(),
					*TEResultToString(Result)
				);
				
				if (Result != TEResultSuccess)
				{
					ErrorLog->AddResult(FTouchErrorLog::EErrorType::TEInstanceLinkSetValueError, Result, Identifier, GET_FUNCTION_NAME_CHECKED(FTouchVariableManager, SetStringInput),
						TEXT("Tried to set a String value in a Table."));
				}
				
				Result = TEInstanceLinkSetTableValue(TouchEngineInstance, IdentifierAsCStr, Table);
				UE_LOG(LogTouchEngineTECalls, Log, TEXT("  TEInstanceLinkSetTableValue(TEInstance: '%p', identifier: '%hs', TETable: '%p') [Thread: '%s'] => Returned: '%s'"),
					TouchEngineInstance.get(),
					IdentifierAsCStr,
					Table.get(),
					*GetCurrentThreadStr(),
					*TEResultToString(Result)
				);
				
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
				UE_LOG(LogTouchEngineTECalls, Log, TEXT("  TETableGetStringValue(TETable: '%p', row: '0', column: '0') [Thread: '%s'] => Returned: '%p'"),
					Op.TableData.get(),
					*GetCurrentThreadStr(),
					String
				);
				const TEResult Result = TEInstanceLinkSetStringValue(TouchEngineInstance, IdentifierAsCStr, String);
				UE_LOG(LogTouchEngineTECalls, Log, TEXT("  TEInstanceLinkSetStringValue(TEInstance: '%p', identifier: '%hs', value: '%p') [Thread: '%s'] => Returned: '%s'"),
					TouchEngineInstance.get(),
					IdentifierAsCStr,
					String,
					*GetCurrentThreadStr(),
					*TEResultToString(Result)
				);

				if (Result != TEResultSuccess)
				{
					ErrorLog->AddResult(FTouchErrorLog::EErrorType::TEInstanceLinkSetValueError, Result, Identifier, GET_FUNCTION_NAME_CHECKED(FTouchVariableManager, SetTableInput),
						TEXT("Tried to set a String."));
				}
			}
			else if (LinkInfo->type == TELinkTypeStringData)
			{
				const TEResult Result = TEInstanceLinkSetTableValue(TouchEngineInstance, IdentifierAsCStr, Op.TableData);
				UE_LOG(LogTouchEngineTECalls, Log, TEXT("  TEInstanceLinkSetTableValue(TEInstance: '%p', identifier: '%hs', TETable: '%p') [Thread: '%s'] => Returned: '%s'"),
					TouchEngineInstance.get(),
					IdentifierAsCStr,
					Op.TableData.get(),
					*GetCurrentThreadStr(),
					*TEResultToString(Result)
				);
				
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
