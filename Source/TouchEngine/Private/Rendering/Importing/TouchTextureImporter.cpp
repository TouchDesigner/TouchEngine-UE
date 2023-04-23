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
	
	TFuture<FTouchTextureImportResult> FTouchTextureImporter::ImportTexture(const FTouchImportParameters& LinkParams)
	{
		if (TaskSuspender.IsSuspended())
		{
			UE_LOG(LogTouchEngine, Warning, TEXT("FTouchTextureLinker is suspended. Your task will be ignored."));
			return MakeFulfilledPromise<FTouchTextureImportResult>(FTouchTextureImportResult{ EImportResultType::Cancelled }).GetFuture();
		}

		//todo: is there a way to figure out the texture metadata at this point? this would allow us to create a UTexture in advance

		// Here we just want to start the texture transfer, but this will be processed later on in the render thread
		LinkParams.GetTextureTransferResult = TEInstanceGetTextureTransfer(LinkParams.Instance, LinkParams.Texture,
			LinkParams.GetTextureTransferSemaphore.take(), &LinkParams.GetTextureTransferWaitValue);
		
		FTaskSuspender::FTaskTracker TaskToken = TaskSuspender.StartTask();
		TPromise<FTouchTextureImportResult> Promise;
		TFuture<FTouchTextureImportResult> Future = Promise.GetFuture()
			.Next([TaskToken](auto Result)
			{
				return Result;
			});
		ENQUEUE_RENDER_COMMAND(ExecuteLinkTextureRequest)([WeakThis = TWeakPtr<FTouchTextureImporter>(SharedThis(this)), LinkParams, Promise = MoveTemp(Promise)](FRHICommandListImmediate& RHICmdList) mutable
		{
			const TSharedPtr<FTouchTextureImporter> ThisPin = WeakThis.Pin();
			if (!ThisPin || ThisPin->TaskSuspender.IsSuspended())
			{
				Promise.SetValue(FTouchTextureImportResult{ EImportResultType::Cancelled });
				TERelease(&LinkParams.GetTextureTransferSemaphore);
				return;
			}

			FTouchTextureLinkData& TextureLinkData = ThisPin->LinkData.FindOrAdd(LinkParams.ParameterName);
			if (TextureLinkData.bIsInProgress)
			{
				ThisPin->EnqueueLinkTextureRequest(TextureLinkData, MoveTemp(Promise),  LinkParams);
				return;
			}
			
			TextureLinkData.bIsInProgress = true;
			ThisPin->ExecuteLinkTextureRequest_RenderThread(RHICmdList, MoveTemp(Promise),  LinkParams);
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

	void FTouchTextureImporter::EnqueueLinkTextureRequest(FTouchTextureLinkData& TextureLinkData, TPromise<FTouchTextureImportResult>&& NewPromise, const FTouchImportParameters& LinkParams)
	{
		if (TextureLinkData.ExecuteNext.IsSet())
		{
			if (TextureLinkData.ExecuteNextParams.PendingTextureImportType) //we expect that we haven't started so the promise has not been set
			{
				TextureLinkData.ExecuteNextParams.PendingTextureImportType->SetValue({EPendingTextureImportType::Cancelled});
			}
			TextureLinkData.ExecuteNext->SetValue(FTouchTextureImportResult::MakeCancelled());
		}
		TextureLinkData.ExecuteNextParams = LinkParams;
		TextureLinkData.ExecuteNext = MoveTemp(NewPromise);
	}

	void FTouchTextureImporter::ExecuteLinkTextureRequest_RenderThread(FRHICommandListImmediate& RHICmdList, TPromise<FTouchTextureImportResult>&& Promise, const FTouchImportParameters& LinkParams)
	{
		check(IsInRenderingThread());
		
		FTouchTextureLinkJob PlatformTexture = CreateJob_RenderThread(RHICmdList, LinkParams);
		// EPendingTextureImportType TextureImportType;
		// TFuture<FTouchTextureLinkJob> TextureCreationOperation = GetOrAllocateUnrealTexture_RenderThread(MoveTemp(PlatformTexture), TextureImportType);
		// FTextureFormat TextureFormat {TextureImportType};
		// TPromise<UTexture2D*> TextureCreatedPromise;

		FTextureFormat TextureFormat;
		TextureFormat.Identifier = PlatformTexture.RequestParams.ParameterName;
		TextureFormat.OnTextureCreated = MakeShared<TPromise<UTexture2D*>>();
		// TFuture<UTexture2D*> TextureCreatedFuture = TextureFormat.OnTextureCreated->GetFuture();
		
		if (PlatformTexture.ErrorCode != ETouchLinkErrorCode::Success || !ensureMsgf(PlatformTexture.PlatformTexture, TEXT("Operation wrongly marked successful")))
		{
			TextureFormat.PendingTextureImportType = EPendingTextureImportType::Failure;
			// Promise.SetValue(IntermediateResult);
			// return Future;
		}
		else
		{
			const FName ParameterName = TextureFormat.Identifier;
			// Common case: early out and continue operations current thread (should be render thread fyi)
			FTouchTextureLinkData& TextureLinkData = LinkData[ParameterName];
			if (TextureLinkData.UnrealTexture && PlatformTexture.PlatformTexture->CanCopyInto(TextureLinkData.UnrealTexture))
			{
				TextureFormat.PendingTextureImportType = EPendingTextureImportType::CanBeImported;
				PlatformTexture.UnrealTexture = TextureLinkData.UnrealTexture;
				TextureFormat.UnrealTexture = TextureLinkData.UnrealTexture;
				// Promise.SetValue(IntermediateResult);
				// return Future;
			}
			else
			{
				const FTextureMetaData TextureData = PlatformTexture.PlatformTexture->GetTextureMetaData();
				if (!ensure(TextureData.PixelFormat != PF_Unknown))
				{
					PlatformTexture.ErrorCode = ETouchLinkErrorCode::FailedToCreateUnrealTexture;
					TextureFormat.PendingTextureImportType = EPendingTextureImportType::Failure;
					// return IntermediateResult;
				}
				else
				{
					TextureFormat.PendingTextureImportType = EPendingTextureImportType::NeedUTextureCreated;
					if (UTexture2D* PreviousTexture = LinkData[ParameterName].UnrealTexture)
					{
						PreviousTexture->RemoveFromRoot(); //todo: should this move to game thread?
					}
					TextureFormat.SizeX = TextureData.SizeX;
					TextureFormat.SizeY = TextureData.SizeY; 
					TextureFormat.PixelFormat= TextureData.PixelFormat;
				}
			}
		}
		LinkParams.PendingTextureImportType->SetValue(TextureFormat); // let the caller know if we should wait for this texture or not


		TFuture<FTouchTextureLinkJob> TextureCreationOperation = TextureFormat.OnTextureCreated->GetFuture().Next(
			[WeakThis = TWeakPtr<FTouchTextureImporter>(SharedThis(this)), IntermediateResult = MoveTemp(PlatformTexture)](UTexture2D* Texture) mutable
			{
				IntermediateResult.UnrealTexture = Texture;
				if (const TSharedPtr<FTouchTextureImporter> ThisPin = WeakThis.Pin())
				{
					ThisPin->LinkData[IntermediateResult.RequestParams.ParameterName].UnrealTexture = Texture;
				}
				return IntermediateResult;
			});
		
		TFuture<FTouchTextureLinkJob> TextureCopyOperation = CopyTexture_RenderThread(MoveTemp(TextureCreationOperation));
		
		TextureCopyOperation.Next([WeakThis = TWeakPtr<FTouchTextureImporter>(SharedThis(this)), Promise = MoveTemp(Promise)](FTouchTextureLinkJob LinkJob) mutable
		{
			const TSharedPtr<FTouchTextureImporter> ThisPin = WeakThis.Pin();
			if (!ensureMsgf(ThisPin, TEXT("Destruction should have been synchronized with SuspendAsyncTasks"))
				|| ThisPin->TaskSuspender.IsSuspended())
			{
				Promise.SetValue(FTouchTextureImportResult::MakeCancelled());
				return;
			}

			ENQUEUE_RENDER_COMMAND(FinaliseTask)([ThisPin, LinkJob, Promise = MoveTemp(Promise)](FRHICommandListImmediate& RHICmdList) mutable
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

				const bool bFailure = LinkJob.ErrorCode != ETouchLinkErrorCode::Success && !ensure(LinkJob.UnrealTexture != nullptr);
				const FTouchTextureImportResult Result = bFailure ? FTouchTextureImportResult::MakeFailure() : FTouchTextureImportResult::MakeSuccessful(LinkJob.UnrealTexture);
				Promise.SetValue(Result);

				if (ExecuteNext.IsSet())
				{
					ThisPin->ExecuteLinkTextureRequest_RenderThread(RHICmdList, MoveTemp(*ExecuteNext), ExecuteNextParams);
				}
			});
		});
	}

	FTouchTextureLinkJob FTouchTextureImporter::CreateJob_RenderThread(FRHICommandListImmediate& RHICmdList, const FTouchImportParameters& LinkParams)
	{
		const TSharedPtr<ITouchImportTexture> ImportTexture = CreatePlatformTexture_RenderThread(RHICmdList, LinkParams.Instance, LinkParams.Texture);
		return FTouchTextureLinkJob { LinkParams, ImportTexture, nullptr, ImportTexture ? ETouchLinkErrorCode::Success : ETouchLinkErrorCode::FailedToCreatePlatformTexture };
	}

	TFuture<FTouchTextureLinkJob> FTouchTextureImporter::GetOrAllocateUnrealTexture_RenderThread(FTouchTextureLinkJob&& IntermediateResult, EPendingTextureImportType& TextureImportType)
	{
		TPromise<FTouchTextureLinkJob> Promise;
		TFuture<FTouchTextureLinkJob> Future = Promise.GetFuture();
		
		if (IntermediateResult.ErrorCode != ETouchLinkErrorCode::Success
			|| !ensureMsgf(IntermediateResult.PlatformTexture, TEXT("Operation wrongly marked successful")))
		{
			TextureImportType = EPendingTextureImportType::Failure;
			Promise.SetValue(IntermediateResult);
			return Future;
		}
		
		const FName ParameterName = IntermediateResult.RequestParams.ParameterName;
		// Common case: early out and continue operations current thread (should be render thread fyi)
		FTouchTextureLinkData& TextureLinkData = LinkData[ParameterName];
		if (TextureLinkData.UnrealTexture && IntermediateResult.PlatformTexture->CanCopyInto(TextureLinkData.UnrealTexture))
		{
			TextureImportType = EPendingTextureImportType::CanBeImported;
			IntermediateResult.UnrealTexture = TextureLinkData.UnrealTexture;
			Promise.SetValue(IntermediateResult);
			return Future;
		}

		// Typically only happens once //todo: deadlock here. We are waiting for the task to end on the GameThread to continue, but here we are trying to do something on the GameThread, which is waiting for this to finish
		ExecuteOnGameThread<FTouchTextureLinkJob>([WeakThis = TWeakPtr<FTouchTextureImporter>(SharedThis(this)), IntermediateResult]() mutable -> FTouchTextureLinkJob
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

			UTexture2D* Texture = UTexture2D::CreateTransient(TextureData.SizeX, TextureData.SizeY, TextureData.PixelFormat, IntermediateResult.RequestParams.ParameterName);
			Texture->AddToRoot();
			Texture->UpdateResource(); // this needs to be on Game Thread
			ThisPin->LinkData[IntermediateResult.RequestParams.ParameterName].UnrealTexture = Texture;
			IntermediateResult.UnrealTexture = Texture;
			UE_LOG(LogTouchEngine, Display, TEXT("[GetOrAllocateUnrealTexture] Created UTexture `%s` (%dx%d `%s`) for identifier `%s`"),
				*Texture->GetName(), TextureData.SizeX, TextureData.SizeY, GPixelFormats[TextureData.PixelFormat].Name,
				*IntermediateResult.RequestParams.ParameterName.ToString())
			
			return IntermediateResult;
		})
		.Next([Promise = MoveTemp(Promise)](FTouchTextureLinkJob IntermediateResult) mutable
		{
			Promise.SetValue(IntermediateResult);
		});
		// });
		TextureImportType = EPendingTextureImportType::NeedUTextureCreated;
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
			
			ENQUEUE_RENDER_COMMAND(CopyTexture)([WeakThis, IntermediateResult, Promise = MoveTemp(Promise)](FRHICommandListImmediate& RHICmdList) mutable
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
					.Next([IntermediateResult, Promise = MoveTemp(Promise)](ECopyTouchToUnrealResult Result) mutable
					{
						const bool bSuccessfulCopy = Result == ECopyTouchToUnrealResult::Success;
						IntermediateResult.ErrorCode = bSuccessfulCopy
							? ETouchLinkErrorCode::Success
							: ETouchLinkErrorCode::FailedToCopyResources;
						UE_LOG(LogTouchEngine, Warning, TEXT("   [CopyTexture_RenderThread] %s copied Texture to Unreal Engine for parameter [%s]"),
							bSuccessfulCopy ? TEXT("Successfully") : TEXT("UNSUCCESSFULLY"),
							*IntermediateResult.RequestParams.ParameterName.ToString())
						Promise.SetValue(IntermediateResult);
					});
			});
			// Promise.SetValue(IntermediateResult); // we can enqueue the command and return directly at this point, as the texture is ready
		});
		return Result;
	}
}
