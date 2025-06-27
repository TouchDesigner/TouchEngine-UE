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

#include "TouchEngineEditorModule.h"

#include "AssetTypeActions_Base.h"
#include "TouchEngineDynamicVariableStruct.h"
#include "TouchEngineDynVarDetsCust.h"
#include "TouchEngineEditorLog.h"
#include "TouchEngineIntVector4.h"
#include "TouchEngineIntVector4StructCust.h"
#include "ToxAsset.h"
#include "Customization/TouchEngineComponentCustomization.h"
#include "Customization/ToxAssetCustomization.h"
#include "Factory/ToxAssetFactoryNew.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Modules/ModuleManager.h"
#include "Widgets/Notifications/SNotificationList.h"

DEFINE_LOG_CATEGORY(LogTouchEngineEditor);

#define LOCTEXT_NAMESPACE "FTouchEngineEditorModule"

EAssetTypeCategories::Type FTouchEngineEditorModule::TouchEngineAssetCategoryBit = EAssetTypeCategories::Misc;

void FTouchEngineEditorModule::StartupModule()
{
	RegisterCustomizations();
	
	RegisterAssetActions();

	if (GDynamicRHI->GetInterfaceType() == ERHIInterfaceType::Vulkan) // todo: For UE 5.6, Vulkan Textures are not supported
	{
		FDelayedAutoRegisterHelper(EDelayedRegisterRunPhase::EndOfEngineInit,[]
		{
			// Create and display a notification about the tile set being modified
			struct FNotificationHolder
			{
				TWeakPtr<SNotificationItem> Notification;
			};
			TSharedRef<FNotificationHolder> NotificationHolder = MakeShared<FNotificationHolder>();
			
			const FText NotificationText = LOCTEXT("VulkanTexturesNotSupported", "Importing and Exporting Textures is not supported on Vulkan for UE 5.6.");
			FNotificationInfo Info(NotificationText);
			Info.ExpireDuration = 20.0f;
			Info.FadeInDuration = 1.0f;
			Info.FadeOutDuration = 1.0f;
			Info.bFireAndForget = true;
			Info.bUseSuccessFailIcons = true;
			Info.Image = FAppStyle::GetBrush(TEXT("MessageLog.Warning"));
			Info.ButtonDetails.Add(FNotificationButtonInfo(
				LOCTEXT("NotificationDismiss", "Dismiss"),
				LOCTEXT("NotificationDismissToolTip", "Dismiss this notification."), 
				FSimpleDelegate::CreateLambda([NotificationHolder]()
				{
					if (const TSharedPtr<SNotificationItem> NotificationPin = NotificationHolder->Notification.Pin())
					{
						NotificationPin->SetCompletionState(SNotificationItem::CS_None);
						NotificationPin->SetFadeOutDuration(0.0f);
						NotificationPin->Fadeout();
					}
				}),
				SNotificationItem::CS_None
			));
			
			const TSharedPtr<SNotificationItem> NotificationItem = FSlateNotificationManager::Get().AddNotification(Info);
			NotificationHolder->Notification = NotificationItem;
			
		});
	}
}

void FTouchEngineEditorModule::ShutdownModule()
{
	UnregisterCustomizations();

	UnregisterAssetActions();
	return;
}

void FTouchEngineEditorModule::RegisterAssetActions()
{
	// Acquire an asset category bit for our custom "TouchEngine" category
	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
	TouchEngineAssetCategoryBit = AssetTools.RegisterAdvancedAssetCategory(FName(TEXT("TouchEngine")), LOCTEXT("TouchEngineCategory", "TouchEngine"));

	// Register New Tox Asset creation Action for Content Browser
	ToxAssetTypeActions = MakeShared<FToxAssetTypeActions>();
	AssetTools.RegisterAssetTypeActions(ToxAssetTypeActions.ToSharedRef());

}

void FTouchEngineEditorModule::UnregisterAssetActions()
{
	if (!FModuleManager::Get().IsModuleLoaded("AssetTools"))
	{
		return;
	}

	FAssetToolsModule::GetModule().Get().UnregisterAssetTypeActions(ToxAssetTypeActions.ToSharedRef());
}

void FTouchEngineEditorModule::RegisterCustomizations()
{
	using namespace UE::TouchEngineEditor::Private;

	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");

	PropertyModule.RegisterCustomClassLayout(UTouchEngineComponentBase::StaticClass()->GetFName(), FOnGetDetailCustomizationInstance::CreateStatic(&FTouchEngineComponentCustomization::MakeInstance));
	PropertyModule.RegisterCustomPropertyTypeLayout(FTouchEngineDynamicVariableContainer::StaticStruct()->GetFName(), FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FTouchEngineDynamicVariableStructDetailsCustomization::MakeInstance));
	PropertyModule.RegisterCustomPropertyTypeLayout(FTouchEngineIntVector4::StaticStruct()->GetFName(), FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FTouchEngineIntVector4StructCust::MakeInstance));

	PropertyModule.RegisterCustomClassLayout(UToxAsset::StaticClass()->GetFName(), FOnGetDetailCustomizationInstance::CreateStatic(&FToxAssetCustomization::MakeInstance));
}

void FTouchEngineEditorModule::UnregisterCustomizations()
{
	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	PropertyModule.UnregisterCustomPropertyTypeLayout(FTouchEngineDynamicVariableContainer::StaticStruct()->GetFName());
	PropertyModule.UnregisterCustomPropertyTypeLayout(FTouchEngineIntVector4::StaticStruct()->GetFName());
	PropertyModule.UnregisterCustomClassLayout(UToxAsset::StaticClass()->GetFName());
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FTouchEngineEditorModule, TouchEngineEditor)
