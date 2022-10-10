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

#include "D3D11RHIPrivate.h"
#include "dxgi.h"
#include "Engine/Texture2D.h"

#include "TouchEngine/TED3D.h"
#include "TouchEngine/TED3D11.h"

namespace UE::TouchEngine::D3DX11
{
	namespace Private
	{
		IDXGIKeyedMutex* GetMutex(TouchObject<TETexture> PlatformTexture)
		{
			TED3D11Texture* TextureD3D11 = static_cast<TED3D11Texture*>(PlatformTexture.get());
			ID3D11Texture2D* Texture2D = TED3D11TextureGetTexture(TextureD3D11);
			IDXGIKeyedMutex* Mutex;
			Texture2D->QueryInterface(_uuidof(IDXGIKeyedMutex), (void**)&Mutex);
			return Mutex;
		}
	}
	
	FTouchTextureLinkerD3D11::FTouchTextureLinkerD3D11(TouchObject<TED3D11Context> Context, ID3D11DeviceContext& DeviceContext)
		: Context(Context)
		, DeviceContext(&DeviceContext)
	{}

	int32 FTouchTextureLinkerD3D11::GetPlatformTextureWidth(TETexture* Texture) const
	{
		return GetDescriptor(Texture).Width;
	}

	int32 FTouchTextureLinkerD3D11::GetPlatformTextureHeight(TETexture* Texture) const
	{
		return GetDescriptor(Texture).Height;
	}

	EPixelFormat FTouchTextureLinkerD3D11::GetPlatformTexturePixelFormat(TETexture* Texture) const
	{
		return ConvertD3FormatToPixelFormat(GetDescriptor(Texture).Format);
	}

	bool FTouchTextureLinkerD3D11::CopyNativeResources(TETexture* SourcePlatformTexture, UTexture2D* Target) const
	{
		const TED3D11Texture* SourceTextureResult = static_cast<TED3D11Texture*>(SourcePlatformTexture);
		
		const FD3D11TextureBase* TargetD3D11Texture = GetD3D11TextureFromRHITexture(Target->GetResource()->TextureRHI);
		ID3D11Resource* TargetResource = TargetD3D11Texture->GetResource();
		ID3D11Texture2D* SourceD3D11Texture = TED3D11TextureGetTexture(SourceTextureResult);
		DeviceContext->CopyResource(TargetResource, SourceD3D11Texture);
		return true;
	}

	TMutexLifecyclePtr<TouchObject<TETexture>> FTouchTextureLinkerD3D11::CreatePlatformTextureWithMutex(const TouchObject<TEInstance>& Instance, const TouchObject<TESemaphore>& Semaphore, uint64 WaitValue, const TouchObject<TETexture>& SharedTexture) const
	{
		TouchObject<TETexture> PlatformTexture = CreatePlatformTexture(SharedTexture);
		
		IDXGIKeyedMutex* Mutex = Private::GetMutex(PlatformTexture);
		Mutex->AcquireSync(WaitValue, INFINITE);
			
		TSharedRef<TouchObject<TETexture>> Result = MakeShareable<TouchObject<TETexture>>(new TouchObject<TETexture>(PlatformTexture), [WaitValue, Instance, Semaphore](TouchObject<TETexture>* Texture)
		{
			// 1. The mute must be released
			IDXGIKeyedMutex* Mutex = Private::GetMutex(*Texture);
			const uint64 ReleaseValue = TEInstanceRequiresKeyedMutexReleaseToZero(Instance)
				? 0
				: WaitValue + 1;
			Mutex->ReleaseSync(ReleaseValue);

			// 2. The mutex will get reacquired by TE, which will destroy the texture
			TEInstanceAddTextureTransfer(Instance, Texture->get(), Semaphore, ReleaseValue);

			// Remember: custom deleter means we'll manage the memory allocations ourselves so we must delete too!
			delete Texture;
		});
		return Result;
	}

	TouchObject<TETexture> FTouchTextureLinkerD3D11::CreatePlatformTexture(const TouchObject<TETexture>& SharedTexture) const
	{
		TED3DSharedTexture* SharedSource = static_cast<TED3DSharedTexture*>(SharedTexture.get());
		TouchObject<TED3D11Texture> SourceTextureResult = nullptr;
		TED3D11ContextCreateTexture(Context, SharedSource, SourceTextureResult.take());
		return SourceTextureResult;
	}

	D3D11_TEXTURE2D_DESC FTouchTextureLinkerD3D11::GetDescriptor(TETexture* Texture) const
	{
		const TED3D11Texture* TextureD3D11 = static_cast<TED3D11Texture*>(Texture);
		ID3D11Texture2D* SourceD3D11Texture2D = TED3D11TextureGetTexture(TextureD3D11);
		D3D11_TEXTURE2D_DESC Desc = { 0 };
		SourceD3D11Texture2D->GetDesc(&Desc);
		
		return Desc;
	}
}
