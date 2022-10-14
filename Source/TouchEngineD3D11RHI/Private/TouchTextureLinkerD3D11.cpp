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

#include "TouchTextureLinkerD3D11.h"

#include "D3D11TouchUtils.h"
#include "Rendering/TouchPlatformTexture_AcquireOnRenderThread.h"

#include "D3D11RHIPrivate.h"

#include "TouchEngine/TED3D.h"
#include "TouchEngine/TED3D11.h"

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
		
		class FTouchPlatformTextureD3D11 : public FTouchPlatformTexture_AcquireOnRenderThread
		{
			using Super = FTouchPlatformTexture_AcquireOnRenderThread;
		public:

			FTouchPlatformTextureD3D11(FTexture2DRHIRef TextureRHI, TouchObject<TED3D11Texture> PlatformTexture)
				: Super(MoveTemp(TextureRHI))
				, PlatformTexture(MoveTemp(PlatformTexture))
			{}
		
		protected:
		
			virtual bool AcquireMutex(const FTouchCopyTextureArgs& CopyArgs, const TouchObject<TESemaphore>& Semaphore, uint64 WaitValue) override
			{
				ID3D11Texture2D* Resource = TED3D11TextureGetTexture(PlatformTexture);
				IDXGIKeyedMutex* Mutex = Private::GetMutex(Resource);
				if (!Mutex)
				{
					return false;
				}
		
				Mutex->AcquireSync(WaitValue, INFINITE);
				return true;
			}
		
			virtual void ReleaseMutex(const FTouchCopyTextureArgs& CopyArgs, const TouchObject<TESemaphore>& Semaphore, uint64 WaitValue) override
			{
				ID3D11Texture2D* Resource = TED3D11TextureGetTexture(PlatformTexture);
		
				// 1. The mutex must be released
				IDXGIKeyedMutex* Mutex = Private::GetMutex(Resource);
				const uint64 ReleaseValue = TEInstanceRequiresKeyedMutexReleaseToZero(CopyArgs.RequestParams.Instance)
					? 0
					: WaitValue + 1;
				Mutex->ReleaseSync(ReleaseValue);

				// 2. The mutex will get reacquired by TE, which will destroy the texture
				TEInstanceAddTextureTransfer(CopyArgs.RequestParams.Instance, PlatformTexture.get(), Semaphore, ReleaseValue);
			}

		private:

			TouchObject<TED3D11Texture> PlatformTexture;
		};
	}
	
	FTouchTextureLinkerD3D11::FTouchTextureLinkerD3D11(TouchObject<TED3D11Context> Context, ID3D11DeviceContext& DeviceContext)
		: Context(Context)
		, DeviceContext(&DeviceContext)
	{}

	TSharedPtr<ITouchPlatformTexture> FTouchTextureLinkerD3D11::CreatePlatformTexture(const TouchObject<TEInstance>& Instance, const TouchObject<TETexture>& SharedTexture)
	{
		const TouchObject<TED3D11Texture> PlatformTexture = CreatePlatformTexture(SharedTexture);
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
		const FTexture2DRHIRef SrcRHI = DynamicRHI->RHICreateTexture2DFromResource(Format, TexCreate_Shared, FClearValueBinding::None, SourceD3D11Texture2D).GetReference();

		// TODO DP: Possibly refactor to not return a future but just return the value
		return MakeShared<Private::FTouchPlatformTextureD3D11>(SrcRHI, PlatformTexture);
	}
	
	TouchObject<TED3D11Texture> FTouchTextureLinkerD3D11::CreatePlatformTexture(const TouchObject<TETexture>& SharedTexture) const
	{
		TED3DSharedTexture* SharedSource = static_cast<TED3DSharedTexture*>(SharedTexture.get());
		TouchObject<TED3D11Texture> SourceTextureResult = nullptr;
		TED3D11ContextCreateTexture(Context, SharedSource, SourceTextureResult.take());
		return SourceTextureResult;
	}
}
