// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TouchEngineInfo.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "TouchEngineDynamicVariableStruct.h"
#include "TouchEngineComponent.generated.h"



UCLASS(DefaultToInstanced, Blueprintable, abstract)
class TOUCHENGINE_API UTouchEngineComponentBase : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UTouchEngineComponentBase();

	// Our Touch Engine Info
	UPROPERTY()
	UTouchEngineInfo* EngineInfo;
	// Path to the Tox File to load
	UPROPERTY(EditDefaultsOnly)
	FString ToxPath;
	// Render target to render the tox file on
	UPROPERTY(BlueprintReadWrite)
	UTextureRenderTarget2D* RenderTarget;
	// Rendering material
	UPROPERTY(EditDefaultsOnly)
	UMaterialInterface* RenderMaterial;
	// The texture that will be rendered to
	UPROPERTY(BlueprintReadWrite)
	UTexture2D* outTex;


	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FTouchEngineDynamicVariableStruct testStruct;

	//UPROPERTY(BlueprintAssignable)
	//FTouchOnLoaded OnPostLoad;


protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	virtual void PostLoad() override;

	virtual void PostEditChangeProperty(FPropertyChangedEvent& e);

	void LoadTox();

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

		
};
