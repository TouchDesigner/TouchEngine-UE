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


/* SSuggestionComboBox structors
 *****************************************************************************/


SSuggestionComboBox::SSuggestionComboBox( )
	: IgnoreUIUpdate(false)
	, SelectedSuggestion(-1)
	, SuggestionTextStyle(nullptr)
{
}


/* SSuggestionComboBox interface
 *****************************************************************************/

void SSuggestionComboBox::Construct( const FArguments& InArgs )
{
	SuggestionTextStyle = InArgs._SuggestionTextStyle;

	OnShowingSuggestions = InArgs._OnShowingSuggestions;
	OnTextChanged = InArgs._OnTextChanged;
	OnTextCommitted = InArgs._OnTextCommitted;

	// Work out which values we should use based on whether we were given an override, or should use the style's version
	const FComboBoxStyle& OurComboBoxStyle = FAppStyle::Get().GetWidgetStyle<FComboBoxStyle>("ComboBox");
	const FComboButtonStyle& OurComboButtonStyle = OurComboBoxStyle.ComboButtonStyle; // FAppStyle::Get().GetWidgetStyle< FComboButtonStyle >( "ComboButton" );
	const FButtonStyle* const OurButtonStyle = &OurComboButtonStyle.ButtonStyle;
	ItemStyle = &FAppStyle::Get().GetWidgetStyle<FTableRowStyle>("ComboBox.Row");
	MenuRowPadding = OurComboBoxStyle.MenuRowPadding;

	FSlateApplication::Get().OnFocusChanging().AddSP(this, &SSuggestionComboBox::OnGlobalFocusChanging);

	ChildSlot
	[
		SAssignNew(MenuAnchor, SMenuAnchor)
		.Placement(MenuPlacement_ComboBox)
		[
			SAssignNew(TextBox, SEditableTextBox)
			.BackgroundColor(InArgs._BackgroundColor)
			.ClearKeyboardFocusOnCommit(InArgs._ClearKeyboardFocusOnCommit)
			.ErrorReporting(InArgs._ErrorReporting)
			.Font(InArgs._Font)
			.ForegroundColor(InArgs._ForegroundColor)
			.HintText(InArgs._HintText)
			.IsCaretMovedWhenGainFocus(InArgs._IsCaretMovedWhenGainFocus)
			.MinDesiredWidth(InArgs._MinDesiredWidth)
			.RevertTextOnEscape(InArgs._RevertTextOnEscape)
			.SelectAllTextOnCommit(InArgs._SelectAllTextOnCommit)
			.SelectAllTextWhenFocused(InArgs._SelectAllTextWhenFocused)
			.Style(InArgs._TextStyle)
			.Text(InArgs._Text)
			.OnKeyDownHandler(this, &SSuggestionComboBox::OnKeyDown)
			.OnTextChanged(this, &SSuggestionComboBox::HandleTextBoxTextChanged)
			.OnTextCommitted(this, &SSuggestionComboBox::HandleTextBoxTextCommitted)
		]
		.MenuContent
		(
			SNew(SBorder)
			.BorderImage(InArgs._BackgroundImage)
			.Padding(OurComboButtonStyle.MenuBorderPadding)
			[
				SAssignNew(VerticalBox, SVerticalBox)
				
				+ SVerticalBox::Slot()
				.AutoHeight()
				.MaxHeight(InArgs._SuggestionListMaxHeight)
				[
					SAssignNew(SuggestionListView, SComboListType)
					.ListItemsSource(&Suggestions)
					.OnGenerateRow(this, &SSuggestionComboBox::HandleSuggestionListViewGenerateRow)
					.OnSelectionChanged(this,&SSuggestionComboBox::HandleSuggestionListViewSelectionChanged)
					.SelectionMode(ESelectionMode::Single)
				]
			]
		)
	];
}


void SSuggestionComboBox::SetText( const TAttribute<FText>& InNewText )
{
	IgnoreUIUpdate = true;

	TextBox->SetText(InNewText);

	IgnoreUIUpdate = false;
}


/* SSuggestionComboBox implementation
 *****************************************************************************/

void SSuggestionComboBox::ClearSuggestions( )
{
	SelectedSuggestion = -1;

	MenuAnchor->SetIsOpen(false);
	Suggestions.Empty();
}


void SSuggestionComboBox::FocusTextBox( ) const
{
	FWidgetPath TextBoxPath;

	FSlateApplication::Get().GeneratePathToWidgetUnchecked(TextBox.ToSharedRef(), TextBoxPath);
	FSlateApplication::Get().SetKeyboardFocus(TextBoxPath, EFocusCause::SetDirectly);
}


FString SSuggestionComboBox::GetSelectedSuggestionString( ) const
{
	const FString SuggestionString = *Suggestions[SelectedSuggestion];

	return SuggestionString.Replace(TEXT("\t"), TEXT(""));
}


void SSuggestionComboBox::MarkActiveSuggestion( )
{
	IgnoreUIUpdate = true;

	if (SelectedSuggestion >= 0)
	{
		const TSharedPtr<FString> Suggestion = Suggestions[SelectedSuggestion];

		SuggestionListView->SetSelection(Suggestion);

		if (!SuggestionListView->IsItemVisible(Suggestion))
		{
			SuggestionListView->RequestScrollIntoView(Suggestion);
		}

		TextBox->SetText(FText::FromString(GetSelectedSuggestionString()));
	}
	else
	{
		SuggestionListView->ClearSelection();
	}

	IgnoreUIUpdate = false;
}


void SSuggestionComboBox::SetSuggestions( TArray<FString>& SuggestionStrings, bool InHistoryMode )
{
	FString SelectionText;

	// cache previously selected suggestion
	if ((SelectedSuggestion >= 0) && (SelectedSuggestion < Suggestions.Num()))
	{
		SelectionText = *Suggestions[SelectedSuggestion];
	}

	SelectedSuggestion = -1;

	// refill suggestions
	Suggestions.Empty();

	for (int32 i = 0; i < SuggestionStrings.Num(); ++i)
	{
		Suggestions.Add(MakeShareable(new FString(SuggestionStrings[i])));

		if (SuggestionStrings[i] == SelectionText)
		{
			SelectedSuggestion = i;
		}
	}

	if (Suggestions.Num())
	{
		// @todo Slate: make the window title not flicker when the box toggles visibility
		MenuAnchor->SetIsOpen(true, false);
		SuggestionListView->RequestScrollIntoView(Suggestions.Last());

		FocusTextBox();
	}
	else
	{
		MenuAnchor->SetIsOpen(false);
	}
}


/* SWidget overrides
 *****************************************************************************/

void SSuggestionComboBox::OnFocusLost( const FFocusEvent& InFocusEvent )
{
	//	MenuAnchor->SetIsOpen(false);
}


FReply SSuggestionComboBox::OnKeyDown( const FGeometry& MyGeometry, const FKeyEvent& KeyEvent )
{
	const FString& InputTextStr = TextBox->GetText().ToString();

	if (KeyEvent.GetKey() == EKeys::Up)
	{
		// backward navigate the list of suggestions
		if (SelectedSuggestion < 0)
		{
			SelectedSuggestion = Suggestions.Num() - 1;
		}
		else
		{
			--SelectedSuggestion;
		}

		MarkActiveSuggestion();

		return FReply::Handled();
	}
	else if(KeyEvent.GetKey() == EKeys::Down)
	{
		// forward navigate the list of suggestions
		if (SelectedSuggestion < Suggestions.Num() - 1)
		{
			++SelectedSuggestion;
		}
		else
		{
			SelectedSuggestion = -1;
		}

		MarkActiveSuggestion();

		return FReply::Handled();
	}
	else if (KeyEvent.GetKey() == EKeys::Tab)
	{
		// auto-complete the highlighted suggestion
		if (Suggestions.Num())
		{
			if ((SelectedSuggestion >= 0) && (SelectedSuggestion < Suggestions.Num()))
			{
				MarkActiveSuggestion();
				HandleTextBoxTextCommitted(TextBox->GetText(), ETextCommit::OnEnter);
			}
			else
			{
				SelectedSuggestion = 0;

				MarkActiveSuggestion();
			}
		}

		return FReply::Handled();
	}

	return FReply::Unhandled();
}


bool SSuggestionComboBox::SupportsKeyboardFocus() const
{
	return true;
}


/* SSuggestionComboBox callbacks
 *****************************************************************************/

TSharedRef<ITableRow> SSuggestionComboBox::HandleSuggestionListViewGenerateRow( TSharedPtr<FString> Text, const TSharedRef<STableViewBase>& OwnerTable )
{
	FString Left, Right, Combined;

	if (Text->Split(TEXT("\t"), &Left, &Right))
	{
		Combined = Left + Right;
	}
	else
	{
		Combined = *Text;
	}

	return SNew(STableRow<TSharedPtr<FString> >, OwnerTable)
		.Style(ItemStyle)
		.Padding(MenuRowPadding)
		[
			SNew(SBox)
			[
				SNew(STextBlock)
				.HighlightText(this, &SSuggestionComboBox::HandleSuggestionListWidgetHighlightText)
				// .TextStyle( TextStyle )
				.Text( FText::FromString(Combined) )						
			]
		];
}


void SSuggestionComboBox::HandleSuggestionListViewSelectionChanged( TSharedPtr<FString> NewValue, ESelectInfo::Type SelectInfo )
{
	if (!IgnoreUIUpdate)
	{
		for (int32 i = 0; i < Suggestions.Num(); ++i)
		{
			if (NewValue == Suggestions[i])
			{
				SelectedSuggestion = i;

				MarkActiveSuggestion();
				FocusTextBox();

				break;
			}
		}
	}
}


FText SSuggestionComboBox::HandleSuggestionListWidgetHighlightText( ) const
{
	return TextBox->GetText();
}

void SSuggestionComboBox::HandleTextBoxTextChanged( const FText& InText )
{
	OnTextChanged.ExecuteIfBound(InText);

	if (!IgnoreUIUpdate)
	{
		const FString& InputTextStr = TextBox->GetText().ToString();

		if (OnShowingSuggestions.IsBound()) // !InputTextStr.IsEmpty() && 
		{
			TArray<FString> SuggestionStrings;

			OnShowingSuggestions.ExecuteIfBound(InText.ToString(), SuggestionStrings);

			for (int32 Index = 0; Index < SuggestionStrings.Num(); ++Index)
			{
				FString& StringRef = SuggestionStrings[Index];

				StringRef = StringRef.Left(InputTextStr.Len()) + TEXT("\t") + StringRef.RightChop(InputTextStr.Len());
			}

			SetSuggestions(SuggestionStrings, false);
		}
		else
		{
			ClearSuggestions();
		}
	}
}


void SSuggestionComboBox::HandleTextBoxTextCommitted( const FText& InText, ETextCommit::Type CommitInfo )
{
	if (!MenuAnchor->IsOpen())
	{
		OnTextCommitted.ExecuteIfBound(InText, CommitInfo);
	}

	if ((CommitInfo == ETextCommit::OnEnter) || (CommitInfo == ETextCommit::OnCleared))
	{
		ClearSuggestions();
	}
}


void SSuggestionComboBox::OnGlobalFocusChanging(const FFocusEvent& FocusEvent, const FWeakWidgetPath& OldFocusedWidgetPath, const TSharedPtr<SWidget>& OldFocusedWidget, const FWidgetPath& NewFocusedWidgetPath, const TSharedPtr<SWidget>& NewFocusedWidget)
{
	if (TextBox.IsValid() && OldFocusedWidgetPath.ContainsWidget(TextBox.Get()) && FocusEvent.GetCause() == EFocusCause::Mouse)
	{
		// If the textbox has lost focus and the SuggestionList has not gained it, then we assume the user clicked somewhere else in the app and clear the SuggestionList.
		if (VerticalBox.IsValid() && !NewFocusedWidgetPath.ContainsWidget(VerticalBox.Get()))
		{
			ClearSuggestions();
			bool Result = OnTextCommitted.ExecuteIfBound(TextBox->GetText(), ETextCommit::Type::OnUserMovedFocus);
			return;
		}
	}
	if (TextBox.IsValid() && NewFocusedWidgetPath.ContainsWidget(TextBox.Get()))
	{
		if (!IgnoreUIUpdate)
		{
			IgnoreUIUpdate = true;
			const FString& InputTextStr = TextBox->GetText().ToString();
			if (OnShowingSuggestions.IsBound())
			{
				TArray<FString> SuggestionStrings;
				OnShowingSuggestions.ExecuteIfBound(InputTextStr, SuggestionStrings);

				for (int32 Index = 0; Index < SuggestionStrings.Num(); ++Index)
				{
					FString& StringRef = SuggestionStrings[Index];

					StringRef = StringRef.Left(InputTextStr.Len()) + TEXT("\t") + StringRef.RightChop(InputTextStr.Len());
				}

				SetSuggestions(SuggestionStrings, false);
			}
			else
			{
				ClearSuggestions();
			}
			IgnoreUIUpdate = false;
		}
	}
}
