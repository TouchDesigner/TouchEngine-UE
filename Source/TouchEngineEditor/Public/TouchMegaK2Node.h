// Copyright © Derivative Inc. 2021

#pragma once

#include "CoreMinimal.h"
#include "TouchK2NodeBase.h"
#include "K2Node_TouchAddPinInterface.h"

#include "TouchMegaK2Node.generated.h"

/**
 *
 */

USTRUCT()
struct FPinPairs
{
	GENERATED_BODY()

		FPinPairs() = default;
	FPinPairs(UEdGraphPin* In_NamePin, UEdGraphPin* In_ValuePin) { NamePin = In_NamePin; ValuePin = In_ValuePin; }

	UEdGraphPin* NamePin;
	UEdGraphPin* ValuePin;
};

UCLASS()
class TOUCHENGINEEDITOR_API UTouchMegaK2Node : public UTouchK2NodeBase, public IK2Node_TouchAddPinInterface
{
	GENERATED_BODY()
public:

	// Removes an input pin
	void RemoveInputPin(UEdGraphPin* Pin);


	//UEdGraphNode implementation
	//Create our pins
	virtual void AllocateDefaultPins() override;
	//Implement our own node title
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	//Implement our own node tooltip text
	virtual FText GetTooltipText() const override;
	//UEdGraphNode implementation

	//K2Node implementation
	virtual void GetNodeContextMenuActions(class UToolMenu* Menu, class UGraphNodeContextMenuContext* Context) const override;
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

	// IK2Node_TouchAddPinInterface interface
	virtual void AddInputPin() override;
	virtual void AddOutputPin() override;
	// End of IK2Node_TouchAddPinInterface interface

protected:

	// Returns whether or not the pin type is valid for a TouchEngine Output
	bool CheckPinCategoryWithDirection(UEdGraphPin* Pin, bool IsInput);
	// Makes a new FName for an additional pin
	FName NewPinName(FName PinName, int Num);
	// Ensures all pins have the correct names after a removal
	void SyncPinNames();
	// Gets the name a pin should be using
	FName GetPinName(const int32 PinIndex);

	bool IsInputValuePin(UEdGraphPin* Pin);

	bool IsInputNamePin(UEdGraphPin* Pin);

	bool IsOutputValuePin(UEdGraphPin* Pin);

	bool IsOutputNamePin(UEdGraphPin* Pin);
	// The number of inputs the node currently has
	UPROPERTY()
	int NumInputs = 1;
	UPROPERTY()
	int NumOutputs = 0;
	// Pin Pairs to help remove both pins when a pin is removed
	TArray<FPinPairs> PinPairs;
};
