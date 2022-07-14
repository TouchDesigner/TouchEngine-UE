/* Shared Use License: This file is owned by Derivative Inc. (Derivative)
* and can only be used, and/or modified for use, in conjunction with
* Derivative's TouchDesigner software, and only if you are a licensee who has
* accepted Derivative's TouchDesigner license or assignment agreement
* (which also govern the use of this file). You may share or redistribute
* a modified version of this file provided the following conditions are met:
*
* 1. The shared file or redistribution must retain the information set out
* above and this list of conditions.
* 2. Derivative's name (Derivative Inc.) or its trademarks may not be used
* to endorse or promote products derived from this file without specific
* prior written permission from Derivative.
*/

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TouchEngineActor.generated.h"

class UTouchEngineComponentBase;

UCLASS(Abstract, Blueprintable)
class TOUCHENGINE_API ATouchEngineActor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ATouchEngineActor();

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Components, meta = (AllowPrivateAccess = "true", DisplayName = "TouchEngine Component"))
	TObjectPtr<UTouchEngineComponentBase> TouchEngineComponent;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
