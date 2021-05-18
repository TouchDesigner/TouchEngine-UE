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

#include "TouchOutputK2Node.h"

#include "TouchBlueprintFunctionLibrary.h"

#include "KismetCompiler.h"
#include "BlueprintActionDatabaseRegistrar.h"
#include "BlueprintNodeSpawner.h"
#include "K2Node_CallFunction.h"
#include "TouchEngineComponent.h"

#define LOCTEXT_NAMESPACE "UTouchOutputK2Node"

struct FGetPinName
{
	static const FName& GetPinNameVarName()
	{
		static const FName TextPinName(TEXT("OutputName"));
		return TextPinName;
	}
	static const FName& GetPinNameComponent()
	{
		static const FName TextPinName(TEXT("TouchEngineComponent"));
		return TextPinName;
	}
	static const FName& GetPinNameValue()
	{
		static const FName TextPinName(TEXT("Value"));
		return TextPinName;
	}

	static const FName& GetPinNameOutput()
	{
		static const FName OutputPinName(TEXT("Result"));
		return OutputPinName;
	}
};



FText UTouchOutputK2Node::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return LOCTEXT("TouchGetOutput_K2Node", "Get TouchEngine Output");
}

void UTouchOutputK2Node::AllocateDefaultPins()
{
	Super::AllocateDefaultPins();
	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();

	/*Create our pins*/

	// Execution pins
	CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Exec, UEdGraphSchema_K2::PN_Execute);
	CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Exec, UEdGraphSchema_K2::PN_Then);

	// Variable pins

	//Output
	UEdGraphPin* InTextPin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_String, FGetPinName::GetPinNameVarName());
	K2Schema->SetPinAutogeneratedDefaultValue(InTextPin, FString(""));
	UEdGraphPin* InObjectPin = CreatePin(EGPD_Input, FName(TEXT("Object")), UTouchEngineComponentBase::StaticClass(), FGetPinName::GetPinNameComponent());
	UEdGraphPin* InValuePin = CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Wildcard, FGetPinName::GetPinNameValue());

	//Output
	UEdGraphPin* OutValidPin = CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Boolean, FGetPinName::GetPinNameOutput());
	K2Schema->SetPinAutogeneratedDefaultValueBasedOnType(OutValidPin);
}

FText UTouchOutputK2Node::GetTooltipText() const
{
	return LOCTEXT("TouchGetOutput_K2Node", "Get TouchEngine Output");
}

FText UTouchOutputK2Node::GetMenuCategory() const
{
	return LOCTEXT("TouchEnigne_MenuCategory", "TouchEngine");
}

void UTouchOutputK2Node::ExpandNode(FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph)
{
	Super::ExpandNode(CompilerContext, SourceGraph);

	if (!FindPin(FGetPinName::GetPinNameComponent())->HasAnyConnections())
	{
		CompilerContext.MessageLog.Error(*LOCTEXT("NoTargetComponent", "No Target Component connected").ToString(), this);
	}

	// Check input pin type to make sure it's a supported type for touchengine
	if (!CheckPinCategory(FindPin(FGetPinName::GetPinNameValue())))
	{
		// pin type is not valid
		FindPin(FGetPinName::GetPinNameValue())->BreakAllPinLinks();
		return;
	}

	//This is just a hard reference to the static method that lives in the BlueprintLibrary. Probably not the best of ways.
	UEdGraphPin* valuePin = FindPin(FGetPinName::GetPinNameValue());

	UFunction* BlueprintFunction = UTouchBlueprintFunctionLibrary::FindGetterByType(
		valuePin->PinType.PinCategory,
		valuePin->PinType.ContainerType == EPinContainerType::Array,
		valuePin->PinType.PinSubCategoryObject.IsValid() ? valuePin->PinType.PinSubCategoryObject->GetFName() : FName("")
	);

	if (BlueprintFunction == NULL) {
		CompilerContext.MessageLog.Error(*LOCTEXT("InvalidFunctionName", "The function has not been found.").ToString(), this);
		return;
	}

	UK2Node_CallFunction* CallFunction = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);

	CallFunction->SetFromFunction(BlueprintFunction);
	CallFunction->AllocateDefaultPins();
	CompilerContext.MessageLog.NotifyIntermediateObjectCreation(CallFunction, this);

	//Input
	CompilerContext.MovePinLinksToIntermediate(*FindPin(FGetPinName::GetPinNameVarName()), *CallFunction->FindPin(TEXT("VarName")));
	CompilerContext.MovePinLinksToIntermediate(*FindPin(FGetPinName::GetPinNameComponent()), *CallFunction->FindPin(TEXT("Target")));

	//Output
	CompilerContext.MovePinLinksToIntermediate(*FindPin(FGetPinName::GetPinNameValue()), *CallFunction->FindPin(TEXT("value")));
	CompilerContext.MovePinLinksToIntermediate(*FindPin(FGetPinName::GetPinNameOutput()), *CallFunction->GetReturnValuePin());

	//Exec pins
	UEdGraphPin* NodeExec = GetExecPin();
	UEdGraphPin* NodeThen = FindPin(UEdGraphSchema_K2::PN_Then);

	UEdGraphPin* InternalExec = CallFunction->GetExecPin();
	CompilerContext.MovePinLinksToIntermediate(*NodeExec, *InternalExec);

	UEdGraphPin* InternalThen = CallFunction->GetThenPin();
	CompilerContext.MovePinLinksToIntermediate(*NodeThen, *InternalThen);

	//After we are done we break all links to this node (not the internally created one)
	BreakAllNodeLinks();
}

void UTouchOutputK2Node::GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
{
	Super::GetMenuActions(ActionRegistrar);

	UClass* Action = GetClass();

	if (ActionRegistrar.IsOpenForRegistration(Action)) {
		UBlueprintNodeSpawner* Spawner = UBlueprintNodeSpawner::Create(GetClass());
		check(Spawner != nullptr);

		ActionRegistrar.AddBlueprintAction(Action, Spawner);
	}
}

void UTouchOutputK2Node::ReallocatePinsDuringReconstruction(TArray<UEdGraphPin*>& OldPins)
{
	Super::ReallocatePinsDuringReconstruction(OldPins);

	// This is necessary to retain type information after pasting or loading from disc
	if (UEdGraphPin* OutputPin = FindPin(FGetPinName::GetPinNameValue()))
	{
		// Only update the output pin if it is currently a wildcard
		if (OutputPin->PinType.PinCategory == UEdGraphSchema_K2::PC_Wildcard)
		{
			// Find the matching Old Pin if it exists
			for (UEdGraphPin* OldPin : OldPins)
			{
				if (OldPin->PinName == OutputPin->PinName)
				{
					// Update our output pin with the old type information
					OutputPin->PinType = OldPin->PinType;

					auto Schema = GetSchema();
					Schema->RecombinePin(OutputPin);

					break;
				}
			}
		}
	}
}

void UTouchOutputK2Node::NotifyPinConnectionListChanged(UEdGraphPin* Pin)
{
	Super::NotifyPinConnectionListChanged(Pin);

	if (Pin->PinName == FGetPinName::GetPinNameValue())
	{
		if (Pin->HasAnyConnections())
		{
			if (!CheckPinCategory(Pin->LinkedTo[0]))
			{
				Pin->BreakAllPinLinks();
				return;
			}

			Pin->PinType = Pin->LinkedTo[0]->PinType;
		}
		else
		{
			Pin->PinType.PinCategory = UEdGraphSchema_K2::PC_Wildcard;
			Pin->PinType.ContainerType = EPinContainerType::None;
		}
	}
}


bool UTouchOutputK2Node::CheckPinCategory(UEdGraphPin* Pin)
{
	FName PinCategory = Pin->PinType.PinCategory;

	if (PinCategory == UEdGraphSchema_K2::PC_Float)
	{
		return true;
	}
	else if (PinCategory == UEdGraphSchema_K2::PC_String)
	{
		return true;
	}
	else if (PinCategory == UEdGraphSchema_K2::PC_Object)
	{
		UClass* objectClass = Cast<UClass>(Pin->PinType.PinSubCategoryObject.Get());

		if (objectClass == UTexture2D::StaticClass() || objectClass->IsChildOf<UTexture2D>() || UTexture2D::StaticClass()->IsChildOf(objectClass))
		{
			return true;
		}
		else if (objectClass == UTouchEngineCHOP::StaticClass() || objectClass->IsChildOf<UTouchEngineCHOP>() || UTouchEngineCHOP::StaticClass()->IsChildOf(objectClass))
		{
			return true;
		}
		 
		return false;
	}

	return false;
}


#undef LOCTEXT_NAMESPACE