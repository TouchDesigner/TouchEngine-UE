// Fill out your copyright notice in the Description page of Project Settings.


#include "TouchEngineActor.h"

// Sets default values
ATouchEngineActor::ATouchEngineActor()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	touchEngineComponent = CreateDefaultSubobject<UTouchEngineComponentBase>("TouchEngine Component");
	//touchEngineComponent->RegisterComponent();
}

// Called when the game starts or when spawned
void ATouchEngineActor::BeginPlay()
{
	Super::BeginPlay();

}

// Called every frame
void ATouchEngineActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

