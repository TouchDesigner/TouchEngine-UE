// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "K2Node.h"
#include "TouchOutputK2Node.generated.h"

/**
 * 
 */
UCLASS()
class TOUCHENGINEEDITOR_API UTouchOutputK2Node : public UK2Node
{
	GENERATED_BODY()

public:

	//UEdGraphNode implementation
	//Create our pins
	virtual void AllocateDefaultPins() override;
	//Implement our own node title
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	//Implement our own node tooltip text
	virtual FText GetTooltipText() const override;
	//UEdGraphNode implementation

	//K2Node implementation
	// Implement our own node category
	virtual FText GetMenuCategory() const override;
	//This method works like a bridge and connects our K2Node to the actual Blueprint Library method. This is where the actual logic happens.
	virtual void ExpandNode(class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph) override;

	virtual void GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const override;

	virtual void ReallocatePinsDuringReconstruction(TArray<UEdGraphPin*>& OldPins);

	virtual void NotifyPinConnectionListChanged(UEdGraphPin* Pin) override;
	//K2Node implementation

	bool CheckPinCategory(UEdGraphPin* Pin);
};
