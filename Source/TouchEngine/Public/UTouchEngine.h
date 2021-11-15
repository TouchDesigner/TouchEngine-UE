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
#include "TouchEngine/TouchEngine.h"
#include <deque>
#include "Windows/AllowWindowsPlatformTypes.h"
#include <d3d11.h>
#include "Windows/HideWindowsPlatformTypes.h"
#include "Engine/Texture2D.h"
#include "Logging/MessageLog.h"
#include "UTouchEngine.generated.h"

class UTouchEngineInfo;
struct FTouchEngineDynamicVariableStruct;

template <typename T>
struct FTouchVar
{
	T Data;
};

struct FTouchCHOPSingleSample
{
	TArray<float>	ChannelData;
	FString ChannelName;
};

struct FTouchCHOPFull
{
	TArray<FTouchCHOPSingleSample> SampleData;
};

struct FTouchDATFull
{
	TETable* ChannelData = nullptr;
	TArray<FString> RowNames;
	TArray<FString> ColumnNames;
};

struct FTouchTOP
{
	UTexture2D*		Texture = nullptr;
	ID3D11Resource* WrappedResource = nullptr;
};


DECLARE_MULTICAST_DELEGATE(FTouchOnLoadComplete);
DECLARE_MULTICAST_DELEGATE_OneParam(FTouchOnLoadFailed, FString);
DECLARE_MULTICAST_DELEGATE_TwoParams(FTouchOnParametersLoaded, const TArray<FTouchEngineDynamicVariableStruct>&, const TArray<FTouchEngineDynamicVariableStruct>&);
DECLARE_MULTICAST_DELEGATE(FTouchOnCookFinished);

UCLASS()
class TOUCHENGINE_API UTouchEngine : public UObject
{
	friend class UTouchEngineInfo;

	GENERATED_BODY()

	virtual void				BeginDestroy() override;

	void						Clear();

public:

	~UTouchEngine();
	void						Copy(UTouchEngine* Other);

	void						LoadTox(FString ToxPath);
	const FString&				GetToxPath() const;

	void						CookFrame(int64 FrameTime_Mill);

	bool						SetCookMode(bool IsIndependent);

	bool						SetFrameRate(int64 FrameRate);

	FTouchCHOPFull				GetCHOPOutputSingleSample(const FString& Identifier);
	void						SetCHOPInputSingleSample(const FString &Identifier, const FTouchCHOPSingleSample& CHOP);
	FTouchCHOPFull				GetCHOPOutputs(const FString& Identifier);
	void						SetCHOPInput(const FString& Identifier, const FTouchCHOPFull& CHOP);

	FTouchTOP					GetTOPOutput(const FString& Identifier);
	void						SetTOPInput(const FString& Identifier, UTexture *Texture);

	FTouchVar<bool>				GetBooleanOutput(const FString& Identifier);
	void						SetBooleanInput(const FString& Identifier, FTouchVar<bool>& Op);
	FTouchVar<double>			GetDoubleOutput(const FString& Identifier);
	void						SetDoubleInput(const FString& Identifier, FTouchVar<TArray<double>>& Op);	
	FTouchVar<int32_t>			GetIntegerOutput(const FString& Identifier);
	void						SetIntegerInput(const FString& Identifier, FTouchVar<TArray<int32_t>>& Op);
	FTouchVar<TEString*>		GetStringOutput(const FString& Identifier);
	void						SetStringInput(const FString& Identifier, FTouchVar<char*>& Op);
	FTouchDATFull				GetTableOutput(const FString& Identifier);
	void						SetTableInput(const FString& Identifier, FTouchDATFull& Op);

	void						SetDidLoad() { MyDidLoad = true; }

	bool						GetDidLoad() {	return MyDidLoad; }

	bool						GetFailedLoad() { return MyFailedLoad; }

	FTouchOnLoadFailed OnLoadFailed;
	FTouchOnParametersLoaded OnParametersLoaded;
	FTouchOnCookFinished OnCookFinished;

	FString FailureMessage;

private:

	class TexCleanup
	{
	public:
		ID3D11Query*	Query = nullptr;
		TED3D11Texture*	Texture = nullptr;
	};

	enum class FinalClean
	{
		False,
		True
	};

	enum class RHIType
	{
		Invalid,
		DirectX11,
		DirectX12
	};


	static void		EventCallback(TEInstance* Instance, TEEvent Event, TEResult Result, int64_t StartTimeValue, int32_t StartTimeScale, int64_t EndTimeValue, int32_t EndTimeScale, void* Info);

	void			AddResult(const FString& s, TEResult Result);
	void			AddError(const FString& s);
	void			AddWarning(const FString& s);

	void			OutputMessages();

	void			OutputResult(const FString& s, TEResult Result);
	void			OutputError(const FString& s);
	void			OutputWarning(const FString& s);

	static void		CleanupTextures(ID3D11DeviceContext* context, std::deque<TexCleanup> *Cleanups, FinalClean FC);
	static void		LinkValueCallback(TEInstance* Instance, TELinkEvent Event, const char* Identifier, void* Info);
	void			LinkValueCallback(TEInstance* Instance, TELinkEvent Event, const char *Identifier);

	TEResult		ParseGroup(TEInstance* Instance, const char* Identifier, TArray<FTouchEngineDynamicVariableStruct>& Variables);
	TEResult		ParseInfo(TEInstance* Instance, const char* Identifier, TArray<FTouchEngineDynamicVariableStruct>& VariableList);

	UPROPERTY()
	FString									MyToxPath;
	TouchObject<TEInstance>					MyTEInstance = nullptr;
	TouchObject<TED3D11Context>				MyTEContext = nullptr;

	ID3D11Device*							MyDevice = nullptr;
	ID3D11DeviceContext*					MyImmediateContext = nullptr;

	TMap<FString, FTouchCHOPSingleSample>	MyCHOPSingleOutputs;
	TMap<FString, FTouchCHOPFull>			MyCHOPFullOutputs;
	FCriticalSection						MyTOPLock;
	TMap<FString, FTouchTOP>				MyTOPOutputs;

	FMessageLog								MyMessageLog = FMessageLog(TEXT("TouchEngine"));
	bool									MyLogOpened = false;
	FCriticalSection						MyMessageLock;
	TArray<FString>							MyErrors;
	TArray<FString>							MyWarnings;

	std::deque<TexCleanup>					MyTexCleanups;
	std::atomic<bool>						MyDidLoad = false;
	bool									MyFailedLoad = false;
	bool									MyCooking = false;
	TETimeMode								MyTimeMode = TETimeMode::TETimeInternal;
	float									MyFrameRate = 60.f;
	int64									MyTime = 0;

	RHIType									MyRHIType = RHIType::Invalid;

};