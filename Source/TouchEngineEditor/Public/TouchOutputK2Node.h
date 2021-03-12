// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "K2Node.h"
#include "TouchOutputK2Node.generated.h"

/**
 * Blueprint node that gets the value of a TouchEngine Output variable from the passed in TouchEngineComponent at runtime
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
	/**
	 * Replacement for GetMenuEntries(). Override to add specific
	 * UBlueprintNodeSpawners pertaining to the sub-class type. Serves as an
	 * extensible way for new nodes, and game module nodes to add themselves to
	 * context menus.
	 *
	 * @param  ActionRegistrar	The list to be populated with new spawners.
	 */
	virtual void GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const override;
	/**
	 * Reallocate pins during reconstruction; by default ignores the old pins and calls AllocateDefaultPins()
	 * If you override this to create additional pins you likely need to call RestoreSplitPins to restore any
	 * pins that have been split (e.g. a vector pin split into its components)
	 */
	virtual void ReallocatePinsDuringReconstruction(TArray<UEdGraphPin*>& OldPins);
	/** Called when the connection list of one of the pins of this node is changed in the editor, after the pin has had it's literal cleared */
	virtual void NotifyPinConnectionListChanged(UEdGraphPin* Pin) override;
	//K2Node implementation

	// Returns whether or not the pin type is valid for a TouchEngine Output
	bool CheckPinCategory(UEdGraphPin* Pin);
};
