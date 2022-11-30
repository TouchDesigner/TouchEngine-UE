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

#include "TouchEngineComponentCustomization.h"

#include "DetailLayoutBuilder.h"
#include "Algo/AnyOf.h"
#include "Async/Async.h"
#include "Blueprint/TouchEngineComponent.h"

#define LOCTEXT_NAMESPACE "FTouchEngineComponentCustomization"

namespace UE::TouchEngineEditor::Private
{
	TSharedRef<IDetailCustomization> FTouchEngineComponentCustomization::MakeInstance()
	{
		return MakeShared<FTouchEngineComponentCustomization>();
	}

	void FTouchEngineComponentCustomization::CustomizeDetails(IDetailLayoutBuilder& InDetailBuilder)
	{
		const TSharedRef<IPropertyHandle> ToxAssetProperty = InDetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UTouchEngineComponentBase, ToxAsset));
		ToxAssetProperty->SetOnPropertyValueChanged(FSimpleDelegate::CreateSP(this, &FTouchEngineComponentCustomization::OnToxAssetChanged));
		if (IDetailPropertyRow* ToxAssetPropertyRow = InDetailBuilder.EditDefaultProperty(ToxAssetProperty))
		{
			TArray<TWeakObjectPtr<UObject>> CustomizedObjects;
			InDetailBuilder.GetObjectsBeingCustomized(CustomizedObjects);
			ToxAssetPropertyRow
				->IsEnabled(TAttribute<bool>::CreateLambda([CustomizedObjects]()
				{
					const bool bIsAnyRunningTouchEngine = Algo::AnyOf(CustomizedObjects, [](TWeakObjectPtr<UObject> Object)
					{
						return Object.IsValid() && Cast<UTouchEngineComponentBase>(Object.Get())->IsRunning();
					});
					return !bIsAnyRunningTouchEngine;
				}))
				.ToolTip(FText::Format(
					LOCTEXT("ToxAssetTooltipFmt", "{0}\nThis property can only be modified while the file is not loaded by this component's TouchEngine instance"),	
					ToxAssetProperty->GetToolTipText()
					)
				);
		}
	}

	void FTouchEngineComponentCustomization::CustomizeDetails(const TSharedPtr<IDetailLayoutBuilder>& InDetailBuilder)
	{
		DetailBuilder = InDetailBuilder;
		IDetailCustomization::CustomizeDetails(InDetailBuilder);
	}

	void FTouchEngineComponentCustomization::OnToxAssetChanged() const
	{
		// Here we can only take the ptr as ForceRefreshDetails() checks that the reference is unique.
		if (IDetailLayoutBuilder* DetailBuilderValuePtr = DetailBuilder.Pin().Get())
		{
			DetailBuilderValuePtr->ForceRefreshDetails();
		}
	}
}

#undef LOCTEXT_NAMESPACE
