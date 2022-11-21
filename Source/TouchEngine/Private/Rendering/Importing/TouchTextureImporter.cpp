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
	
	TFuture<FTouchImportResult> FTouchTextureImporter::ImportTexture(const FTouchImportParameters& LinkParams)
	{
		if (TaskSuspender.IsSuspended())
		{
			UE_LOG(LogTouchEngine, Warning, TEXT("FTouchTextureLinker is suspended. Your task will be ignored."));
			return MakeFulfilledPromise<FTouchImportResult>(FTouchImportResult{ EImportResultType::Cancelled }).GetFuture();
		}

		FTaskSuspender::FTaskTracker TaskToken = TaskSuspender.StartTask();
		TPromise<FTouchImportResult> Promise;
		TFuture<FTouchImportResult> Future = Promise.GetFuture()
			.Next([TaskToken](auto Result)
			{
				return Result;
			});
		ENQUEUE_RENDER_COMMAND(ExecuteLinkTextureRequest)([WeakThis = TWeakPtr<FTouchTextureImporter>(SharedThis(this)), LinkParams, Promise = MoveTemp(Promise)](FRHICommandListImmediate& RHICmdList) mutable
		{
			const TSharedPtr<FTouchTextureImporter> ThisPin = WeakThis.Pin();
			if (!ThisPin || ThisPin->TaskSuspender.IsSuspended())
			{
				Promise.SetValue(FTouchImportResult{ EImportResultType::Cancelled });
				return;
			}

			FTouchTextureLinkData& TextureLinkData = ThisPin->LinkData.FindOrAdd(LinkParams.ParameterName);
			if (TextureLinkData.bIsInProgress)
			{
				ThisPin->EnqueueLinkTextureRequest(TextureLinkData, MoveTemp(Promise), LinkParams);
				return;
			}
			
			TextureLinkData.bIsInProgress = true;
			ThisPin->ExecuteLinkTextureRequest_RenderThread(RHICmdList, MoveTemp(Promise), LinkParams);
		});
		
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
					Data.Value.ExecuteNext->SetValue(FTouchImportResult{ EImportResultType::Cancelled });
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

	void FTouchTextureImporter::EnqueueLinkTextureRequest(FTouchTextureLinkData& TextureLinkData, TPromise<FTouchImportResult>&& NewPromise, const FTouchImportParameters& LinkParams)
	{
		if (TextureLinkData.ExecuteNext.IsSet())
		{
			TextureLinkData.ExecuteNext->SetValue(FTouchImportResult::MakeCancelled());
		}
		TextureLinkData.ExecuteNextParams = LinkParams;
		TextureLinkData.ExecuteNext = MoveTemp(NewPromise);
	}

	void FTouchTextureImporter::ExecuteLinkTextureRequest_RenderThread(FRHICommandListImmediate& RHICmdList, TPromise<FTouchImportResult>&& Promise, const FTouchImportParameters& LinkParams)
	{
		check(IsInRenderingThread());
		
		TFuture<FTouchTextureLinkJob> CreateJobOperation = CreateJob_RenderThread(RHICmdList, LinkParams);
		TFuture<FTouchTextureLinkJob> TextureCreationOperation = GetOrAllocateUnrealTexture_RenderThread(MoveTemp(CreateJobOperation));
		TFuture<FTouchTextureLinkJob> TextureCopyOperation = CopyTexture_RenderThread(MoveTemp(TextureCreationOperation));
		
		TextureCopyOperation.Next([WeakThis = TWeakPtr<FTouchTextureImporter>(SharedThis(this)), Promise = MoveTemp(Promise)](FTouchTextureLinkJob LinkJob) mutable
		{
			const TSharedPtr<FTouchTextureImporter> ThisPin = WeakThis.Pin();
			if (!ensureMsgf(ThisPin, TEXT("Destruction should have been synchronized with SuspendAsyncTasks"))
				|| ThisPin->TaskSuspender.IsSuspended())
			{
				Promise.SetValue(FTouchImportResult::MakeCancelled());
				return;
			}

			ENQUEUE_RENDER_COMMAND(FinaliseTask)([ThisPin, LinkJob, Promise = MoveTemp(Promise)](FRHICommandListImmediate& RHICmdList) mutable
			{
				TOptional<TPromise<FTouchImportResult>> ExecuteNext;
				FTouchImportParameters ExecuteNextParams;
				FTouchTextureLinkData& TextureLinkData = ThisPin->LinkData.FindOrAdd(LinkJob.RequestParams.ParameterName);
				if (TextureLinkData.ExecuteNext.IsSet())
				{
					ExecuteNext = MoveTemp(TextureLinkData.ExecuteNext);
					TextureLinkData.ExecuteNext.Reset();
					ExecuteNextParams = TextureLinkData.ExecuteNextParams;
				}
				TextureLinkData.bIsInProgress = false;

				const bool bFailure = LinkJob.ErrorCode != ETouchLinkErrorCode::Success && !ensure(LinkJob.UnrealTexture != nullptr);
				const FTouchImportResult Result = bFailure ? FTouchImportResult::MakeFailure() : FTouchImportResult::MakeSuccessful(LinkJob.UnrealTexture);
				Promise.SetValue(Result);

				if (ExecuteNext.IsSet())
				{
					ThisPin->ExecuteLinkTextureRequest_RenderThread(RHICmdList, MoveTemp(*ExecuteNext), ExecuteNextParams);
				}
			});
		});
	}

	TFuture<FTouchTextureLinkJob> FTouchTextureImporter::CreateJob_RenderThread(FRHICommandListImmediate& RHICmdList, const FTouchImportParameters& LinkParams)
	{
		TPromise<FTouchTextureLinkJob> Promise;
		TFuture<FTouchTextureLinkJob> Future = Promise.GetFuture();
		CreatePlatformTexture_RenderThread(RHICmdList, LinkParams.Instance, LinkParams.Texture)
			.Next([LinkParams, Promise = MoveTemp(Promise)](TSharedPtr<ITouchImportTexture> ImportTexture) mutable
			{
				Promise.SetValue(FTouchTextureLinkJob { LinkParams, ImportTexture, nullptr, ImportTexture ? ETouchLinkErrorCode::Success : ETouchLinkErrorCode::FailedToCreatePlatformTexture });
			});
		return Future;
	}

	TFuture<FTouchTextureLinkJob> FTouchTextureImporter::GetOrAllocateUnrealTexture_RenderThread(TFuture<FTouchTextureLinkJob>&& IntermediateResult)
	{
		TPromise<FTouchTextureLinkJob> Promise;
		TFuture<FTouchTextureLinkJob> Future = Promise.GetFuture();
		IntermediateResult.Next([WeakThis = TWeakPtr<FTouchTextureImporter>(SharedThis(this)), Promise = MoveTemp(Promise)](FTouchTextureLinkJob IntermediateResult) mutable
		{
			const TSharedPtr<FTouchTextureImporter> ThisPin = WeakThis.Pin();
			if (!ensureMsgf(ThisPin, TEXT("Destruction should have been synchronized with SuspendAsyncTasks")))
			{
				IntermediateResult.ErrorCode = ETouchLinkErrorCode::Cancelled;
				Promise.SetValue(IntermediateResult);
				return;
			}
			
			if (IntermediateResult.ErrorCode != ETouchLinkErrorCode::Success
				|| !ensureMsgf(IntermediateResult.PlatformTexture, TEXT("Operation wrongly marked successful")))
			{
				Promise.SetValue(IntermediateResult);
				return;
			}
			
			const FName ParameterName = IntermediateResult.RequestParams.ParameterName;
			// Common case: early out and continue operations current thread (should be render thread fyi)
			FTouchTextureLinkData& TextureLinkData = ThisPin->LinkData[ParameterName];
			if (TextureLinkData.UnrealTexture && IntermediateResult.PlatformTexture->CanCopyInto(TextureLinkData.UnrealTexture))
			{
				IntermediateResult.UnrealTexture = TextureLinkData.UnrealTexture;
				Promise.SetValue(IntermediateResult);
				return;
			}

			// Typically only happens once
			ExecuteOnGameThread<FTouchTextureLinkJob>([WeakThis = TWeakPtr<FTouchTextureImporter>(ThisPin), IntermediateResult]() mutable -> FTouchTextureLinkJob
			{
				const TSharedPtr<FTouchTextureImporter> ThisPin = WeakThis.Pin();
				if (!ensureMsgf(ThisPin, TEXT("Destruction should have been synchronized with SuspendAsyncTasks"))
					|| ThisPin->TaskSuspender.IsSuspended())
				{
					IntermediateResult.ErrorCode = ETouchLinkErrorCode::Cancelled;
					return IntermediateResult;
				}
				
				const FTextureMetaData TextureData = IntermediateResult.PlatformTexture->GetTextureMetaData();
				if (!ensure(TextureData.PixelFormat != PF_Unknown))
				{
					IntermediateResult.ErrorCode = ETouchLinkErrorCode::FailedToCreateUnrealTexture;
					return IntermediateResult;
				}

				if (UTexture2D* PreviousTexture = ThisPin->LinkData[IntermediateResult.RequestParams.ParameterName].UnrealTexture)
				{
					PreviousTexture->RemoveFromRoot();
				}
				
				UTexture2D* Texture = UTexture2D::CreateTransient(TextureData.SizeX, TextureData.SizeY, TextureData.PixelFormat);
				Texture->AddToRoot();
				Texture->UpdateResource();
				ThisPin->LinkData[IntermediateResult.RequestParams.ParameterName].UnrealTexture = Texture;
				IntermediateResult.UnrealTexture = Texture;
				
				return IntermediateResult;
			})
			.Next([Promise = MoveTemp(Promise)](FTouchTextureLinkJob IntermediateResult) mutable
			{
				Promise.SetValue(IntermediateResult);
			});
		});
		return Future;
	}

	TFuture<FTouchTextureLinkJob> FTouchTextureImporter::CopyTexture_RenderThread(TFuture<FTouchTextureLinkJob>&& ContinueFrom)
	{
		TPromise<FTouchTextureLinkJob> Promise;
		TFuture<FTouchTextureLinkJob> Result = Promise.GetFuture();
		ContinueFrom.Next([WeakThis = TWeakPtr<FTouchTextureImporter>(SharedThis(this)), Promise = MoveTemp(Promise)](FTouchTextureLinkJob IntermediateResult) mutable
		{
			const TSharedPtr<FTouchTextureImporter> ThisPin = WeakThis.Pin();
			if (!ensureMsgf(ThisPin, TEXT("Destruction should have been synchronized with SuspendAsyncTasks"))
				|| ThisPin->TaskSuspender.IsSuspended()
				|| IntermediateResult.ErrorCode != ETouchLinkErrorCode::Success
				|| !ensureMsgf(IntermediateResult.PlatformTexture && IsValid(IntermediateResult.UnrealTexture), TEXT("Should be valid if no error code was set")))
			{
				Promise.SetValue(IntermediateResult);
				return;
			}
			
			ENQUEUE_RENDER_COMMAND(CopyTexture)([WeakThis, Promise = MoveTemp(Promise), IntermediateResult](FRHICommandListImmediate& RHICmdList) mutable
			{
				check(IntermediateResult.ErrorCode == ETouchLinkErrorCode::Success);
				
				const TSharedPtr<FTouchTextureImporter> ThisPin = WeakThis.Pin();
				if (!ensureMsgf(ThisPin, TEXT("Destruction should have been synchronized with SuspendAsyncTasks"))
					|| ThisPin->TaskSuspender.IsSuspended())
				{
					IntermediateResult.ErrorCode = ETouchLinkErrorCode::Cancelled;
					Promise.SetValue(IntermediateResult);
					return;
				}

				const FTouchCopyTextureArgs CopyArgs { IntermediateResult.RequestParams, RHICmdList, IntermediateResult.UnrealTexture };
				IntermediateResult.PlatformTexture->CopyNativeToUnreal_RenderThread(CopyArgs)
					.Next([Promise = MoveTemp(Promise), IntermediateResult](ECopyTouchToUnrealResult Result) mutable
					{
						const bool bSuccessfulCopy = Result == ECopyTouchToUnrealResult::Success;
						IntermediateResult.ErrorCode = bSuccessfulCopy
							? ETouchLinkErrorCode::Success
							: ETouchLinkErrorCode::FailedToCopyResources;
						
						Promise.SetValue(IntermediateResult);
					});
			});
		});
		return Result;
	}
}
