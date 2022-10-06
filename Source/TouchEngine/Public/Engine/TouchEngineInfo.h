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
#include "TouchEngineDynamicVariableStruct.h"
#include "TouchEngineInfo.generated.h"

namespace UE
{
	namespace TouchEngine
	{
		struct FCookFrameRequest;
		struct FCookFrameResult;
	}
}

class UTouchEngine;
struct FTouchCHOPSingleSample;
struct FTouchCHOPFull;
struct FTouchDATFull;
struct FTouchTOP;
template <typename T>
struct TTouchVar;
struct TEString;

typedef void TEObject;
typedef struct TETable_ TETable;

DECLARE_MULTICAST_DELEGATE(FTouchOnLoadComplete);
DECLARE_MULTICAST_DELEGATE_OneParam(FTouchOnLoadFailed, const FString&);
DECLARE_MULTICAST_DELEGATE_TwoParams(FTouchOnParametersLoaded, const TArray<FTouchEngineDynamicVariableStruct>&, const TArray<FTouchEngineDynamicVariableStruct>&);

/*
 * Interface to handle the TouchEngine instance
 */
UCLASS(Category = "TouchEngine", DisplayName = "TouchEngineInfo Instance")
class TOUCHENGINE_API UTouchEngineInfo : public UObject
{
	GENERATED_BODY()
public:

	UTouchEngineInfo();

	/** @param AbsoluteOrRelativeToxPath Path to tox file - can be relative to content folder */
	bool Load(const FString& AbsoluteOrRelativeToxPath);
	bool Unload();
	void Clear_GameThread();
	void Destroy();
	
	FString GetToxPath() const;
	
	void SetCookMode(bool IsIndependent);
	bool SetFrameRate(int64 FrameRate);
	
	FTouchCHOPFull GetCHOPOutputSingleSample(const FString& Identifier);
	UTexture2D* GetTOPOutput(const FString& Identifier);
	FTouchDATFull GetTableOutput(const FString& Identifier);
	TTouchVar<bool> GetBooleanOutput(const FString& Identifier);
	TTouchVar<double> GetDoubleOutput(const FString& Identifier);
	TTouchVar<int32> GetIntegerOutput(const FString& Identifier);
	TTouchVar<TEString*> GetStringOutput(const FString& Identifier);
	TArray<FString> GetCHOPChannelNames(const FString& Identifier) const;
	
	void SetTableInput(const FString& Identifier, FTouchDATFull& Op);
	void SetCHOPInputSingleSample(const FString& Identifier, const FTouchCHOPSingleSample& Chop);
	void SetTOPInput(const FString& Identifier, UTexture* Texture);
	void SetDoubleInput(const FString& Identifier, TTouchVar<TArray<double>>& Op);
	void SetIntegerInput(const FString& Identifier, TTouchVar<TArray<int32>>& Op);
	void SetBooleanInput(const FString& Identifier, TTouchVar<bool>& Op);
	void SetStringInput(const FString& Identifier, TTouchVar<char*>& Op);

	TFuture<UE::TouchEngine::FCookFrameResult> CookFrame_GameThread(const UE::TouchEngine::FCookFrameRequest& CookFrameRequest);
	bool IsLoaded() const;
	bool IsLoading() const;
	bool HasFailedLoad() const;
	void LogTouchEngineError(const FString& Error);
	bool IsRunning() const;

	FTouchOnLoadFailed* GetOnLoadFailedDelegate();
	FTouchOnParametersLoaded* GetOnParametersLoadedDelegate();

	FString GetFailureMessage() const;

private:
	
	UPROPERTY(Transient)
	TObjectPtr<UTouchEngine> Engine = nullptr;
};
