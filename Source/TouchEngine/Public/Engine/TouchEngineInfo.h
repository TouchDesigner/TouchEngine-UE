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
#include "PixelFormat.h"
#include "Async/Future.h"
#include "Engine/Texture.h"
#include "Engine/TouchEngine.h"
#include "TouchEngine/TouchObject.h"
#include "Util/CookFrameData.h"
#include "Util/TouchErrorLog.h"
#include "TouchEngineInfo.generated.h"

class UTexture2D;

namespace UE::TouchEngine
{
	struct FTouchLoadResult;
    struct FCookFrameRequest;
    struct FCookFrameResult;
}


struct FTouchEngineCHOPChannel;
struct FTouchEngineCHOP;
struct FTouchDATFull;
struct FTouchTOP;
template <typename T>
struct TTouchVar;
struct TEString;

typedef void TEObject;
typedef struct TETable_ TETable;

/*
 * Interface to handle the TouchEngine instance
 */
UCLASS(Category = "TouchEngine", DisplayName = "TouchEngineInfo Instance")
class TOUCHENGINE_API UTouchEngineInfo : public UObject
{
	GENERATED_BODY()
public:

	UTouchEngineInfo();

	TFuture<UE::TouchEngine::FTouchLoadResult> LoadTox(const FString& AbsolutePath, class UTouchEngineComponentBase* Component, double TimeoutInSeconds = -1.0);
	bool Unload();
	void Destroy();
	
	FTouchEngineCHOP GetCHOPOutput(const FString& Identifier) const;
	UTexture2D* GetTOPOutput(const FString& Identifier) const;
	FTouchDATFull GetTableOutput(const FString& Identifier) const;
	bool GetBooleanOutput(const FString& Identifier) const;
	double GetDoubleOutput(const FString& Identifier) const;
	int32 GetIntegerOutput(const FString& Identifier) const;
	TouchObject<TEString> GetStringOutput(const FString& Identifier) const;

	int64 GetFrameLastUpdatedForParameter(const FString& Identifier) const;
	
	/**
	 * Enqueue the given FCookFrameRequest to be cooked by TouchEngine and start the next one in the queue if none are ongoing.
	 * @param CookFrameRequest The CookFrameRequest
	 * @param InputBufferLimit  Sets the maximum number of cooks we will enqueue while another cook is processing by TouchEngine. If the limit is reached, older cooks will be discarded.
	 * If set to less than 0, there will be no limit to the amount of cooks enqueued.
	 * @return 
	 */
	TFuture<UE::TouchEngine::FCookFrameResult> CookFrame_GameThread(UE::TouchEngine::FCookFrameRequest&& CookFrameRequest, int32 InputBufferLimit);
	/** Execute the next queued CookFrameRequest if no cook is on going */
	bool ExecuteNextPendingCookFrame_GameThread() const;
	
	bool IsCookingFrame() const;
	
	void LogTouchEngineWarning(const FString& Message, const FString& VarName = FString(), const FName& FunctionName = FName(), const FString& AdditionalDescription = FString()) const;
	void LogTouchEngineWarning(UE::TouchEngine::FTouchErrorLog::EErrorType ErrorType, const FString& VarName = FString(), const FName& FunctionName = FName(), const FString& AdditionalDescription = FString()) const;
	void LogTouchEngineError(const FString& Message, const FString& VarName = FString(), const FName& FunctionName = FName(), const FString& AdditionalDescription = FString()) const;
	void LogTouchEngineError(UE::TouchEngine::FTouchErrorLog::EErrorType ErrorType, const FString& VarName = FString(), const FName& FunctionName = FName(), const FString& AdditionalDescription = FString()) const;

	bool GetSupportedPixelFormats(TSet<TEnumAsByte<EPixelFormat>>& SupportedPixelFormat) const;
	void CancelCurrentAndNextCooks_GameThread(ECookFrameResult CookFrameResult = ECookFrameResult::Cancelled);
	/**
	 * Cancel the current Frame if it matches the given FrameID
	 * @param FrameID The FrameID of the Frame to cancel. As parts of the code is asynchronous, this is to ensure we are cancelling the right frame. Pass -1 to cancel the current frame
	 * @param CookFrameResult The Result to give back to the user
	 * @return Returns true if the frame with the GivenID was cancelled
	 */
	bool CancelCurrentFrame_GameThread(int64 FrameID, ECookFrameResult CookFrameResult = ECookFrameResult::Cancelled);
	bool CheckIfCookTimedOut_GameThread(double CookTimeoutInSeconds);

	TSharedRef<UE::TouchEngine::FTouchEngine> Engine;
};
