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

#pragma once

#include "CoreMinimal.h"
#include "Async/Future.h"
#include "TouchEngineDynamicVariableStruct.h"
#include "Blueprint/TouchEngineInputFrameData.h"
#include "Engine/TouchVariables.h"
#include "TouchEngine/TouchObject.h"
#include "TouchEngine/TEInstance.h"

namespace UE::TouchEngine
{
	class FTouchErrorLog;
	class FTouchResourceProvider;
	
	using FInputTextureUpdateId = int64;

	struct FTextureInputUpdateInfo
	{
		FName Texture;
		FInputTextureUpdateId TextureUpdateId;
	};

	enum class ETextureUpdateErrorCode
	{
		Success,
		Cancelled
	};

	struct FFinishTextureUpdateInfo
	{
		ETextureUpdateErrorCode ErrorCode;
	};

	class FTouchVariableManager : public TSharedFromThis<FTouchVariableManager>
	{
	public:
		FTouchVariableManager(TouchObject<TEInstance> TouchEngineInstance, TSharedPtr<FTouchResourceProvider> ResourceProvider, const TSharedPtr<FTouchErrorLog>& ErrorLog);
		~FTouchVariableManager();

		void AllocateLinkedTop(FName ParamName);
		/**
		 * Update the TOP with the given texture. Returns the previous texture which can be reused in the texture pool
		 */
		UTexture2D* UpdateLinkedTOP(FName ParamName, UTexture2D* Texture);
		
		FTouchEngineCHOP GetCHOPOutputSingleSample(const FString& Identifier);
		FTouchEngineCHOP GetCHOPOutput(const FString& Identifier);
		UTexture2D* GetTOPOutput(const FString& Identifier);
		bool GetBooleanOutput(const FString& Identifier);
		double GetDoubleOutput(const FString& Identifier);
		int32_t GetIntegerOutput(const FString& Identifier);
		TouchObject<TEString> GetStringOutput(const FString& Identifier);
		FTouchDATFull GetTableOutput(const FString& Identifier) const;
		TArray<FString> GetCHOPChannelNames(const FString& Identifier) const;

		void SetCHOPInputSingleSample(const FString& Identifier, const FTouchEngineCHOPChannel& CHOPChannel);
		void SetCHOPInput(const FString& Identifier, const FTouchEngineCHOP& CHOP);
		TFuture<bool> SetTOPInput(const FString& Identifier, const TSharedPtr<FExportedTouchTexture>& Texture, const FTouchEngineInputFrameData& FrameData);
		void SetBooleanInput(const FString& Identifier, const bool& Op);
		void SetDoubleInput(const FString& Identifier, const TArray<double>& Op);
		void SetIntegerInput(const FString& Identifier, const TArray<int32_t>& Op);
		void SetStringInput(const FString& Identifier, const char*& Op);
		void SetTableInput(const FString& Identifier, const FTouchDATFull& Op);

		/** Sets in which frame a TouchEngine Parameter was last updated. This should come from a LinkValue Callback */
		void SetFrameLastUpdatedForParameter(const FString& Identifier, int64 FrameID);
		int64 GetFrameLastUpdatedForParameter(const FString& Identifier);

		/** Empty the saved data. Should be called before trying to close TE to be sure we do not keep hold on any pointer */
		void ClearSavedData();
		void ResetTouchEngineInstance() { TouchEngineInstance.reset(); }

		const TSharedPtr<FTouchErrorLog>& GetErrorLog() { return ErrorLog; }
	private:
		struct FInputTextureUpdateTask
		{
			FInputTextureUpdateId TaskId;
			/** It's done but we're waiting to notify the listeners. */
			bool bIsAwaitingFinalisation = false;
		};

		TouchObject<TEInstance> TouchEngineInstance;
		TSharedPtr<FTouchResourceProvider> ResourceProvider;
		TSharedPtr<FTouchErrorLog> ErrorLog;

		TMap<FString, FTouchEngineCHOPChannel> CHOPChannelOutputs;
		TMap<FString, FTouchEngineCHOP> CHOPOutputs;
		TMap<FName, TouchObject<TETexture>> TOPInputs;
		FCriticalSection TOPInputsLock;
		TMap<FName, UTexture2D*> TOPOutputs;
		FCriticalSection TOPOutputsLock;

		/** The FrameID the parameters were last updated */
		TMap<FString, int64> LastFrameParameterUpdated; //todo: could this be a FName? we would need more guarantees on what names can be given to TouchEngine parameters to ensure no clashes

		/**
		 * Helper Function to call TEInstanceLinkGetInfo and take care of common error logging.
		 * Returns true if TEInstanceLinkGetInfo was successful, as well as the expected scope and type matches.
		 */
		bool GetLinkInfo(const FString& Identifier,TouchObject<TELinkInfo>& LinkInfo, TEScope ExpectedScope, TELinkType ExpectedType, const FName& FunctionName) const;
		/**
		 * Helper Function to call TEInstanceLinkGetInfo and take care of common error logging.
		 * Returns true if TEInstanceLinkGetInfo was successful, as well as the expected scope and type matches.
		 * This overload does not check for the type, if multiple types can be specified for example
		 */
		bool GetLinkInfo(const FString& Identifier,TouchObject<TELinkInfo>& LinkInfo, TEScope ExpectedScope, const FName& FunctionName) const;
	};
}
