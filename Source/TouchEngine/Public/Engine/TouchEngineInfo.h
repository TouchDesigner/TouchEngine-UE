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
#include "TouchEngineDynamicVariableStruct.h"
#include "TouchEngineInfo.generated.h"

class UTexture2D;

namespace UE::TouchEngine
{
	struct FTouchLoadResult;
	class FTouchEngine;
    struct FCookFrameRequest;
    struct FCookFrameResult;
}


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

	TFuture<UE::TouchEngine::FTouchLoadResult> LoadTox(const FString& AbsolutePath);
	bool Unload();
	void Destroy();
	
	FTouchCHOPFull GetCHOPOutputSingleSample(const FString& Identifier);
	UTexture2D* GetTOPOutput(const FString& Identifier);
	FTouchDATFull GetTableOutput(const FString& Identifier);
	TTouchVar<bool> GetBooleanOutput(const FString& Identifier);
	TTouchVar<double> GetDoubleOutput(const FString& Identifier);
	TTouchVar<int32> GetIntegerOutput(const FString& Identifier);
	TTouchVar<TEString*> GetStringOutput(const FString& Identifier);

	void SetTableInput(const FString& Identifier, FTouchDATFull& Op);
	void SetCHOPInputSingleSample(const FString& Identifier, const FTouchCHOPSingleSample& Chop);
	/**
	 * @param bReuseExistingTexture Set this to true if you never change the content of Texture (e.g. the pixels).
	 * If Texture was used as parameter in the past, this parameter determines whether it is safe to reuse that data.
	 * In that case, we will skip allocating a new texture resource and copying Texture into it:
	 * we'll just return the existing resource.
	 */
	void SetTOPInput(const FString& Identifier, UTexture* Texture, bool bReuseExistingTexture = true);
	void SetDoubleInput(const FString& Identifier, TTouchVar<TArray<double>>& Op);
	void SetIntegerInput(const FString& Identifier, TTouchVar<TArray<int32>>& Op);
	void SetBooleanInput(const FString& Identifier, TTouchVar<bool>& Op);
	void SetStringInput(const FString& Identifier, TTouchVar<const char*>& Op);

	TFuture<UE::TouchEngine::FCookFrameResult> CookFrame_GameThread(const UE::TouchEngine::FCookFrameRequest& CookFrameRequest);
	void LogTouchEngineError(const FString& Error);
	bool GetSupportedPixelFormats(TSet<TEnumAsByte<EPixelFormat>>& SupportedPixelFormat) const;

	FTouchOnLoadFailed* GetOnLoadFailedDelegate();
	FTouchOnParametersLoaded* GetOnParametersLoadedDelegate();

	TSharedPtr<UE::TouchEngine::FTouchEngine> Engine = nullptr;
};
