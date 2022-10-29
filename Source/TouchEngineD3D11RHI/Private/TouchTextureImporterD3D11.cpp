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

#include "TouchTextureImporterD3D11.h"

#include "D3D11TouchUtils.h"
#include "Rendering/Importing/TouchImportTexture_AcquireOnRenderThread.h"

#include "D3D11RHIPrivate.h"

#include "TouchEngine/TED3D.h"
#include "TouchEngine/TED3D11.h"

#include "d3d11.h"

namespace UE::TouchEngine::D3DX11
{
	namespace Private
	{
		static IDXGIKeyedMutex* GetMutex(ID3D11Texture2D* Texture2D)
		{
			IDXGIKeyedMutex* Mutex;
			Texture2D->QueryInterface(_uuidof(IDXGIKeyedMutex), (void**)&Mutex);
			return Mutex;
		}
		
		class FTouchPlatformTextureD3D11 : public FTouchImportTexture_AcquireOnRenderThread
		{
			using Super = FTouchImportTexture_AcquireOnRenderThread;
		public:

			FTouchPlatformTextureD3D11(TouchObject<TED3D11Context> Context, TouchObject<TETexture> OutputTexture)
				: Context(MoveTemp(Context))
				, OutputTexture(MoveTemp(OutputTexture))
			{}

			virtual FTextureMetaData GetTextureMetaData() const override
			{
				const TED3DSharedTexture* SharedSource = static_cast<TED3DSharedTexture*>(OutputTexture.get());
				const uint32 SizeX = TED3DSharedTextureGetWidth(SharedSource);
				const uint32 SizeY = TED3DSharedTextureGetHeight(SharedSource);
				const DXGI_FORMAT Format = TED3DSharedTextureGetFormat(SharedSource);
				return { SizeX, SizeY, ConvertD3FormatToPixelFormat(Format) };
			}

		protected:
		
			virtual bool AcquireMutex(const FTouchCopyTextureArgs& CopyArgs, const TouchObject<TESemaphore>& Semaphore, uint64 WaitValue) override
			{
				check(!PlatformTexture);

				// TODO DP: It feels a bit weird to call TED3D11ContextCreateTexture just to destroy the result just a moment after.
				// The TE API does not seem to provide a direct way to obtain ID3D11Texture2D resource without a call to TED3D11ContextCreateTexture
				TED3DSharedTexture* SharedSource = static_cast<TED3DSharedTexture*>(OutputTexture.get());
				TED3D11ContextCreateTexture(Context, SharedSource, PlatformTexture.take());
				
				ID3D11Texture2D* Resource = TED3D11TextureGetTexture(PlatformTexture);
				IDXGIKeyedMutex* Mutex = Private::GetMutex(Resource);
				if (!Mutex)
				{
					return false;
				}
		
				Mutex->AcquireSync(WaitValue, INFINITE);
				return true;
			}

			virtual FTexture2DRHIRef ReadTextureDuringMutex() override
			{
				if (!PlatformTexture)
				{
					return nullptr;
				}
		
				ID3D11Texture2D* SourceD3D11Texture2D = TED3D11TextureGetTexture(PlatformTexture);
				check(SourceD3D11Texture2D);

				D3D11_TEXTURE2D_DESC Desc = { 0 };
				SourceD3D11Texture2D->GetDesc(&Desc);
				const EPixelFormat Format = ConvertD3FormatToPixelFormat(Desc.Format);
		
				FD3D11DynamicRHI* DynamicRHI = static_cast<FD3D11DynamicRHI*>(GDynamicRHI);
				return DynamicRHI->RHICreateTexture2DFromResource(Format, TexCreate_Shared, FClearValueBinding::None, SourceD3D11Texture2D).GetReference();
			}

			virtual void ReleaseMutex(const FTouchCopyTextureArgs& CopyArgs, const TouchObject<TESemaphore>& Semaphore, uint64 WaitValue) override
			{
				// RHI and raw D3D11 mutex does not play nice together when r.RHICmdByPass is turned off. Hence,
				// we need to flush commands to ensure the texture copy is completed before releasing mutex.
				CopyArgs.RHICmdList.SubmitCommandsAndFlushGPU();

				ID3D11Texture2D* Resource = TED3D11TextureGetTexture(PlatformTexture);
		
				// 1. The mutex must be released
				IDXGIKeyedMutex* Mutex = Private::GetMutex(Resource);
				const uint64 ReleaseValue = TEInstanceRequiresKeyedMutexReleaseToZero(CopyArgs.RequestParams.Instance)
					? 0
					: WaitValue + 1;
				Mutex->ReleaseSync(ReleaseValue);

				// 2. The mutex will get reacquired by TE, which will destroy the texture
				TEInstanceAddTextureTransfer(CopyArgs.RequestParams.Instance, PlatformTexture.get(), Semaphore, ReleaseValue);
				
				PlatformTexture.reset();
			}

		private:
			
			TouchObject<TED3D11Context> Context;
			TouchObject<TETexture> OutputTexture;
			TouchObject<TED3D11Texture> PlatformTexture;
		};
	}
	
	FTouchTextureImporterD3D11::FTouchTextureImporterD3D11(TouchObject<TED3D11Context> Context, ID3D11DeviceContext& DeviceContext)
		: Context(Context)
		, DeviceContext(&DeviceContext)
	{}

	TFuture<TSharedPtr<ITouchImportTexture>> FTouchTextureImporterD3D11::CreatePlatformTexture(const TouchObject<TEInstance>& Instance, const TouchObject<TETexture>& OutputTexture)
	{
		const TSharedPtr<Private::FTouchPlatformTextureD3D11> Texture = MakeShared<Private::FTouchPlatformTextureD3D11>(Context, OutputTexture);
		return MakeFulfilledPromise<TSharedPtr<ITouchImportTexture>>(StaticCastSharedPtr<ITouchImportTexture>(Texture)).GetFuture();
	}
}