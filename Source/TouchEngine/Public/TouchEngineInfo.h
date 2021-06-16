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
#include "TouchEngineInfo.generated.h"

class UTouchEngine;
struct FTouchCHOPSingleSample;
struct FTouchCHOPFull;
struct FTouchDATFull;
struct FTouchTOP;
template <typename T>
struct FTouchVar;
struct TEString;

typedef void TEObject;
typedef struct TETable_ TETable;

DECLARE_MULTICAST_DELEGATE(FTouchOnLoadComplete);
DECLARE_MULTICAST_DELEGATE_OneParam(FTouchOnLoadFailed, FString);
DECLARE_MULTICAST_DELEGATE_TwoParams(FTouchOnParametersLoaded, const TArray<FTouchEngineDynamicVariableStruct>&, const TArray<FTouchEngineDynamicVariableStruct>&);

/*
 * Interface to handle the TouchEngine instance 
 */
UCLASS(Category = "TouchEngine", DisplayName = "TouchEngineInfo Instance")
class TOUCHENGINE_API UTouchEngineInfo : public UObject
{
	GENERATED_BODY()

	friend class UTouchOnLoadTask;

public:

	UTouchEngineInfo();
	// creates a TouchEngine instance and loads a tox file 
	bool		Load(FString toxPath);
	// clears the TouchEngine instance of a loaded tox file
	void		Clear();
	// destroys the TouchEngine instance
	void		Destroy();
	// returns the loaded tox path of the current TouchEngine instance
	FString		GetToxPath() const;
	// sets the cook mode of the current TouchEngine instance
	bool		SetCookMode(bool isIndependent);
	// sets the target frame rate of the current TouchEngineInstnace
	bool		SetFrameRate(int64 frameRate);
	// returns a single frame of a channel operator output
	FTouchCHOPFull	GetCHOPOutputSingleSample(const FString &Identifier);
	// sets a single frame of a channel operator input
	void			SetCHOPInputSingleSample(const FString &Identifier, const FTouchCHOPSingleSample &chop);
	// gets a texture operator output
	FTouchTOP		GetTOPOutput(const FString &Identifier);
	// sets a texture operator input
	void			SetTOPInput(const FString &Identifier, UTexture *texture);
	// gets a boolean output
	FTouchVar<bool>				GetBooleanOutput(const FString& Identifier);
	// sets a boolean input
	void						SetBooleanInput(const FString& Identifier, FTouchVar<bool>& Op);
	// gets a double output
	FTouchVar<double>			GetDoubleOutput(const FString& Identifier);
	// sets a double input
	void						SetDoubleInput(const FString& Identifier, FTouchVar<TArray<double>>& Op);
	// gets an integer output
	FTouchVar<int32_t>			GetIntegerOutput(const FString& Identifier);
	// sets an integer input
	void						SetIntegerInput(const FString& Identifier, FTouchVar<TArray<int32_t>>& Op);
	// gets a string output
	FTouchVar<TEString*>		GetStringOutput(const FString& Identifier);
	// sets a string input
	void						SetStringInput(const FString& Identifier, FTouchVar<char*>& Op);
	// gets a table output
	FTouchDATFull				GetTableOutput(const FString& Identifier);
	// sets a table input
	void						SetTableInput(const FString& Identifier, FTouchDATFull& Op);

	// starts the cook for a frame 
	void		CookFrame(int64 FrameTime_Mill);
	// returns whether or not the tox file has been loaded
	bool		IsLoaded();
	// returns whether the last call to cook frame has finished
	bool		IsCookComplete();
	// returns whether or not the tox file has failed to load
	bool		HasFailedLoad();
	// logs an error with both TouchEngine and UE4
	void		LogTouchEngineError(FString Error);
	// returns whether or not the engine instance is running
	bool		IsRunning();

	// returns the on load complete delegate from the TouchEngine instnace
	//FTouchOnLoadComplete* getOnLoadCompleteDelegate();
	// returns the on load failed delegate from the TouchEngine instnace
	FTouchOnLoadFailed* GetOnLoadFailedDelegate();
	// returns the on parameters loaded delegate from the TouchEngine instnace
	FTouchOnParametersLoaded* GetOnParametersLoadedDelegate();

	FString GetFailureMessage();

	TArray<FString> GetCHOPChannelNames(FString Identifier);

private:
	// TouchEngine instance
	UPROPERTY(Transient)
	UTouchEngine*			Engine = nullptr;
	// absolute tox file path
	FString					MyToxFile;

	int64 CookStartFrame;

public:

};