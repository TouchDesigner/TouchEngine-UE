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
#include "TouchEngine/TEInstance.h"
#include "TouchEngine/TEResult.h"
// #include "TouchEngine/TEVulkan.h"
#include "TouchEngine/TouchObject.h"
#include "Util/TouchHelpers.h"

DEFINE_LOG_CATEGORY_STATIC(LogTouchEngineCalls, Log, All)

inline FString TEEventToString(const TEEvent Event)
{
	switch (Event)
	{
	case TEEventGeneral: return FString(TEXT("TEEventGeneral"));
	case TEEventInstanceReady: return FString(TEXT("TEEventInstanceReady"));
	case TEEventInstanceDidLoad: return FString(TEXT("TEEventInstanceDidLoad"));
	case TEEventInstanceDidUnload: return FString(TEXT("TEEventInstanceDidUnload"));
	case TEEventFrameDidFinish: return FString(TEXT("TEEventFrameDidFinish"));
	default: return FString(TEXT("default"));
	}
}
inline FString TELinkEventToString(const TELinkEvent Event)
{
	switch (Event)
	{
	case TELinkEventAdded: return FString(TEXT("TELinkEventAdded"));
	case TELinkEventRemoved: return FString(TEXT("TELinkEventRemoved"));
	case TELinkEventModified: return FString(TEXT("TELinkEventModified"));
	case TELinkEventMoved: return FString(TEXT("TELinkEventMoved"));
	case TELinkEventStateChange: return FString(TEXT("TELinkEventStateChange"));
	case TELinkEventChildChange: return FString(TEXT("TELinkEventChildChange"));
	case TELinkEventValueChange: return FString(TEXT("TELinkEventValueChange"));
	default: return FString(TEXT("default"));
	}
}
inline FString TEObjectEventToString(const TEObjectEvent Event)
{
	switch (Event)
	{
	case TEObjectEventBeginUse: return FString(TEXT("TEObjectEventBeginUse"));
	case TEObjectEventEndUse: return FString(TEXT("TEObjectEventEndUse"));
	case TEObjectEventRelease: return FString(TEXT("TEObjectEventRelease"));
	default: return FString(TEXT("default"));
	}
}

/*
 Provide the instance with a semaphore to synchronize texture usage by the instance. Note that the texture may not be
	 used, in which case the semaphore will not be used.
 'texture' is the texture to synchronize usage of
 'semaphore' is a TESemaphore to synchronize usage
	 The instance will wait for this semaphore prior to using the texture
	 If this semaphore is a Vulkan binary semaphore, the instance will also signal this semaphore after waiting
	 To synchronize a D3D11 texture with a DXGI Keyed Mutex, pass NULL for this value
 'waitValue' is, if appropriate for the semaphore type, a value for the semaphore wait operation
 */
inline TEResult TEInstanceAddTextureTransfer_Debug(TEInstance *instance, TETexture *texture, TESemaphore * TE_NULLABLE semaphore, uint64_t value)
{
	const TEResult Result = TEInstanceAddTextureTransfer(instance, texture, semaphore, value);
	UE_LOG(LogTouchEngineCalls, Verbose, TEXT(" Called TEInstanceAddTextureTransfer on thread [%s] with waitvalue `%llu`.%s%s%s   => Returned `%hs`"),
		*UE::TouchEngine::GetCurrentThreadStr(),
		value,
		instance ? TEXT("") : TEXT(" `instance` is null!"),
		texture ? TEXT("") : TEXT(" `texture` is null!"),
		semaphore ? TEXT("") : TEXT(" `semaphore` is null!"),
		TEResultGetDescription(Result));
	return Result;
}


/*
 Returns true if 'instance' has a pending texture transfer for 'texture'
 */
inline bool TEInstanceHasTextureTransfer_Debug(TEInstance* instance, const TETexture* texture)
{
	const bool Result = TEInstanceHasTextureTransfer(instance, texture);
	UE_LOG(LogTouchEngineCalls, Verbose, TEXT(" Called TEInstanceHasTextureTransfer on thread [%s].%s%s   => Returned `%s`"),
		*UE::TouchEngine::GetCurrentThreadStr(),
		instance ? TEXT("") : TEXT(" `instance` is null!"),
		texture ? TEXT("") : TEXT(" `texture` is null!"),
		Result ? TEXT("TRUE") : TEXT("FALSE"));
	return Result;
}

/*
 Get the semaphore needed to transfer ownership from the instance prior to using a texture, if
 such an operation is pending.
 'texture' is a texture associated with one of the instance's links
 'semaphore' is, on successful return, a TESemaphore to synchronize the transfer on the GPU
	 The caller must wait for this semaphore prior to using the texture
	 If this semaphore is a Vulkan binary semaphore, the caller must also signal this semaphore after waiting
	 If the TETexture is a D3D11 texture and this value is NULL, use a DXGI Keyed Mutex acquire operation on the
	 instantiated texture
	The caller is responsible for releasing the returned TESemaphore using TERelease()
 'waitValue' is, on successful return and if appropriate for the semaphore type, a value for the semaphore wait operation
 */
inline TEResult TEInstanceGetTextureTransfer_Debug(TEInstance *instance,
												const TETexture *texture,
												TESemaphore * TE_NULLABLE * TE_NONNULL semaphore,
												uint64_t *waitValue)
{
	const TEResult Result = TEInstanceGetTextureTransfer(instance, texture, semaphore, waitValue);
	UE_LOG(LogTouchEngineCalls, Verbose, TEXT(" Called TEInstanceGetTextureTransfer on thread [%s] with waitvalue `%llu`%s.%s%s%s   => Returned `%hs`"),
		*UE::TouchEngine::GetCurrentThreadStr(),
		waitValue ? *waitValue : -1,
		waitValue ? TEXT("") : TEXT("(`waitValue` is null!)"),
		instance ? TEXT("") : TEXT(" `instance` is null!"),
		texture ? TEXT("") : TEXT(" `texture` is null!"),
		semaphore ? TEXT("") : TEXT(" `semaphore` is null!"),
		TEResultGetDescription(Result));
	return Result;
}

/*
 Sets the value of a texture input link
 'texture' may be retained by the instance
 'context' is a valid TEGraphicsContext of a type suitable for working with the provided texture.
	NULL may be passed ONLY if 'texture' is of type TETextureTypeD3DShared or TETextureTypeIOSurface or TETextureTypeVulkan
	Work may be done in the provided graphics context by this call.
	 An OpenGL context may change the current framebuffer and GL_TEXTURE_2D bindings during this call.
	This may be a different context than any previously passed to TEInstanceAssociateGraphicsContext().
 */
inline TEResult TEInstanceLinkSetTextureValue_Debug(TEInstance *instance, const char *identifier, TETexture *TE_NULLABLE texture, TEGraphicsContext * TE_NULLABLE context)
{
	const TEResult Result = TEInstanceLinkSetTextureValue(instance, identifier, texture, context);
	UE_LOG(LogTouchEngineCalls, Verbose, TEXT(" Called TEInstanceLinkSetTextureValue on thread [%s] for identifier `%hs`.%s%s%s   => Returned `%hs`"),
		*UE::TouchEngine::GetCurrentThreadStr(),
		identifier,
		instance ? TEXT("") : TEXT(" `instance` is null!"),
		texture ? TEXT("") : TEXT(" `texture` is null!"),
		context ? TEXT("") : TEXT(" `semaphore` is null!"),
		TEResultGetDescription(Result));
	return Result;
}

// /*
//  Returns true if 'instance' has a pending texture transfer for 'texture'
//  */
// inline bool TEInstanceHasVulkanTextureTransfer_Debug(TEInstance *instance, const TETexture *texture)
// {
// 	const bool Result = TEInstanceHasVulkanTextureTransfer(instance, texture);
// 	UE_LOG(LogTouchEngineCalls, Verbose, TEXT(" Called TEInstanceHasVulkanTextureTransfer on thread [%s].%s%s   => Returned `%s`"),
// 		*UE::TouchEngine::GetCurrentThreadStr(),
// 		instance ? TEXT("") : TEXT(" `instance` is null!"),
// 		texture ? TEXT("") : TEXT(" `texture` is null!"),
// 		Result ? TEXT("TRUE") : TEXT("FALSE"));
// 	return Result;
// }
