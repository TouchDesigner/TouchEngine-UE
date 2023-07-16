// Copyright © Derivative Inc. 2022

#pragma once

// #include "CoreMinimal.h"
// #include "Engine/Texture2D.h"
// #include "Containers/Ticker.h"
// #include "TemporaryTexture2D.generated.h"

// /**
//  * 
//  */
// UCLASS(BlueprintType)
// class TOUCHENGINE_API UTemporaryTexture2D : public UTexture2D
// {
// 	GENERATED_BODY()
//
// public:
// 	/** creates and initializes a new Texture2D with the requested settings */
// 	static UTemporaryTexture2D* CreateTransient(int32 InSizeX, int32 InSizeY, EPixelFormat InFormat = PF_B8G8R8A8, const FName InName = NAME_None);
// 	
// 	/** If set to true, the texture and its resources will be deleted next frame. */
// 	UFUNCTION(BlueprintCallable, Category="Rendering|Texture")
// 	void SetIsTemporary(bool InIsTemporary);
// 	/** Check if the texture is Temporary. If true, the texture and its resources will be deleted next frame. */
// 	UFUNCTION(BlueprintCallable, Category="Rendering|Texture")
// 	bool GetIsTemporary() const;
// 	
// protected:
// 	bool TickToDeath(float DeltaTime);
// 	
// private:
// 	/** True by default. If set to true, the texture will be deleted next frame */
// 	UPROPERTY(Transient)
// 	bool IsTemporary = true;
//
// 	FTickerDelegate TickDelegate;
// 	FTSTicker::FDelegateHandle TickDelegateHandle;
// };
