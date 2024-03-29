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

#include "TouchInputK2Node.h"

#include "Blueprint/TouchBlueprintFunctionLibrary.h"

#include "BlueprintActionDatabaseRegistrar.h"
#include "BlueprintNodeSpawner.h"
#include "K2Node_CallFunction.h"
#include "KismetCompiler.h"
#include "Engine/TouchVariables.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Settings/EditorStyleSettings.h"

#define LOCTEXT_NAMESPACE "TouchInputK2Node"

FText UTouchInputK2Node::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return LOCTEXT("SetTEInput" ,"Set TouchEngine Input");
}

void UTouchInputK2Node::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	const FName PropertyName = (PropertyChangedEvent.Property  ? PropertyChangedEvent.Property->GetFName() : NAME_None);
	if (PropertyName == GET_MEMBER_NAME_CHECKED(UTouchInputK2Node, AdditionalPins))
	{
		ReconstructNode();
		GetGraph()->NotifyGraphChanged();
	}
	Super::PostEditChangeProperty(PropertyChangedEvent);
}

void UTouchInputK2Node::AllocateDefaultPins()
{
	Super::AllocateDefaultPins();
	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();

	/*Create our pins*/

	// Execution pins
	CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Exec, UEdGraphSchema_K2::PN_Execute);
	CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Exec, UEdGraphSchema_K2::PN_Then);

	// Variable pins

	//Input
	UEdGraphPin* InObjectPin = CreateTouchComponentPin(LOCTEXT("Inputs_ObjectTooltip", "TouchEngine Component to set input values on"));
	UEdGraphPin* InTextPin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_String, FPinNames::InputName);
	K2Schema->SetPinAutogeneratedDefaultValue(InTextPin, FString(""));
	UEdGraphPin* InValuePin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Wildcard, FPinNames::Value);

	//Output
	UEdGraphPin* OutValidPin = CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Boolean, FPinNames::Result);
	K2Schema->SetPinAutogeneratedDefaultValueBasedOnType(OutValidPin);

	// Additional Pin Names for functions taking additional parameters, like setting a texture
	for (const auto& Pin : AdditionalPins)
	{
		CreatePin(EGPD_Input, Pin.Value, Pin.Key);
	}
}

FText UTouchInputK2Node::GetPinNameOverride(const UEdGraphPin& Pin) const
{
	if (Pin.PinName == FPinNames::TouchEngineComponent)
	{
		return Pin.PinFriendlyName;
	}

	if (Pin.PinType.PinCategory == UEdGraphSchema_K2::PC_Exec)
	{
		return FText::GetEmpty();
	}

	FText DisplayName = [&Pin]
	{
		if (Pin.bAllowFriendlyName && !Pin.PinFriendlyName.IsEmpty())
		{
			return Pin.PinFriendlyName;
		}
		if (!Pin.PinName.IsNone())
		{
			return FText::FromName(Pin.PinName);
		}
		return FText::GetEmpty();
	}();

	if (GetDefault<UEditorStyleSettings>()->bShowFriendlyNames && Pin.bAllowFriendlyName)
	{
		const bool bIsBool = Pin.PinType.PinCategory == UEdGraphSchema_K2::PC_Boolean;
		return FText::FromString(FName::NameToDisplayString(DisplayName.ToString(), bIsBool));
	}

	return DisplayName;
}

UEdGraphPin* UTouchInputK2Node::CreatePinForProperty(const UFunction* BlueprintFunction, const FProperty* Param)
{
	// inspired by UK2Node_CallFunction::CreatePinsForFunctionCall. Will need updating if we want to handle more complex cases
	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();
	
	const bool bIsFunctionInput = !Param->HasAnyPropertyFlags(CPF_ReturnParm) && (!Param->HasAnyPropertyFlags(CPF_OutParm) || Param->HasAnyPropertyFlags(CPF_ReferenceParm));
	const bool bIsRefParam = Param->HasAnyPropertyFlags(CPF_ReferenceParm) && bIsFunctionInput;
	const EEdGraphPinDirection Direction = bIsFunctionInput ? EGPD_Input : EGPD_Output;
				
	UEdGraphNode::FCreatePinParams PinParams;
	PinParams.bIsReference = bIsRefParam;
	UEdGraphPin* ParamPin = CreatePin(Direction, NAME_None, Param->GetFName(), PinParams);
	const bool bPinGood = (ParamPin && K2Schema->ConvertPropertyToPinType(Param, /*out*/ ParamPin->PinType));
	if (ensure(bPinGood))
	{
		// Check for a display name override
		const FString& PinDisplayName = Param->GetMetaData(FBlueprintMetadata::MD_DisplayName);
		if (!PinDisplayName.IsEmpty())
		{
			ParamPin->PinFriendlyName = FText::FromString(PinDisplayName);
		}

		//Flag pin as read only for const reference property
		ParamPin->bDefaultValueIsIgnored = Param->HasAllPropertyFlags(CPF_ConstParm | CPF_ReferenceParm) && (!BlueprintFunction->HasMetaData(FBlueprintMetadata::MD_AutoCreateRefTerm) || ParamPin->PinType.IsContainer());

		const bool bAdvancedPin = Param->HasAllPropertyFlags(CPF_AdvancedDisplay);
		ParamPin->bAdvancedView = bAdvancedPin;
		if(bAdvancedPin && (ENodeAdvancedPins::NoPins == AdvancedPinDisplay))
		{
			AdvancedPinDisplay = ENodeAdvancedPins::Hidden;
		}

		FString ParamValue;
		if (K2Schema->FindFunctionParameterDefaultValue(BlueprintFunction, Param, ParamValue))
		{
			K2Schema->SetPinAutogeneratedDefaultValue(ParamPin, ParamValue);
		}
		else
		{
			K2Schema->SetPinAutogeneratedDefaultValueBasedOnType(ParamPin);
		}
		
		UK2Node_CallFunction::GeneratePinTooltipFromFunction(*ParamPin, BlueprintFunction);
	}

	return ParamPin;
}

void UTouchInputK2Node::PinConnectionListChanged(UEdGraphPin* Pin)
{
	Super::PinConnectionListChanged(Pin);
	
	UEdGraphPin* ValuePin = FindPinChecked(FPinNames::Value);

	// Clear all pins.
	if(Pin == ValuePin)
	{
		if (!IsPinCategoryValid(ValuePin))
		{
			return;
		}
		
		Modify();
		// get the proper function from the library based on pin category
		const UFunction* BlueprintFunction = UTouchBlueprintFunctionLibrary::FindSetterByType(
			GetCategoryNameChecked(ValuePin),
			ValuePin->PinType.ContainerType == EPinContainerType::Array,
			ValuePin->PinType.PinSubCategoryObject.IsValid() ? ValuePin->PinType.PinSubCategoryObject->GetFName() : FName("")
		);
		if (!ensure(BlueprintFunction)) // we will raise an error on compile anyway
		{
			return;
		}

		AdditionalPins.Empty();

		// then we create a pin fpr each additional parameter, unless we already have an existing pin matching
		TArray<UEdGraphPin*> NewPins;
		for (const FProperty* Param = CastField<FProperty>(BlueprintFunction->ChildProperties); Param; Param = CastField<FProperty>(Param->Next))
		{
			if (EnumHasAnyFlags(Param->PropertyFlags, CPF_Parm) && !EnumHasAnyFlags(Param->PropertyFlags, CPF_ReturnParm) && !FFunctionParametersNames::DefaultParameters.Contains(Param->GetFName()))
			{
				const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();
				FEdGraphPinType PinType;
				K2Schema->ConvertPropertyToPinType(Param, /*out*/ PinType);
				UEdGraphPin** FoundExistingPin = Pins.FindByPredicate([&PinType, &Param](const UEdGraphPin* Pin)
				{
					return Pin && Pin->Direction == EGPD_Input && Pin->PinType.PinCategory != UEdGraphSchema_K2::PC_Exec && !FPinNames::DefaultInputs.Contains(Pin->PinName) &&
						Pin->PinName == Param->GetFName() && Pin->PinType == PinType;
				});
				if (FoundExistingPin)
				{
					UEdGraphPin* ExistingPin = *FoundExistingPin;
					AdditionalPins.Add(ExistingPin->GetFName(), ExistingPin->PinType.PinCategory);
					NewPins.Add(ExistingPin);
				}
				else
				{
					UEdGraphPin* NewPin = CreatePinForProperty(BlueprintFunction, Param);
					AdditionalPins.Add(NewPin->GetFName(), NewPin->PinType.PinCategory);
					NewPins.Add(NewPin);
				}
			}
		}

		// finally we delete any other pin that might remain
		for (auto It = Pins.CreateConstIterator(); It; ++It) // from UK2Node_FormatText::PinConnectionListChanged
		{
			UEdGraphPin* CheckPin = *It;
			if (CheckPin->Direction == EGPD_Input && CheckPin->PinType.PinCategory != UEdGraphSchema_K2::PC_Exec && !FPinNames::DefaultInputs.Contains(CheckPin->PinName) && !NewPins.Contains(CheckPin))
			{
				CheckPin->Modify();
				CheckPin->MarkAsGarbage();
				Pins.Remove(CheckPin);
				--It;
			}
		}

		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(GetBlueprint());
	}
}

void UTouchInputK2Node::PinDefaultValueChanged(UEdGraphPin* Pin)
{
	Super::PinDefaultValueChanged(Pin);
}

void UTouchInputK2Node::PinTypeChanged(UEdGraphPin* Pin)
{
	Super::PinTypeChanged(Pin);
}

void UTouchInputK2Node::ExpandNode(FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph)
{
	Super::ExpandNode(CompilerContext, SourceGraph);


	if (!FindPin(FPinNames::TouchEngineComponent)->HasAnyConnections())
	{
		CompilerContext.MessageLog.Error(*LOCTEXT("NoTargetComponent", "No Target Component connected").ToString(), this);
	}

	// Check input pin type to make sure it's a supported type for TouchEngine

	UEdGraphPin* ValuePin = FindPinChecked(FPinNames::Value);

	if (!IsPinCategoryValid(ValuePin))
	{
		// pin type is not valid
		ValuePin->BreakAllPinLinks();
		return;
	}

	// get the proper function from the library based on pin category
	const UFunction* BlueprintFunction = UTouchBlueprintFunctionLibrary::FindSetterByType(
		GetCategoryNameChecked(ValuePin),
		ValuePin->PinType.ContainerType == EPinContainerType::Array,
		ValuePin->PinType.PinSubCategoryObject.IsValid() ? ValuePin->PinType.PinSubCategoryObject->GetFName() : FName("")
	);

	if (!BlueprintFunction) {
		CompilerContext.MessageLog.Error(*LOCTEXT("InvalidFunctionName", "The function has not been found.").ToString(), this);
		return;
	}

	UK2Node_CallFunction* CallFunction = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);

	CallFunction->SetFromFunction(BlueprintFunction);
	CallFunction->AllocateDefaultPins();
	CallFunction->FindPinChecked(FPinNames::Prefix)->DefaultValue = "i/";
	CompilerContext.MessageLog.NotifyIntermediateObjectCreation(CallFunction, this);

	ValidateLegacyVariableNames(FPinNames::InputName, CompilerContext, "i/");

	//Input
	CompilerContext.MovePinLinksToIntermediate(*FindPin(FPinNames::InputName), *CallFunction->FindPinChecked(FFunctionParametersNames::ParameterName));
	CompilerContext.MovePinLinksToIntermediate(*FindPin(FPinNames::TouchEngineComponent), *CallFunction->FindPinChecked(FFunctionParametersNames::TouchEngineComponent));
	CompilerContext.MovePinLinksToIntermediate(*ValuePin, *CallFunction->FindPinChecked(FFunctionParametersNames::Value));

	//Output
	CompilerContext.MovePinLinksToIntermediate(*FindPin(FPinNames::Result), *CallFunction->GetReturnValuePin());

	// Other nodes
	// TArray<FName> ErroredPins;
	for (UEdGraphPin* Pin : Pins)
	{
		if (Pin->Direction == EGPD_Input && Pin->PinType.PinCategory != UEdGraphSchema_K2::PC_Exec && !FPinNames::DefaultInputs.Contains(Pin->PinName))
		{
			UEdGraphPin* FunctionPin = CallFunction->FindPin(Pin->GetFName(), Pin->Direction);
			if (ensureMsgf(FunctionPin, TEXT("[UTouchInputK2Node::ExpandNode] Unable to find a pin from the function `%s` matching the pin `%s` [%s]"), *BlueprintFunction->GetName(), *Pin->GetName(), *GetCategoryNameChecked(Pin).ToString()))
			{
				CompilerContext.MovePinLinksToIntermediate(*Pin, *FunctionPin);
			}
			else if(AdditionalPins.Find(Pin->GetFName()))
			{
				AdditionalPins.Remove(Pin->GetFName());
				// ErroredPins.Add(Pin->GetFName());
				CompilerContext.MessageLog.Error(*FText::Format(LOCTEXT("PinNotFound", "Unable to find the parameter named '{0}' for the function '{1}' called internally from the node @@. Please reconnect the `Value` Parameter."),
					FText::FromString(Pin->GetName()), FText::FromString(BlueprintFunction->GetName())).ToString(), this);
			}
		}
	}
	
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

void UTouchInputK2Node::GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
{
	Super::GetMenuActions(ActionRegistrar);

	const UClass* Action = GetClass();

	if (ActionRegistrar.IsOpenForRegistration(Action)) {
		UBlueprintNodeSpawner* Spawner = UBlueprintNodeSpawner::Create(GetClass());
		check(Spawner != nullptr);

		ActionRegistrar.AddBlueprintAction(Action, Spawner);
	}
}

void UTouchInputK2Node::ReallocatePinsDuringReconstruction(TArray<UEdGraphPin*>& OldPins)
{
	Super::ReallocatePinsDuringReconstruction(OldPins);

	// This is necessary to retain type information after pasting or loading from disc
	if (UEdGraphPin* InputPin = FindPin(FPinNames::Value))
	{
		// Only update the output pin if it is currently a wildcard
		if (InputPin->PinType.PinCategory == UEdGraphSchema_K2::PC_Wildcard)
		{
			// Find the matching Old Pin if it exists
			for (const UEdGraphPin* OldPin : OldPins)
			{
				if (OldPin->PinName == InputPin->PinName)
				{
					// Update our output pin with the old type information and then propagate it to our input pins
					InputPin->PinType = OldPin->PinType;

					const UEdGraphSchema* Schema = GetSchema();
					Schema->RecombinePin(InputPin);

					break;
				}
			}
		}
	}
}

void UTouchInputK2Node::NotifyPinConnectionListChanged(UEdGraphPin* Pin)
{
	Super::NotifyPinConnectionListChanged(Pin);

	if (Pin->PinName == FPinNames::Value)
	{
		if (Pin->HasAnyConnections())
		{
			// Check input pin type to make sure it's a supported type for TouchEngine
			if (!IsPinCategoryValid(Pin->LinkedTo[0]))
			{
				// pin type is not valid
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

bool UTouchInputK2Node::IsPinCategoryValid(UEdGraphPin* Pin) const
{
	if(!Super::IsPinCategoryValid(Pin))
	{
		// Ensure we allow chop channel data for inputs
		const UObject* PinSubCategoryObject = Pin->PinType.PinSubCategoryObject.Get();
		if (PinSubCategoryObject && Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Struct)
		{
			return PinSubCategoryObject->GetFName() == TBaseStructure<FTouchEngineCHOP>::Get()->GetFName() ||
				PinSubCategoryObject->GetFName() == TBaseStructure<FTouchEngineCHOPChannel>::Get()->GetFName();
		}
		return false;
	}
	return true;
}

#undef LOCTEXT_NAMESPACE
