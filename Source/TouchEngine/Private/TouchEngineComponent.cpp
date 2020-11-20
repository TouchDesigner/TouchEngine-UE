// Fill out your copyright notice in the Description page of Project Settings.


#include "TouchEngineComponent.h"
#include "Engine/Canvas.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"

// Sets default values for this component's properties
UTouchEngineComponent::UTouchEngineComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void UTouchEngineComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	EngineInfo = NewObject< ATouchEngineInfo>();
	EngineInfo->load(ToxPath);

	RenderTarget = UKismetRenderingLibrary::CreateRenderTarget2D(this);
}


// Called every frame
void UTouchEngineComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...

	if (!EngineInfo->isLoaded())
	{
		return;
	}

	// render noise texture

	// output
	UCanvas* canvas; FVector2D size; FDrawToRenderTargetContext context;
	UKismetRenderingLibrary::BeginDrawCanvasToRenderTarget(this, RenderTarget, canvas, size, context);

	canvas->K2_DrawMaterial(RenderMaterial, FVector2D(0.f, 0.f), size, FVector2D(0.f, 0.f), FVector2D(1.f, 1.f), 0.f, FVector2D(.5f, .5f));
	UKismetRenderingLibrary::EndDrawCanvasToRenderTarget(this, context);

	// set touchengine inputs
	FTouchCHOPSingleSample tcss; tcss.channelData.Add( UKismetMathLibrary::DegSin(UGameplayStatics::GetTimeSeconds(this)));
	EngineInfo->setCHOPInputSingleSample(FString("RGBBrightness"), tcss);

	EngineInfo->setTOPInput(FString("ImageIn"), RenderTarget);

	EngineInfo->cookFrame();

	outTex = EngineInfo->getTOPOutput(FString("ImageOut")).texture;
}

