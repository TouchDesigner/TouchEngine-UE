// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TouchEngineComponent.h"
#include "TouchEngineActor.generated.h"

UCLASS(Abstract, Blueprintable)
class TOUCHENGINE_API ATouchEngineActor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ATouchEngineActor();

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Components, meta = (AllowPrivateAccess = "true", DisplayName = "TouchEngine Component"))
	UTouchEngineComponentBase* TouchEngineComponent;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
