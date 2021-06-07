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
DECLARE_MULTICAST_DELEGATE_TwoParams(FTouchOnParametersLoaded, TArray<FTouchEngineDynamicVariableStruct>, TArray<FTouchEngineDynamicVariableStruct>);

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
	bool		load(FString toxPath);
	// clears the TouchEngine instance of a loaded tox file
	void		clear();
	// destroys the TouchEngine instance
	void		destroy();
	// returns the loaded tox path of the current TouchEngine instance
	FString		getToxPath() const;
	// sets the cook mode of the current TouchEngine instance
	bool		setCookMode(bool isIndependent);
	// sets the target frame rate of the current TouchEngineInstnace
	bool		setFrameRate(int64 frameRate);
	// returns a single frame of a channel operator output
	FTouchCHOPFull	getCHOPOutputSingleSample(const FString &identifier);
	// sets a single frame of a channel operator input
	void			setCHOPInputSingleSample(const FString &identifier, const FTouchCHOPSingleSample &chop);
	// gets a texture operator output
	FTouchTOP		getTOPOutput(const FString &identifier);
	// sets a texture operator input
	void			setTOPInput(const FString &identifier, UTexture *texture);
	// gets a boolean output
	FTouchVar<bool>				getBooleanOutput(const FString& identifier);
	// sets a boolean input
	void						setBooleanInput(const FString& identifier, FTouchVar<bool>& op);
	// gets a double output
	FTouchVar<double>			getDoubleOutput(const FString& identifier);
	// sets a double input
	void						setDoubleInput(const FString& identifier, FTouchVar<TArray<double>>& op);
	// gets an integer output
	FTouchVar<int32_t>			getIntegerOutput(const FString& identifier);
	// sets an integer input
	void						setIntegerInput(const FString& identifier, FTouchVar<TArray<int32_t>>& op);
	// gets a string output
	FTouchVar<TEString*>		getStringOutput(const FString& identifier);
	// sets a string input
	void						setStringInput(const FString& identifier, FTouchVar<char*>& op);
	// gets a table output
	FTouchDATFull				getTableOutput(const FString& identifier);
	// sets a table input
	void						setTableInput(const FString& identifier, FTouchDATFull& op);

	// starts the cook for a frame 
	void		cookFrame(int64 FrameTime_Mill);
	// returns whether or not the tox file has been loaded
	bool		isLoaded();
	// returns whether the last call to cook frame has finished
	bool		isCookComplete();
	// returns whether or not the tox file has failed to load
	bool		hasFailedLoad();
	// logs an error with both TouchEngine and UE4
	void		logTouchEngineError(FString error);
	// returns whether or not the engine instance is running
	bool		isRunning();

	// returns the on load complete delegate from the TouchEngine instnace
	//FTouchOnLoadComplete* getOnLoadCompleteDelegate();
	// returns the on load failed delegate from the TouchEngine instnace
	FTouchOnLoadFailed* getOnLoadFailedDelegate();
	// returns the on parameters loaded delegate from the TouchEngine instnace
	FTouchOnParametersLoaded* getOnParametersLoadedDelegate();

	FString getFailureMessage();

	TArray<FString> GetCHOPChannelNames(FString Identifier);

private:
	// TouchEngine instance
	UPROPERTY(Transient)
	UTouchEngine*			engine = nullptr;
	// absolute tox file path
	FString					myToxFile;

	int64 cookStartFrame;

public:

};