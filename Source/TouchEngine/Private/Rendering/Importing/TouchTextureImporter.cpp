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

#include "Rendering/Importing/TouchTextureImporter.h"

#include "Logging.h"
#include "Rendering/Importing/ITouchImportTexture.h"
#include "Rendering/TouchResourceProvider.h"

#include "Algo/ForEach.h"
#include "Engine/Util/TouchFrameCooker.h"
#include "Util/TouchHelpers.h"

namespace UE::TouchEngine
{
	FTouchTextureImporter::~FTouchTextureImporter()
	{
		UE_LOG(LogTouchEngine, Verbose, TEXT("Shutting down ~FTouchTextureLinker"));

		// We don't need to care about removing textures from root set in this situation (in fact scheduling a game thread task will not work)
		if (IsEngineExitRequested())
		{
			return;
		}
		
		TArray<UTexture*> TexturesToCleanUp;
		for (TPair<FName, FTouchTextureLinkData>& Data : LinkData)
		{
			if (IsValid(Data.Value.UnrealTexture))
			{
				TexturesToCleanUp.Add(Data.Value.UnrealTexture);
			}
		}

		ExecuteOnGameThread<void>([TexturesToCleanUp]()
		{
			Algo::ForEach(TexturesToCleanUp, [](UTexture* Texture){ Texture->RemoveFromRoot(); });
		});
	}
	
	TFuture<FTouchTextureImportResult> FTouchTextureImporter::ImportTexture_AnyThread(const FTouchImportParameters& LinkParams, TSharedPtr<FTouchFrameCooker> FrameCooker)
	{
		if (TaskSuspender.IsSuspended())
		{
			UE_LOG(LogTouchEngine, Warning, TEXT("FTouchTextureLinker is suspended. Your task will be ignored."));
			return MakeFulfilledPromise<FTouchTextureImportResult>(FTouchTextureImportResult{ EImportResultType::Cancelled }).GetFuture();
		}

		// Here we just want to start the texture transfer, but this will be processed later on in the render thread
		LinkParams.GetTextureTransferResult = GetTextureTransfer(LinkParams);
		
		FTaskSuspender::FTaskTracker TaskToken = TaskSuspender.StartTask();
		TPromise<FTouchTextureImportResult> Promise;
		TFuture<FTouchTextureImportResult> Future = Promise.GetFuture()
			.Next([TaskToken](auto Result)
			{
				return Result;
			});

		FTouchTextureLinkData& TextureLinkData = LinkData.FindOrAdd(LinkParams.ParameterName);
		if (TextureLinkData.bIsInProgress)
		{
			//todo: check when this happens
			EnqueueLinkTextureRequest(TextureLinkData, MoveTemp(Promise), LinkParams);
		}
		else
		{
			ExecuteLinkTextureRequest_AnyThread(MoveTemp(Promise), LinkParams, FrameCooker);
		}
		
		return Future;
	}

	TFuture<FTouchSuspendResult> FTouchTextureImporter::SuspendAsyncTasks()
	{
		TPromise<FTouchSuspendResult> Promise;
		TFuture<FTouchSuspendResult> Future = Promise.GetFuture();

		ENQUEUE_RENDER_COMMAND(FinishRemainingTasks)([ThisPin = SharedThis(this), Promise = MoveTemp(Promise)](FRHICommandListImmediate& RHICmdList) mutable
		{
			for (TPair<FName, FTouchTextureLinkData>& Data : ThisPin->LinkData)
			{
				if (Data.Value.ExecuteNext)
				{
					Data.Value.ExecuteNext->SetValue(FTouchTextureImportResult{ EImportResultType::Cancelled });
					Data.Value.ExecuteNext.Reset();
				}
			}
			ThisPin->TaskSuspender.Suspend().Next([Promise = MoveTemp(Promise)](auto) mutable
			{
				Promise.SetValue({});
			});
		});
		return Future; ;
	}

	TEResult FTouchTextureImporter::GetTextureTransfer(const FTouchImportParameters& ImportParams)
	{
		ImportParams.GetTextureTransferResult = TEInstanceGetTextureTransfer(ImportParams.Instance, ImportParams.Texture, ImportParams.GetTextureTransferSemaphore.take(), &ImportParams.GetTextureTransferWaitValue);
		return ImportParams.GetTextureTransferResult;
	}

	void FTouchTextureImporter::ExecuteLinkTextureRequest_AnyThread(TPromise<FTouchTextureImportResult>&& Promise, const FTouchImportParameters& LinkParams, TSharedPtr<FTouchFrameCooker> FrameCooker)
	{
		// At this point, we are neither on the GameThread nor on the RenderThread, we are on a parallel thread.
		// A UTexture2D can only be created on the GameThread (especially calling UTexture2D::UpdateResource) so if we need to create one, we will need to send the necessary information to the GameThread.
		// In synchronised mode though, we are already waiting on the cook promise at this point, so we cannot create it right now.
		// Once the TEEventFrameDidFinish occurs, the GameThread will restart and we will create them at that point and update the  before being able to call GetOutputs.

		
		// 1. We get the texture sent by TouchEngine
		const TSharedPtr<ITouchImportTexture> ImportTexture = CreatePlatformTexture_AnyThread(LinkParams.Instance, LinkParams.Texture);
		FTouchTextureLinkJob PlatformTexture { LinkParams, ImportTexture, nullptr, ImportTexture ? ETouchLinkErrorCode::Success : ETouchLinkErrorCode::FailedToCreatePlatformTexture };
		
		// 2. Check if we already have a UTexture that could hold the data from TouchEngine, or enqueue one to be created on GameThread
		FTextureCreationFormat TextureFormat;
		TextureFormat.Identifier = PlatformTexture.RequestParams.ParameterName;
		TextureFormat.OnTextureCreated = MakeShared<TPromise<UTexture2D*>>(); // This promise will be set from the GameThread, unless we have a UTexture available or if we have an issue
		TFuture<UTexture2D*> OnTextureCreated = TextureFormat.OnTextureCreated->GetFuture(); // Creating this Future as TextureFormat might be moved
		
		const FName Identifier = PlatformTexture.RequestParams.ParameterName;
		if (PlatformTexture.ErrorCode != ETouchLinkErrorCode::Success || !ensureMsgf(PlatformTexture.PlatformTexture, TEXT("Operation wrongly marked successful")))
		{
			UE_LOG(LogTouchEngine, Error, TEXT("[ExecuteLinkTextureRequest_AnyThread[%s]] Unable to create a platform texture for `%s`"),
				*GetCurrentThreadStr(), *Identifier.ToString());
			TextureFormat.OnTextureCreated->SetValue(nullptr);
		}
		else
		{
			// Common case: We already have a UTexture that can hold the data, early out and continue operations
			const FTouchTextureLinkData& TextureLinkData = LinkData[Identifier];
			if (TextureLinkData.UnrealTexture && PlatformTexture.PlatformTexture->CanCopyInto(TextureLinkData.UnrealTexture))
			{
				const FTextureMetaData TextureData = PlatformTexture.PlatformTexture->GetTextureMetaData();
				UE_LOG(LogTouchEngine, Log, TEXT("[ExecuteLinkTextureRequest_AnyThread[%s]] Reusing existing UTexture `%s` for parameter `%s`: %dx%d [%s]"),
					*GetCurrentThreadStr(), *TextureLinkData.UnrealTexture->GetName(), *Identifier.ToString(), TextureData.SizeX, TextureData.SizeY, GetPixelFormatString(TextureData.PixelFormat));

				TextureFormat.OnTextureCreated->SetValue(TextureLinkData.UnrealTexture);
			}
			else
			{
				// if not, we gather the necessary information to create it on the GameThread when we can
				const FTextureMetaData TextureData = PlatformTexture.PlatformTexture->GetTextureMetaData();
				if (!ensure(TextureData.PixelFormat != PF_Unknown))
				{
					UE_LOG(LogTouchEngine, Error, TEXT("[ExecuteLinkTextureRequest_AnyThread[%s]] The PlatformMetadata has an unknown Pixel format `%s` for parameter `%s`"),
					       *GetCurrentThreadStr(), GetPixelFormatString(TextureData.PixelFormat), *Identifier.ToString());

					PlatformTexture.ErrorCode = ETouchLinkErrorCode::FailedToCreateUnrealTexture;
					TextureFormat.OnTextureCreated->SetValue(nullptr);
				}
				else
				{
					UE_LOG(LogTouchEngine, Log, TEXT("[ExecuteLinkTextureRequest_AnyThread[%s]] Need to create new UTexture for parameter `%s`: %dx%d [%s]"),
					       *GetCurrentThreadStr(), *Identifier.ToString(), TextureData.SizeX, TextureData.SizeY, GetPixelFormatString(TextureData.PixelFormat));

					TextureFormat.SizeX = TextureData.SizeX;
					TextureFormat.SizeY = TextureData.SizeY;
					TextureFormat.PixelFormat = TextureData.PixelFormat;

					FrameCooker->AddTextureToCreateOnGameThread(MoveTemp(TextureFormat));
				}
			}
		}
		
		// 3. When the UTexture is created, we add it to our pool as it might be reused next frame
		TFuture<FTouchTextureLinkJob> TextureCreationOperation = OnTextureCreated.Next(
			[WeakThis = TWeakPtr<FTouchTextureImporter>(SharedThis(this)), PlatformTexture = MoveTemp(PlatformTexture)](UTexture2D* Texture) mutable
			{
				// here we could either be on a parallel thread or on the GameThread, depending on if we had to create UTextures
				PlatformTexture.UnrealTexture = Texture;
				if (const TSharedPtr<FTouchTextureImporter> ThisPin = WeakThis.Pin())
				{
					//todo: lock LinkData
					FTouchTextureLinkData& TextureLinkData = ThisPin->LinkData[PlatformTexture.RequestParams.ParameterName];
					if (UTexture2D* PreviousTexture = TextureLinkData.UnrealTexture)
					{
						PreviousTexture->RemoveFromRoot();
					}
					TextureLinkData.UnrealTexture = Texture;
				}
				return PlatformTexture;
			});

		// 4. Then we enqueue the copy to the render thread
		TFuture<FTouchTextureLinkJob> CopyOperation = CopyTexture_AnyThread(MoveTemp(TextureCreationOperation));
		
		// 5. Then we start the next one if there is one enqueued. //todo: when does this happen?
		CopyOperation.Next([WeakThis = TWeakPtr<FTouchTextureImporter>(SharedThis(this)), Promise = MoveTemp(Promise), FrameCooker](FTouchTextureLinkJob LinkJob) mutable
		{
			const TSharedPtr<FTouchTextureImporter> ThisPin = WeakThis.Pin();
			if (!ThisPin || ThisPin->TaskSuspender.IsSuspended())
			{
				Promise.SetValue(FTouchTextureImportResult::MakeCancelled());
				return;
			}
			
			ENQUEUE_RENDER_COMMAND(FinaliseTask)([ThisPin, LinkJob, FrameCooker](FRHICommandListImmediate& RHICmdList) mutable
			{
				TOptional<TPromise<FTouchTextureImportResult>> ExecuteNext;
				FTouchImportParameters ExecuteNextParams;
				FTouchTextureLinkData& TextureLinkData = ThisPin->LinkData.FindOrAdd(LinkJob.RequestParams.ParameterName);
				if (TextureLinkData.ExecuteNext.IsSet())
				{
					ExecuteNext = MoveTemp(TextureLinkData.ExecuteNext);
					TextureLinkData.ExecuteNext.Reset();
					ExecuteNextParams = TextureLinkData.ExecuteNextParams;
				}
				TextureLinkData.bIsInProgress = false;

				if (ExecuteNext.IsSet()) //todo: when does this happen?
				{
					ThisPin->ExecuteLinkTextureRequest_AnyThread(MoveTemp(*ExecuteNext), ExecuteNextParams, FrameCooker);
				}
			});
			const bool bFailure = LinkJob.ErrorCode != ETouchLinkErrorCode::Success && !ensure(LinkJob.UnrealTexture != nullptr);
			const FTouchTextureImportResult Result = bFailure ? FTouchTextureImportResult::MakeFailure() : FTouchTextureImportResult::MakeSuccessful(LinkJob.UnrealTexture);
			Promise.SetValue(Result);
		});
	}

	void FTouchTextureImporter::EnqueueLinkTextureRequest(FTouchTextureLinkData& TextureLinkData, TPromise<FTouchTextureImportResult>&& NewPromise, const FTouchImportParameters& LinkParams)
	{
		if (TextureLinkData.ExecuteNext.IsSet())
		{
			TextureLinkData.ExecuteNext->SetValue(FTouchTextureImportResult::MakeCancelled());
		}
		TextureLinkData.ExecuteNextParams = LinkParams;
		TextureLinkData.ExecuteNext = MoveTemp(NewPromise);
	}
	
	FTouchTextureLinkJob FTouchTextureImporter::CreateJob_AnyThread(const FTouchImportParameters& LinkParams)
	{
		const TSharedPtr<ITouchImportTexture> ImportTexture = CreatePlatformTexture_AnyThread(LinkParams.Instance, LinkParams.Texture);
		return FTouchTextureLinkJob { LinkParams, ImportTexture, nullptr, ImportTexture ? ETouchLinkErrorCode::Success : ETouchLinkErrorCode::FailedToCreatePlatformTexture };
	}

	TFuture<FTouchTextureLinkJob> FTouchTextureImporter::CopyTexture_AnyThread(TFuture<FTouchTextureLinkJob>&& ContinueFrom)
	{
		//todo: this could be simplified
		TPromise<FTouchTextureLinkJob> Promise;
		TFuture<FTouchTextureLinkJob> Result = Promise.GetFuture();
		ContinueFrom.Next([WeakThis = TWeakPtr<FTouchTextureImporter>(SharedThis(this)), Promise = MoveTemp(Promise)](FTouchTextureLinkJob IntermediateResult) mutable
		{
			const TSharedPtr<FTouchTextureImporter> ThisPin = WeakThis.Pin();
			if (!ensureMsgf(ThisPin, TEXT("Destruction should have been synchronized with SuspendAsyncTasks")) //todo: how does this work?
				|| ThisPin->TaskSuspender.IsSuspended()
				|| IntermediateResult.ErrorCode != ETouchLinkErrorCode::Success
				|| !ensureMsgf(IntermediateResult.PlatformTexture && IsValid(IntermediateResult.UnrealTexture), TEXT("Should be valid if no error code was set")))
			{
				Promise.SetValue(IntermediateResult);
				return;
			}
			
			ENQUEUE_RENDER_COMMAND(CopyTexture)([WeakThis, IntermediateResult](FRHICommandListImmediate& RHICmdList) mutable// , Promise = MoveTemp(Promise)
			{
				check(IntermediateResult.ErrorCode == ETouchLinkErrorCode::Success);
				
				const TSharedPtr<FTouchTextureImporter> ThisPin = WeakThis.Pin();
				if (!ensureMsgf(ThisPin, TEXT("Destruction should have been synchronized with SuspendAsyncTasks"))
					|| ThisPin->TaskSuspender.IsSuspended())
				{
					IntermediateResult.ErrorCode = ETouchLinkErrorCode::Cancelled;
					return;
				}

				const FTouchCopyTextureArgs CopyArgs { IntermediateResult.RequestParams, RHICmdList, IntermediateResult.UnrealTexture };
				IntermediateResult.PlatformTexture->CopyNativeToUnreal_RenderThread(CopyArgs)
					.Next([IntermediateResult](ECopyTouchToUnrealResult Result) mutable
					{
						const bool bSuccessfulCopy = Result == ECopyTouchToUnrealResult::Success;
						IntermediateResult.ErrorCode = bSuccessfulCopy
							? ETouchLinkErrorCode::Success
							: ETouchLinkErrorCode::FailedToCopyResources;
						if (bSuccessfulCopy)
						{
							UE_LOG(LogTouchEngine, Log, TEXT("   [CopyTexture_AnyThread] Successfully copied Texture to Unreal Engine for parameter [%s]"),*IntermediateResult.RequestParams.ParameterName.ToString())
						}
						else
						{
							UE_LOG(LogTouchEngine, Error, TEXT("   [CopyTexture_AnyThread] UNSUCCESSFULLY copied Texture to Unreal Engine for parameter [%s]"),*IntermediateResult.RequestParams.ParameterName.ToString())
						}
					});
			});
			Promise.SetValue(IntermediateResult); // we can enqueue the command and return directly at this point, as the texture is ready
		});
		return Result;
	}
}
