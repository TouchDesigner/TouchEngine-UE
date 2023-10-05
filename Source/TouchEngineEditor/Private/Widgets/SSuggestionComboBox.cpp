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

#include "SSuggestionComboBox.h"
#include "Layout/WidgetPath.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/Input/SMenuAnchor.h"
#include "Widgets/Views/SListView.h"


void SSuggestionComboBox::Construct(const FArguments& InArgs)
{
	OnTextChanged = InArgs._OnTextChanged;
	OnTextCommitted = InArgs._OnTextCommitted;
	OnGenerateWidget = InArgs._OnGenerateWidget;
	OnMenuOpenChanged = InArgs._OnMenuOpenChanged;

	FSlateApplication::Get().OnFocusChanging().AddSP(this, &SSuggestionComboBox::OnGlobalFocusChanging);

	const FComboButtonStyle& OurComboButtonStyle = InArgs._ComboBoxStyle->ComboButtonStyle;
	FButtonStyle OurButtonStyle = InArgs._ButtonStyle ? *InArgs._ButtonStyle : OurComboButtonStyle.ButtonStyle;
	OurButtonStyle.PressedPadding = FMargin(0);
	OurButtonStyle.NormalPadding = FMargin(0);
	
	SOpenableComboBox::Construct(SOpenableComboBox::FArguments()
		.Content()
		[
			SAssignNew(TextBox, SEditableTextBox)
			.ClearKeyboardFocusOnCommit(false)
			.IsCaretMovedWhenGainFocus(true)
			.RevertTextOnEscape(false)
			.SelectAllTextOnCommit(false)
			.SelectAllTextWhenFocused(false)
			.Text(InArgs._Text)
			.OnKeyDownHandler(this, &SSuggestionComboBox::OnKeyDown)
			.OnTextChanged(this, &SSuggestionComboBox::HandleTextBoxTextChanged)
			.OnTextCommitted(this, &SSuggestionComboBox::HandleTextBoxTextCommitted)
		]
		.ContentPadding(FMargin(0))
		.OptionsSource(InArgs._OptionsSource)
		.OnGenerateWidget(this, &SSuggestionComboBox::HandleOnGenerateWidget)
		.OnSelectionCommitted_Lambda([this](TSharedPtr<FString> ProposedSelection, ESelectInfo::Type SelectInfo)
		{
			if (!IgnoreUIUpdate && ProposedSelection)
			{
				const FString Selection = *ProposedSelection;
				UE_LOG(LogTemp,Log, TEXT("OnSelectionCommitted_Lambda: %s  [%s]"), *Selection, *UEnum::GetValueAsString(SelectInfo))
				TextBox->SetText(FText::FromString(Selection));
				HandleTextBoxTextCommitted(TextBox->GetText(), ETextCommit::OnEnter);
			}
		})
		.OnMenuOpenChanged(this, &SSuggestionComboBox::HandleOnMenuOpenChanged)
	);
}

void SSuggestionComboBox::SetText(const TAttribute<FText>& InNewText)
{
	IgnoreUIUpdate = true;
	TextBox->SetText(InNewText);
	IgnoreUIUpdate = false;
}

void SSuggestionComboBox::FocusTextBox() const
{
	FWidgetPath TextBoxPath;

	FSlateApplication::Get().GeneratePathToWidgetUnchecked(TextBox.ToSharedRef(), TextBoxPath);
	FSlateApplication::Get().SetKeyboardFocus(TextBoxPath, EFocusCause::SetDirectly);
}

FReply SSuggestionComboBox::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& KeyEvent)
{
	const FString& InputTextStr = TextBox->GetText().ToString();

	if (KeyEvent.GetKey() == EKeys::Enter)
	{
		HandleTextBoxTextCommitted(TextBox->GetText(), ETextCommit::OnEnter);
		return FReply::Handled();
	}
	if (KeyEvent.GetKey() == EKeys::Up)
	{
		if(!IsOpen())
		{
			RefreshOptions(TextBox->GetText());
			SetIsOpen(false, false);
		}
		// // forward navigate the list of suggestions
		// const TArray<TSharedPtr<FString>>& Options = GetOptions();
		// if (!Options.IsEmpty())
		// {
		// 	const TSharedPtr<FString> Item = GetSelectedItem();
		// 	const int32 Index = Item ? Options.Find(Item) : INDEX_NONE;
		// 	if (Index == INDEX_NONE)
		// 	{
		// 		SetSelectedItem(Options[0]);
		// 	}
		// 	else if (Index == INDEX_NONE || Index + 1 >= Options.Num())
		// 	{
		// 		SetSelectedItem(Options[0]);
		// 	}
		// 	else
		// 	{
		// 		SetSelectedItem(Options[Index+1]);
		// 	}
		// }
		// else
		// {
		// 	ClearSelection();
		// }
		
		return FReply::Handled();
	}
	else if(KeyEvent.GetKey() == EKeys::Down)
	{
		if(!IsOpen())
		{
			RefreshOptions(TextBox->GetText());
			SetIsOpen(false, false);
		}
		// // forward navigate the list of suggestions
		// const TArray<TSharedPtr<FString>>& Options = GetOptions();
		// if (!Options.IsEmpty())
		// {
		// 	const TSharedPtr<FString> Item = GetSelectedItem();
		// 	const int32 Index = Item ? Options.Find(Item) : INDEX_NONE;
		// 	if (Index == INDEX_NONE || Index + 1 >= Options.Num())
		// 	{
		// 		SetSelectedItem(Options[0]);
		// 	}
		// 	else
		// 	{
		// 		SetSelectedItem(Options[Index+1]);
		// 	}
		// }
		// else
		// {
		// 	ClearSelection();
		// }
	
		return FReply::Handled();
	}
	// else if (KeyEvent.GetKey() == EKeys::Tab)
	// {
	// 	// auto-complete the highlighted suggestion
	// 	if (Suggestions.Num())
	// 	{
	// 		if ((SelectedSuggestion >= 0) && (SelectedSuggestion < Suggestions.Num()))
	// 		{
	// 			MarkActiveSuggestion();
	// 			HandleTextBoxTextCommitted(TextBox->GetText(), ETextCommit::OnEnter);
	// 		}
	// 		else
	// 		{
	// 			SelectedSuggestion = 0;
	//
	// 			MarkActiveSuggestion();
	// 		}
	// 	}
	//
	// 	return FReply::Handled();
	// }

	return SOpenableComboBox::OnKeyDown(MyGeometry, KeyEvent);
}

void SSuggestionComboBox::HandleTextBoxTextChanged(const FText& InText)
{
	SetText(InText);
	OnTextChanged.ExecuteIfBound(InText);
	RefreshOptions(InText);
}

void SSuggestionComboBox::HandleTextBoxTextCommitted(const FText& InText, ETextCommit::Type CommitInfo)
{
	OnTextCommitted.ExecuteIfBound(InText, CommitInfo);
	if (!IgnoreUIUpdate)
	{
		IgnoreUIUpdate = true;
		RefreshOptions();
		this->SetIsOpen(false, false);
		
		if (TextBox->HasKeyboardFocus())
		{
			TextBox->SelectAllText();
		}
		else
		{
			TextBox->ClearSelection();
		}
		IgnoreUIUpdate = false;
	}
}

void SSuggestionComboBox::OnGlobalFocusChanging(const FFocusEvent& FocusEvent, const FWeakWidgetPath& OldFocusedWidgetPath, const TSharedPtr<SWidget>& OldFocusedWidget, const FWidgetPath& NewFocusedWidgetPath, const TSharedPtr<SWidget>& NewFocusedWidget)
{
	if (TextBox.IsValid() && NewFocusedWidgetPath.ContainsWidget(TextBox.Get())) // if we have the textbox focused
	{
		if (!IgnoreUIUpdate)
		{
			IgnoreUIUpdate = true;
			RefreshOptions();
			this->SetIsOpen(true, false);
			IgnoreUIUpdate = false;
		}
	}
	else if (TextBox.IsValid() && OldFocusedWidgetPath.ContainsWidget(TextBox.Get()) && FocusEvent.GetCause() == EFocusCause::Mouse)
	{
		// If the textbox has lost focus and the SuggestionList has not gained it, then we assume the user clicked somewhere else in the app and clear the SuggestionList.
		// if (VerticalBox.IsValid() && !NewFocusedWidgetPath.ContainsWidget(VerticalBox.Get()))
		// {
		// 	ClearSuggestions();
		// 	bool Result = OnTextCommitted.ExecuteIfBound(TextBox->GetText(), ETextCommit::Type::OnUserMovedFocus);
		// 	return;
		// }
		TextBox->ClearSelection();
	}

}

TSharedRef<SWidget> SSuggestionComboBox::HandleOnGenerateWidget(TSharedPtr<FString> String)
{
	if (OnGenerateWidget.IsBound())
	{
		return OnGenerateWidget.Execute(String, SharedThis(this).ToSharedPtr());
	}
	return SNew(STextBlock).Text(NSLOCTEXT("SlateCore", "ComboBoxMissingOnGenerateWidgetMethod", "Please provide a .OnGenerateWidget() handler."));
}

void SSuggestionComboBox::HandleOnMenuOpenChanged(bool bOpen)
{
	ClearSelection();
	OnMenuOpenChanged.ExecuteIfBound(bOpen);
}

FReply SSuggestionComboBox::OnButtonClicked()
{
	RefreshOptions();
	FReply Reply = SOpenableComboBox::OnButtonClicked();

	return Reply;
}
