// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "TouchEngine.h"
#include <deque>
#include "Windows/AllowWindowsPlatformTypes.h"
#include <d3d11.h>
#include <d3d11on12.h>
#include "Windows/HideWindowsPlatformTypes.h"
#include "Engine/Texture2D.h"
#include "UTouchEngine.generated.h"

class UTouchEngineInfo;
struct FTouchDynamicVariable;

template <typename T>
struct FTouchVar
{
	T data;
};

USTRUCT(BlueprintType, DisplayName = "TouchEngine CHOP", Category = "TouchEngine Structs")
struct FTouchCHOPSingleSample
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TouchEngine Struct")
	TArray<float>	channelData;
};

USTRUCT(BlueprintType, DisplayName = "TouchEngine TOP", Category = "TouchEngine Structs")
struct FTouchTOP
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TouchEngine Struct")
	UTexture2D*		texture = nullptr;

	ID3D11Resource* wrappedResource = nullptr;
};


DECLARE_MULTICAST_DELEGATE(FTouchOnLoadComplete);
DECLARE_MULTICAST_DELEGATE(FTouchOnLoadFailed);
DECLARE_MULTICAST_DELEGATE_TwoParams(FTouchOnParametersLoaded, TArray<FTouchDynamicVariable>, TArray<FTouchDynamicVariable>);
DECLARE_MULTICAST_DELEGATE(FTouchOnCookFinished);

UCLASS()
class TOUCHENGINE_API UTouchEngine : public UObject
{
	friend class UTouchEngineInfo;

	GENERATED_BODY()

	virtual void	BeginDestroy() override;

	void clear();

public:

	void Copy(UTouchEngine* other);

	void			loadTox(FString toxPath);
	const FString&	getToxPath() const;

	void			cookFrame(int64 FrameTime_Mill);

	bool			setCookMode(bool IsIndependent);

	bool			setFrameRate(int64 frameRate);

	FTouchCHOPSingleSample		getCHOPOutputSingleSample(const FString& identifier);
	void						setCHOPInputSingleSample(const FString &identifier, const FTouchCHOPSingleSample &chop);

	FTouchTOP					getTOPOutput(const FString& identifier);
	void						setTOPInput(const FString& identifier, UTexture *texture);

	FTouchVar<bool>				getBooleanOutput(const FString& identifier);
	void						setBooleanInput(const FString& identifier, FTouchVar<bool>& op);
	FTouchVar<double>			getDoubleOutput(const FString& identifier);
	void						setDoubleInput(const FString& identifier, FTouchVar<TArray<double>>& op);	
	FTouchVar<int32_t>			getIntegerOutput(const FString& identifier);
	void						setIntegerInput(const FString& identifier, FTouchVar<int32_t>& op);
	FTouchVar<TEString*>		getStringOutput(const FString& identifier);
	void						setStringInput(const FString& identifier, FTouchVar<char*>& op);
	FTouchVar<TETable*>			getTableOutput(const FString& identifier);
	void						setTableInput(const FString& identifier, FTouchVar<TETable*>& op);

	void
	setDidLoad()
	{
		myDidLoad = true;
	}

	bool
	getDidLoad()
	{
		return myDidLoad;
	}

	bool getFailedLoad() { return myFailedLoad; }

	FTouchOnLoadComplete OnLoadComplete;
	FTouchOnLoadFailed OnLoadFailed;
	FTouchOnParametersLoaded OnParametersLoaded;
	FTouchOnCookFinished OnCookFinished;

private:

	class TexCleanup
	{
	public:
		ID3D11Query*	query = nullptr;
		TED3D11Texture*	texture = nullptr;
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





	static void		eventCallback(TEInstance* instance, TEEvent event, TEResult result, int64_t start_time_value, int32_t start_time_scale, int64_t end_time_value, int32_t end_time_scale, void* info);

	void			addResult(const FString& s, TEResult result);
	void			addError(const FString& s);
	void			addWarning(const FString& s);

	void			outputMessages();

	void			outputResult(const FString& s, TEResult result);
	void			outputError(const FString& s);
	void			outputWarning(const FString& s);

	static void		cleanupTextures(ID3D11DeviceContext* context, std::deque<TexCleanup> *cleanups, FinalClean fa);
	static void		parameterValueCallback(TEInstance* instance, TEParameterEvent event, const char* identifier, void* info);
	void			parameterValueCallback(TEInstance * instance, TEParameterEvent event, const char *identifier);

	TEResult		parseGroup(TEInstance* instance, const char* identifier, TArray<FTouchDynamicVariable>& variables);
	TEResult		parseInfo(TEInstance* instance, const char* identifier, TArray<FTouchDynamicVariable>& variableList);

	UPROPERTY()
	FString									myToxPath;
	TEInstance*								myTEInstance = nullptr;
	TEGraphicsContext*						myTEContext = nullptr;

	ID3D11Device*							myDevice = nullptr;
	ID3D11DeviceContext*					myImmediateContext = nullptr;
	ID3D11On12Device*						myD3D11On12 = nullptr;

	TMap<FString, FTouchCHOPSingleSample>	myCHOPOutputs;
	FCriticalSection						myTOPLock;
	TMap<FString, FTouchTOP>				myTOPOutputs;

	FMessageLog								myMessageLog = FMessageLog(TEXT("TouchEngine"));
	bool									myLogOpened = false;
	FCriticalSection						myMessageLock;
	TArray<FString>							myErrors;
	TArray<FString>							myWarnings;

	std::deque<TexCleanup>					myTexCleanups;
	std::atomic<bool>						myDidLoad = false;
	bool									myFailedLoad = false;
	bool									myCooking = false;
	TETimeMode								myTimeMode = TETimeMode::TETimeInternal;
	float									myFrameRate = 60.f;
	int64									myTime = 0;

	RHIType									myRHIType = RHIType::Invalid;

};