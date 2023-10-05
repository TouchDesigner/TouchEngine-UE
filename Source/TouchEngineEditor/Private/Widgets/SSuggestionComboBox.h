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

#include "CoreMinimal.h"
#include "SOpenableComboBox.h"
#include "Styling/SlateColor.h"
#include "Fonts/SlateFontInfo.h"
#include "Input/Reply.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"
#include "Styling/SlateTypes.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Notifications/SErrorText.h"
#include "Widgets/Views/STableViewBase.h"
#include "Widgets/Views/STableRow.h"

class SMenuAnchor;
class SVerticalBox;

template< typename ObjectType > class TAttribute;

class TOUCHENGINEEDITOR_API SSuggestionComboBox
	: public SOpenableComboBox
{
public:
	/** Type of list used for showing menu options. */
	typedef SListView< TSharedPtr<FString> > SComboListType;
	/** Delegate type used to generate widgets that represent Options */
	typedef typename TSlateDelegates< TSharedPtr<FString> >::FOnGenerateWidget FOnGenerateWidget;

	/** A delegate type for OnGenerateWidget handler. Given a data item, the handler should return a Widget visualizing that item */
	DECLARE_DELEGATE_RetVal_TwoParams (
	/** return: The Widget visualization of the item */
		TSharedRef<SWidget>,
		FOnGenerateSuggestionWidget,
		/** param: An item to visualize */
		TSharedPtr<FString>, TSharedPtr<SSuggestionComboBox> );

	
	SLATE_BEGIN_ARGS(SSuggestionComboBox)
		: _ComboBoxStyle(&FAppStyle::Get().GetWidgetStyle<FComboBoxStyle>("ComboBox"))
		, _ButtonStyle(nullptr)
		, _Text()
	{ }
		
		SLATE_STYLE_ARGUMENT(FComboBoxStyle, ComboBoxStyle)
		/** The visual style of the button part of the combo box (overrides ComboBoxStyle) */
		SLATE_STYLE_ARGUMENT(FButtonStyle, ButtonStyle)

		/** Called whenever the text is changed programmatically or interactively by the user. */
		SLATE_EVENT(FOnTextChanged, OnTextChanged)

		/** Called when the text has been committed. */
		SLATE_EVENT(FOnTextCommitted, OnTextCommitted)

		SLATE_ARGUMENT(const TArray< TSharedPtr<FString> >*, OptionsSource)
		/** Called when combo box is opened, before list is actually created */
		SLATE_EVENT(FOnComboBoxOpening, OnComboBoxOpening)
		/** Called when combo box is opened, before list is actually created */
		SLATE_EVENT(FOnIsOpenChanged, OnMenuOpenChanged)
		SLATE_EVENT(FOnGenerateSuggestionWidget, OnGenerateWidget)

		/** Sets the text content for this editable text box widget. */
		SLATE_ATTRIBUTE(FText, Text)

	SLATE_END_ARGS()

public:
	/**
	 * Construct this widget.  Called by the SNew() Slate macro.
	 *
	 * @param InArgs Declaration used by the SNew() macro to construct this widget.
	 */
	void Construct( const FArguments& InArgs );

	/**
	 * Returns the text string.
	 *
	 * @return The text string.
	 */
	FText GetText( ) const
	{
		return TextBox->GetText();
	}
	
	/**
	 * Sets the text string currently being edited 
	 *
	 * @param InNewText The new text string.
	 */
	void SetText( const TAttribute< FText >& InNewText );

protected:
	/** Sets the keyboard focus to the text box. */
	void FocusTextBox( ) const;
	virtual FReply OnKeyDown( const FGeometry& MyGeometry, const FKeyEvent& KeyEvent ) override;
private:
	// Callback for changing the text box input.
	void HandleTextBoxTextChanged( const FText& InText );

	// Callback committing the text box input.
	void HandleTextBoxTextCommitted( const FText& InText, ETextCommit::Type CommitInfo );

	// Called when global focus changes so we can detect when to close the SuggestionList.
	void OnGlobalFocusChanging(const FFocusEvent& FocusEvent, const FWeakWidgetPath& OldFocusedWidgetPath, const TSharedPtr<SWidget>& OldFocusedWidget, const FWidgetPath& NewFocusedWidgetPath, const TSharedPtr<SWidget>& NewFocusedWidget);

	TSharedRef<SWidget> HandleOnGenerateWidget(TSharedPtr<FString> String);

	void HandleOnMenuOpenChanged(bool bOpen);

	// Holds a flag to prevent recursive calls in UI callbacks.
	bool IgnoreUIUpdate = false; 
	
	// Holds the editable text box.
	TSharedPtr<SEditableTextBox> TextBox;

	/** Handle clicking on the content menu */
	virtual FReply OnButtonClicked() override;
	
	// Holds a delegate that is executed when the text has changed.
	FOnTextChanged OnTextChanged;

	// Holds a delegate that is executed when the text has been committed.
	FOnTextCommitted OnTextCommitted;

	/** Delegate to invoke when the combo box is opening or closing. */
	FOnIsOpenChanged OnMenuOpenChanged;
	/** Delegate to invoke when we need to visualize an option as a widget. */
	FOnGenerateSuggestionWidget OnGenerateWidget;
};