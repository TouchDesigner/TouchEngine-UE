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
#include "Windows/HideWindowsPlatformTypes.h"
#include "Engine/Texture2D.h"
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

	int				w = 0;
	int				h = 0;
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

	static void		cleanupTextures(ID3D11DeviceContext* context, std::deque<TexCleanup> *cleanups, FinalClean fa);
	static void		parameterValueCallback(TEInstance * instance, const char *identifier, void * info);
	void			parameterValueCallback(TEInstance * instance, const char *identifier);

	FString			myToxPath;
	TEInstance*		myInstance = nullptr;
	TEGraphicsContext*	myContext = nullptr;
	ID3D11Device*	myDevice = nullptr;
	ID3D11DeviceContext*	myImmediateContext = nullptr;

	TMap<FString, FTouchCHOPSingleSample>	myCHOPOutputs;
	std::mutex					myTOPLock;
	TMap<FString, FTouchTOP>	myTOPOutputs;


	std::deque<TexCleanup>		myTexCleanups;
	std::atomic<bool>			myDidLoad = false;

};
