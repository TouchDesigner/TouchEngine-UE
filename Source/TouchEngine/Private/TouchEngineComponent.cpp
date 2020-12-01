// Fill out your copyright notice in the Description page of Project Settings.


#include "TouchEngineComponent.h"
#include "Engine/Canvas.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Editor.h"

// Sets default values for this component's properties
UTouchEngineComponentBase::UTouchEngineComponentBase()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void UTouchEngineComponentBase::BeginPlay()
{
	Super::BeginPlay();

	// ...

	RenderTarget = UKismetRenderingLibrary::CreateRenderTarget2D(this);

	//GEditor->OnBlueprintCompiled().AddUFunction(this, &UTouchEngineComponentBase::OnCompile);
}


void UTouchEngineComponentBase::PostLoad()
{
	Super::PostLoad();
	LoadTox();

	GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, FString::Printf(TEXT("Post Load")));
}

void UTouchEngineComponentBase::PostEditChangeProperty(FPropertyChangedEvent& e)
{
	FName PropertyName = (e.Property != NULL) ? e.Property->GetFName() : NAME_None;
	if (PropertyName == GET_MEMBER_NAME_CHECKED(UTouchEngineComponentBase, ToxPath))
	{
		LoadTox();
		GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Orange, FString::Printf(TEXT("Post Edit Tox Path")));
	}
	Super::PostEditChangeProperty(e);
}

void UTouchEngineComponentBase::LoadTox()
{
	EngineInfo = NewObject< UTouchEngineInfo>();
	EngineInfo->load(ToxPath);

	testStruct.parent = this;
}

// Called every frame
void UTouchEngineComponentBase::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...

	if (!EngineInfo || !EngineInfo->isLoaded())
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

