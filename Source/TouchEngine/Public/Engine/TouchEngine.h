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

#include "Engine/TouchLoadResults.h"
#include "Engine/Util/TouchVariableManager.h"
#include "TouchEngineDynamicVariableStruct.h"
#include "TouchVariables.h"
#include "Util/TouchErrorLog.h"

#include "PixelFormat.h"

#include "TouchEngine/TouchObject.h"

class UTexture;
class UTexture2D;
class UTouchEngineInfo;
template <typename T> struct TTouchVar;
struct FTouchEngineDynamicVariableStruct;

DECLARE_MULTICAST_DELEGATE(FTouchOnLoadComplete);
DECLARE_MULTICAST_DELEGATE_OneParam(FTouchOnLoadFailed, const FString&);
DECLARE_MULTICAST_DELEGATE_TwoParams(FTouchOnParametersLoaded, const TArray<FTouchEngineDynamicVariableStruct>&, const TArray<FTouchEngineDynamicVariableStruct>&);
DECLARE_MULTICAST_DELEGATE(FTouchOnCookFinished);

namespace UE::TouchEngine
{
	class FTouchFrameCooker;
	class FTouchVariableManager;
	class FTouchResourceProvider;
	
	struct FCookFrameRequest;
	struct FCookFrameResult;

	/**
	 * An instance of this is passed as info object to callback functions from TE (e.g. TouchEventCallback_AnyThread).
	 * The calls are simply forwarded to FTouchEngineHazardPointer::TouchEngine if it has not yet been destroyed (since resource destruction is latent and may outlive ~FTouchEngine).
	 * A shared pointer to a FTouchEngineHazardPointer instance is kept fur the duration of the latent resource destruction.
	 */
	struct FTouchEngineHazardPointer : TSharedFromThis<FTouchEngineHazardPointer>
	{
		TWeakPtr<class FTouchEngine> TouchEngine;

		FTouchEngineHazardPointer(TSharedRef<class FTouchEngine> StrongThis)
			: TouchEngine(MoveTemp(StrongThis))
		{}
		
		static void TouchEventCallback_AnyThread(TEInstance* Instance, TEEvent Event, TEResult Result, int64_t StartTimeValue, int32_t StartTimeScale, int64_t EndTimeValue, int32_t EndTimeScale, void* Info);
		static void	LinkValueCallback_AnyThread(TEInstance* Instance, TELinkEvent Event, const char* Identifier, void* Info);
	};
	
	class TOUCHENGINE_API FTouchEngine : public TSharedFromThis<FTouchEngine>
	{
		friend class UTouchEngineInfo;
		friend FTouchEngineHazardPointer;
	public:

		~FTouchEngine();

		/** Starts a new TE instance or reuses the active one to load a .tox file. The future is executed on the game thread once the file has been loaded. */
		TFuture<FTouchLoadResult> LoadTox_GameThread(const FString& InToxPath);
		
		/** Unloads the .tox file. Calls TEInstanceUnload on the TE instance suspending it but keeping the process alive; you can call LoadTox to resume it. */
		void Unload_GameThread();
		/** Will end up calling TERelease on the instance. Kills the process. */
		void DestroyTouchEngine_GameThread();

		TFuture<FCookFrameResult> CookFrame_GameThread(const FCookFrameRequest& CookFrameRequest);
		void SetCookMode(bool bIsIndependent);
		bool SetFrameRate(int64 FrameRate);

		FTouchCHOPFull GetCHOPOutputSingleSample(const FString& Identifier) const	{ return LoadState_GameThread == ELoadState::Ready && ensure(TouchResources.VariableManager) ? TouchResources.VariableManager->GetCHOPOutputSingleSample(Identifier) : FTouchCHOPFull{}; }
		FTouchCHOPFull GetCHOPOutputs(const FString& Identifier) const				{ return LoadState_GameThread == ELoadState::Ready && ensure(TouchResources.VariableManager) ? TouchResources.VariableManager->GetCHOPOutputs(Identifier) : FTouchCHOPFull{}; }
		UTexture2D* GetTOPOutput(const FString& Identifier) const					{ return LoadState_GameThread == ELoadState::Ready && ensure(TouchResources.VariableManager) ? TouchResources.VariableManager->GetTOPOutput(Identifier) : nullptr; }
		TTouchVar<bool> GetBooleanOutput(const FString& Identifier) const			{ return LoadState_GameThread == ELoadState::Ready && ensure(TouchResources.VariableManager) ? TouchResources.VariableManager->GetBooleanOutput(Identifier) : TTouchVar<bool>{}; }
		TTouchVar<double> GetDoubleOutput(const FString& Identifier) const			{ return LoadState_GameThread == ELoadState::Ready && ensure(TouchResources.VariableManager) ? TouchResources.VariableManager->GetDoubleOutput(Identifier) : TTouchVar<double>{}; }
		TTouchVar<int32_t> GetIntegerOutput(const FString& Identifier) const		{ return LoadState_GameThread == ELoadState::Ready && ensure(TouchResources.VariableManager) ? TouchResources.VariableManager->GetIntegerOutput(Identifier) : TTouchVar<int32_t>{}; }
		TTouchVar<TEString*> GetStringOutput(const FString& Identifier) const		{ return LoadState_GameThread == ELoadState::Ready && ensure(TouchResources.VariableManager) ? TouchResources.VariableManager->GetStringOutput(Identifier) : TTouchVar<TEString*>{}; }
		FTouchDATFull GetTableOutput(const FString& Identifier) const				{ return LoadState_GameThread == ELoadState::Ready && ensure(TouchResources.VariableManager) ? TouchResources.VariableManager->GetTableOutput(Identifier) : FTouchDATFull{}; }
		TArray<FString> GetCHOPChannelNames(const FString& Identifier) const		{ return LoadState_GameThread == ELoadState::Ready && ensure(TouchResources.VariableManager) ? TouchResources.VariableManager->GetCHOPChannelNames(Identifier) : TArray<FString>{}; }

		void SetCHOPInputSingleSample(const FString& Identifier, const FTouchCHOPSingleSample& CHOP)		{ if (LoadState_GameThread == ELoadState::Ready && ensure(TouchResources.VariableManager)) { TouchResources.VariableManager->SetCHOPInputSingleSample(Identifier, CHOP); } }
		void SetCHOPInput(const FString& Identifier, const FTouchCHOPFull& CHOP)							{ if (LoadState_GameThread == ELoadState::Ready && ensure(TouchResources.VariableManager)) { TouchResources.VariableManager->SetCHOPInput(Identifier, CHOP); } }
		void SetTOPInput(const FString& Identifier, UTexture* Texture, bool bReuseExistingTexture = true)	{ if (LoadState_GameThread == ELoadState::Ready && ensure(TouchResources.VariableManager)) { TouchResources.VariableManager->SetTOPInput(Identifier, Texture, bReuseExistingTexture); } }
		void SetBooleanInput(const FString& Identifier, TTouchVar<bool>& Op)								{ if (LoadState_GameThread == ELoadState::Ready && ensure(TouchResources.VariableManager)) { TouchResources.VariableManager->SetBooleanInput(Identifier, Op); } }
		void SetDoubleInput(const FString& Identifier, TTouchVar<TArray<double>>& Op)						{ if (LoadState_GameThread == ELoadState::Ready && ensure(TouchResources.VariableManager)) { TouchResources.VariableManager->SetDoubleInput(Identifier, Op); } }
		void SetIntegerInput(const FString& Identifier, TTouchVar<TArray<int32_t>>& Op)						{ if (LoadState_GameThread == ELoadState::Ready && ensure(TouchResources.VariableManager)) { TouchResources.VariableManager->SetIntegerInput(Identifier, Op); } }
		void SetStringInput(const FString& Identifier, TTouchVar<const char*>& Op)							{ if (LoadState_GameThread == ELoadState::Ready && ensure(TouchResources.VariableManager)) { TouchResources.VariableManager->SetStringInput(Identifier, Op); } }
		void SetTableInput(const FString& Identifier, FTouchDATFull& Op)									{ if (LoadState_GameThread == ELoadState::Ready && ensure(TouchResources.VariableManager)) { TouchResources.VariableManager->SetTableInput(Identifier, Op); } }

		const FString& GetToxPath() const { return LastToxPathAttemptedToLoad; }
		bool HasCreatedTouchInstance() const { check(IsInGameThread()); return TouchResources.ResourceProvider.IsValid(); }
		bool IsLoading() const { check(IsInGameThread()); return LoadState_GameThread == ELoadState::Loading; }
		bool HasFailedToLoad() const { check(IsInGameThread()); return LoadState_GameThread == ELoadState::FailedToLoad; }
		bool IsReadyToCookFrame() const { check(IsInGameThread()); return LoadState_GameThread == ELoadState::Ready; }

		bool GetSupportedPixelFormat(TSet<TEnumAsByte<EPixelFormat>>& SupportedPixelFormat) const;

	private:

		struct FTouchResources
		{
			TouchObject<TEInstance> TouchEngineInstance = nullptr;

			TSharedPtr<FTouchEngineHazardPointer> HazardPointer;
			
			/** Helps print messages to the message log. Ideally would be a member of FTouchEngine but it is needed by other systems, like FTouchVariableManager. */
			TSharedPtr<FTouchErrorLog> ErrorLog;
		
			/** Created when the TE is spun up. */
			TSharedPtr<FTouchResourceProvider> ResourceProvider;
			/** Only valid when there is a valid TE running. */
			TSharedPtr<FTouchVariableManager> VariableManager;
			/** Handles cooking frames */
			TSharedPtr<FTouchFrameCooker> FrameCooker;

			void Reset()
			{
				TouchEngineInstance.reset();
				HazardPointer.Reset();
				FrameCooker.Reset();
				VariableManager.Reset();
				ResourceProvider.Reset();
				ErrorLog.Reset();
			}
		};

		FString FailureMessage;
		FString	LastToxPathAttemptedToLoad;
		
		enum class ELoadState
		{
			NoTouchInstance,
			Loading,
			FailedToLoad,
			Ready,
			Unloading,
			Unloaded
		};
		/** Can only be written to from the game thread. */
		ELoadState LoadState_GameThread = ELoadState::NoTouchInstance;

		FCriticalSection LoadPromiseMutex;
		/** Has a valid value while a load is active. */
		TOptional<TPromise<FTouchLoadResult>> LoadPromise;

		float TargetFrameRate = 60.f;
		TETimeMode TimeMode = TETimeInternal;

		/** Systems that are only valid while there is a Touch Engine (being) loaded. */
		FTouchResources TouchResources;
		
		TFuture<FTouchLoadResult> LoadTouchEngine(const FString& InToxPath);
		/** Create a touch engine instance, if none exists, and set up the engine with the tox path. This won't call TEInstanceLoad. */
		bool InstantiateEngineWithToxFile(const FString& InToxPath);

		// Handlers for loading tox
		void TouchEventCallback_AnyThread(TEInstance* Instance, TEEvent Event, TEResult Result, int64_t StartTimeValue, int32_t StartTimeScale, int64_t EndTimeValue, int32_t EndTimeScale);
		
		void OnInstancedLoaded_AnyThread(TEInstance* Instance, TEResult Result);
		void FinishLoadInstance_AnyThread(TEInstance* Instance);
		void OnLoadError_AnyThread(const FString& BaseErrorMessage = {}, TOptional<TEResult> Result = {});
		TPair<TEResult, TArray<FTouchEngineDynamicVariableStruct>> ProcessTouchVariables(TEInstance* Instance, TEScope Scope);

		void OnInstancedUnloaded_AnyThread();
		void ResumeLoadAfterUnload_GameThread();

		void LinkValue_AnyThread(TEInstance* Instance, TELinkEvent Event, const char* Identifier);
		void ProcessLinkTextureValueChanged_AnyThread(const char* Identifier);

		void SharedCleanUp();
		void CreateNewLoadPromise();
		TFuture<FTouchLoadResult> EmplaceFailedPromiseAndReturnFuture_GameThread(FString ErrorMessage);
		void EmplaceLoadPromiseIfSet_GameThread(FTouchLoadResult LoadResult);
		void DestroyResources(FString OldToxPath);

		bool OutputResultAndCheckForError_GameThread(const TEResult Result, const FString& ErrMessage) const;
	};

}
