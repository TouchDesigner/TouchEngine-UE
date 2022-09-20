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

	friend class UTouchEngineComponentBase;

public:

	UTouchEngineInfo();

	// preloads a TouchEngine instance
	bool		PreLoad();
	// preloads a TouchEngine instance
	bool		PreLoad(const FString& ToxPath);
	// creates a TouchEngine instance and loads a tox file
	bool		Load(FString ToxPath);
	// unloads a TouchEngine instance
	bool		Unload();
	// clears the TouchEngine instance of a loaded tox file
	void		Clear();
	// destroys the TouchEngine instance
	void		Destroy();
	// returns the loaded tox path of the current TouchEngine instance
	FString		GetToxPath() const;
	// sets the cook mode of the current TouchEngine instance
	bool		SetCookMode(bool IsIndependent);
	// sets the target frame rate of the current TouchEngineInstance
	bool		SetFrameRate(int64 FrameRate);
	// returns a single frame of a channel operator output
	FTouchCHOPFull	GetCHOPOutputSingleSample(const FString& Identifier);
	// sets a single frame of a channel operator input
	void			SetCHOPInputSingleSample(const FString& Identifier, const FTouchCHOPSingleSample& Chop);
	// gets a texture operator output
	FTouchTOP		GetTOPOutput(const FString& Identifier);
	// sets a texture operator input
	void			SetTOPInput(const FString& Identifier, UTexture* Texture);
	// gets a boolean output
	TTouchVar<bool>				GetBooleanOutput(const FString& Identifier);
	// sets a boolean input
	void						SetBooleanInput(const FString& Identifier, TTouchVar<bool>& Op);
	// gets a double output
	TTouchVar<double>			GetDoubleOutput(const FString& Identifier);
	// sets a double input
	void						SetDoubleInput(const FString& Identifier, TTouchVar<TArray<double>>& Op);
	// gets an integer output
	TTouchVar<int32>			GetIntegerOutput(const FString& Identifier);
	// sets an integer input
	void						SetIntegerInput(const FString& Identifier, TTouchVar<TArray<int32>>& Op);
	// gets a string output
	TTouchVar<TEString*>		GetStringOutput(const FString& Identifier);
	// sets a string input
	void						SetStringInput(const FString& Identifier, TTouchVar<char*>& Op);
	// gets a table output
	FTouchDATFull				GetTableOutput(const FString& Identifier);
	// sets a table input
	void						SetTableInput(const FString& Identifier, FTouchDATFull& Op);

	// starts the cook for a frame
	void		CookFrame(int64 FrameTime_Mill);
	// returns whether or not the tox file has been loaded
	bool		IsLoaded() const;
	// returns whether or not the tox file is still loading
	bool		IsLoading() const;
	// returns whether the last call to cook frame has finished
	bool		IsCookComplete() const;
	// returns whether or not the tox file has failed to load
	bool		HasFailedLoad() const;
	// logs an error with both TouchEngine and UE
	void		LogTouchEngineError(const FString& Error);
	// returns whether or not the engine instance is running
	bool		IsRunning() const;

	FTouchOnLoadFailed* GetOnLoadFailedDelegate();
	FTouchOnParametersLoaded* GetOnParametersLoadedDelegate();

	FString GetFailureMessage() const;

	TArray<FString> GetCHOPChannelNames(const FString& Identifier) const;

private:
	// TouchEngine instance
	UPROPERTY(Transient)
	TObjectPtr<UTouchEngine> Engine = nullptr;

	// absolute tox file path
	FString	MyToxFile;

	// frame the last cook started on
	int64 WaitStartFrame = 0;
};
