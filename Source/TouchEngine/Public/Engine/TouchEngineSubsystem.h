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
#include "ToxDelegateInfo.h"
#include "Subsystems/EngineSubsystem.h"
#include "TouchEngineSubsystem.generated.h"

class UTouchEngineInfo;

/** Keeps a global list of loaded tox files. */
UCLASS()
class TOUCHENGINE_API UTouchEngineSubsystem : public UEngineSubsystem
{
	GENERATED_BODY()
	friend class UFileParams;
public:

	//~ Begin UEngineSubsystem Interface
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	//~ End UEngineSubsystem Interface

	/** Calls the passed in delegate when the parameters for the specified tox path have been loaded */
	void GetOrLoadParamsFromTox(FString ToxPath, UObject* Owner, FTouchOnParametersLoaded::FDelegate ParamsLoadedDel, FTouchOnFailedLoad::FDelegate LoadFailedDel, FDelegateHandle& ParamsLoadedDelHandle, FDelegateHandle& LoadFailedDelHandle);
	TObjectPtr<UFileParams> GetParamsFromToxIfLoaded(FString ToxPath);

	/** Gives ans answer whether or not the given EPixelFormat is a supported one for this ResourceProvider. */
	bool IsSupportedPixelFormat(EPixelFormat PixelFormat) const;
	
	/** Deletes the parameters associated with the passed in tox path and attempts to reload */
	bool ReloadTox(const FString& ToxPath, UObject* Owner, FTouchOnParametersLoaded::FDelegate ParamsLoadedDel, FTouchOnFailedLoad::FDelegate LoadFailedDel, FDelegateHandle& ParamsLoadedDelHandle, FDelegateHandle& LoadFailedDelHandle);
	
	bool IsLoaded(const FString& ToxPath) const;
	bool HasFailedLoad(const FString& ToxPath) const;
	
	/** Attempts to unbind the passed in handles from any UFileParams they may be bound to */
	bool UnbindDelegates(FDelegateHandle ParamsLoadedDelHandle, FDelegateHandle LoadFailedDelHandle);
	
private:
	
	/** Tox files that still need to be loaded */
	UPROPERTY(Transient)
	TMap<FString, FToxDelegateInfo> CachedToxPaths;

	/** List of Supported EPixelFormat */
	UPROPERTY(Transient)
	TSet<TEnumAsByte<EPixelFormat>> CachedSupportedPixelFormats;

	/** TouchEngine instance used to load items into the details panel */
	UPROPERTY(Transient)
	TObjectPtr<UTouchEngineInfo> TempEngineInfo;

	/** Map of files loaded to their parameters */
	UPROPERTY(Transient)
	TMap<FString, TObjectPtr<UFileParams>> LoadedParams;

	/** Loads a tox file and stores its parameters in LoadedParams. */
	UFileParams* LoadTox(FString ToxPath, UObject* Owner, FTouchOnParametersLoaded::FDelegate ParamsLoadedDel, FTouchOnFailedLoad::FDelegate LoadFailedDel, FDelegateHandle& ParamsLoadedDelHandle, FDelegateHandle& LoadFailedDelHandle);

	void LoadNext();
};
