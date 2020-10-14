// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "TouchEngine.h"
#include <atomic>
#include <mutex>
#include <deque>
#include "Windows/AllowWindowsPlatformTypes.h"
#include <d3d11.h>
#include <d3d11on12.h>
#include "Windows/HideWindowsPlatformTypes.h"
#include "Engine/Texture2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "UTouchEngine.generated.h"

USTRUCT(BlueprintType, DisplayName = "TouchEngine CHOP", Category = "TouchEngine Structs")
struct FTouchCHOPSingleSample
{
	GENERATED_BODY()

	UPROPERTY(BlueprintType, EditAnywhere, BlueprintReadWrite, Category = "TouchEngine Struct")
	TArray<float>	channelData;
};

USTRUCT(BlueprintType, DisplayName = "TouchEngine TOP", Category = "TouchEngine Structs")
struct FTouchTOP
{
	GENERATED_BODY()

	UPROPERTY(BlueprintType, EditAnywhere, BlueprintReadWrite, Category = "TouchEngine Struct")
	UTexture2D*		texture = nullptr;

	ID3D11Resource* wrappedResource = nullptr;
};

UCLASS(BlueprintType)
class TOUCHENGINE_API UTouchEngine : public UObject
{
	GENERATED_BODY()

	virtual void	BeginDestroy() override;

	void clear();

public:
	void			loadTox(FString toxPath);
	const FString&	getToxPath() const;

	void			cookFrame();

	FTouchCHOPSingleSample		getCHOPOutputSingleSample(const FString& identifier);
	void						setCHOPInputSingleSample(const FString &identifier, const FTouchCHOPSingleSample &chop);

	FTouchTOP					getTOPOutput(const FString& identifier);
	void						setTOPInput(const FString& identifier, UTexture *texture);

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
private:

	class TexCleanup
	{
	public:
		ID3D11Query*	query = nullptr;
		TED3DTexture*	texture = nullptr;
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

	static void		eventCallback(TEInstance* instance,
									TEEvent event,
									TEResult result,
									int64_t start_time_value,
									int32_t start_time_scale,
									int64_t end_time_value,
									int32_t end_time_scale,
									void* info);

	void			addResult(const FString& s, TEResult result);
	void			addError(const FString& s);
	void			addWarning(const FString& s);

	void			outputMessages();

	void			outputResult(const FString& s, TEResult result);
	void			outputError(const FString& s);
	void			outputWarning(const FString& s);

	static void		cleanupTextures(ID3D11DeviceContext* context, std::deque<TexCleanup> *cleanups, FinalClean fa);
	static void		parameterValueCallback(TEInstance * instance, const char *identifier, void * info);
	void			parameterValueCallback(TEInstance * instance, const char *identifier);

	FString			myToxPath;
	TEInstance*		myTEInstance = nullptr;
	TEGraphicsContext*	myTEContext = nullptr;

	ID3D11Device*	myDevice = nullptr;
	ID3D11DeviceContext*	myImmediateContext = nullptr;
	ID3D11On12Device*		myD3D11On12 = nullptr;

	TMap<FString, FTouchCHOPSingleSample>	myCHOPOutputs;
	FCriticalSection			myTOPLock;
	TMap<FString, FTouchTOP>	myTOPOutputs;

	FMessageLog					myMessageLog = FMessageLog(TEXT("TouchEngine"));
	bool						myLogOpened = false;
	FCriticalSection			myMessageLock;
	TArray<FString>				myErrors;
	TArray<FString>				myWarnings;

	std::deque<TexCleanup>		myTexCleanups;
	std::atomic<bool>			myDidLoad = false;

	RHIType						myRHIType = RHIType::Invalid;

};
