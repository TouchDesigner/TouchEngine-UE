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

#include "TouchFrameFinalizer.h"

#include "Engine/Util/TouchVariableManager.h"
#include "Rendering/TouchResourceProvider.h"

namespace UE::TouchEngine
{
	FTouchFrameFinalizer::FTouchFrameFinalizer(TSharedRef<FTouchResourceProvider> ResourceProvider, TSharedRef<FTouchVariableManager> VariableManager, TouchObject<TEInstance> TouchEngineInstance)
		: ResourceProvider(MoveTemp(ResourceProvider))
		, VariableManager(MoveTemp(VariableManager))
		, TouchEngineInstance(MoveTemp(TouchEngineInstance))
	{}
	
	void FTouchFrameFinalizer::ImportTextureForCurrentFrame_AnyThread(const FName ParamId, uint64 CookFrameNumber, TouchObject<TETexture> TextureToImport)
	{
		VariableManager->AllocateLinkedTop(ParamId); // Avoid system querying this param from generating an output error
		ResourceProvider->ImportTextureToUnrealEngine({ TouchEngineInstance, ParamId, TextureToImport })
			.Next([this, ParamId](const FTouchImportResult& TouchLinkResult)
			{
				if (TouchLinkResult.ResultType != EImportResultType::Success)
				{
					return;
				}

				UTexture2D* Texture = TouchLinkResult.ConvertedTextureObject.GetValue();
				if (IsInGameThread())
				{
					VariableManager->UpdateLinkedTOP(ParamId, Texture);
				}
				else
				{
					AsyncTask(ENamedThreads::GameThread, [WeakVariableManger = TWeakPtr<FTouchVariableManager>(VariableManager), ParamId, Texture]()
					{
						// Scenario: end PIE session > causes FlushRenderCommands > finishes the link texture task > enqueues a command on game thread > will execute when we've already been destroyed
						if (TSharedPtr<FTouchVariableManager> PinnedVariableManager = WeakVariableManger.Pin())
						{
							PinnedVariableManager->UpdateLinkedTOP(ParamId, Texture);
						}
					});
				}
			});
	}

	void FTouchFrameFinalizer::NotifyFrameFinishedCooking(uint64 CookFrameNumber)
	{
		
	}

	TFuture<FCookFrameFinalizedResult> FTouchFrameFinalizer::OnFrameFinalized_GameThread(uint64 CookFrameNumber)
	{
		return MakeFulfilledPromise<FCookFrameFinalizedResult>(FCookFrameFinalizedResult{ ECookFrameFinalizationErrorCode::RequestInvalid, CookFrameNumber }).GetFuture();
	}
}
