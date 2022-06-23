// Copyright © Derivative Inc. 2021

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "K2Node_TouchAddPinInterface.generated.h"

UINTERFACE(meta = (CannotImplementInterfaceInBlueprint))
class TOUCHENGINEEDITOR_API UK2Node_TouchAddPinInterface : public UInterface
{
	GENERATED_BODY()
};

class TOUCHENGINEEDITOR_API IK2Node_TouchAddPinInterface
{
	GENERATED_BODY()

public:
	virtual void AddInputPin() { }
	virtual void AddOutputPin() { }
	virtual bool CanAddPin() const { return true; }
};
