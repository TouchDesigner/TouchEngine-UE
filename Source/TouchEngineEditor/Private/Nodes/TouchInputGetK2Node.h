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

#include "TouchK2NodeBase.h"
#include "TouchInputGetK2Node.generated.h"

/**
 *
 */
UCLASS()
class TOUCHENGINEEDITOR_API UTouchInputGetK2Node : public UTouchK2NodeBase
{
	GENERATED_BODY()

public:
	//~ Begin UEdGraphNode implementation
	//Create our pins
	virtual void AllocateDefaultPins() override;
	//Implement our own node title
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	//~ End UEdGraphNode implementation

	//~ Begin K2Node implementation
	//This method works like a bridge and connects our K2Node to the actual Blueprint Library method. This is where the actual logic happens.
	virtual void ExpandNode(FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph) override;
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
	virtual void ReallocatePinsDuringReconstruction(TArray<UEdGraphPin*>& OldPins) override;
	/** Called when the connection list of one of the pins of this node is changed in the editor, after the pin has had it's literal cleared */
	virtual void NotifyPinConnectionListChanged(UEdGraphPin* Pin) override;
	//~ End K2Node implementation
};
