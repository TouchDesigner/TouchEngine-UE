// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TouchEngineInfo.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "TouchEngineComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class TOUCHENGINE_API UTouchEngineComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UTouchEngineComponent();

	// Our Touch Engine Info
	UPROPERTY()
	ATouchEngineInfo* EngineInfo;
	// Path to the Tox File to load
	UPROPERTY(EditDefaultsOnly)
	FString ToxPath;
	// Render target to render the tox file on
	UPROPERTY(EditDefaultsOnly)
	UTextureRenderTarget2D* RenderTarget;
	// rendering material
	UPROPERTY(EditDefaultsOnly)
	UMaterialInterface* RenderMaterial;

	UPROPERTY(BlueprintReadWrite)
	UTexture2D* outTex;

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

		
};
