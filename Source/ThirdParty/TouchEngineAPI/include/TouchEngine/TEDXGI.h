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


#ifndef TEDXGI_h
#define TEDXGI_h

#include <TouchEngine/TETexture.h>

#ifdef __cplusplus
extern "C" {
#endif

TE_ASSUME_NONNULL_BEGIN

#ifdef _WIN32

typedef void *HANDLE;

/*
 DXGI Textures - see TETexture.h for functions common to all textures
 */

typedef struct TEDXGITexture_ TEDXGITexture;
typedef struct TEInstance_ TEInstance;

/*
 Event callback for DXGI Textures
 */
typedef void (*TEDXGITextureCallback)(HANDLE handle, TEObjectEvent event, void * TE_NULLABLE info);

/*
Create a texture from a shared handle
'handle' must be the result of a call to IDXGIResource::GetSharedHandle(), and not IDXGIResource1::CreateSharedHandle()
'origin' is the position in 2D space of the 0th texel of the texture
'map' describes how components are to be mapped when the texture is read. If components are not swizzled, you
	can pass kTETextureComponentMapIdentity
'callback' will be called with the values passed to 'handle' and 'info' when the texture is released
The caller is responsible for releasing the returned TEDXGITexture using TERelease()
*/
TE_EXPORT TEDXGITexture *TEDXGITextureCreate(HANDLE handle,
											TETextureOrigin origin,
											TETextureComponentMap map,
											TEDXGITextureCallback TE_NULLABLE callback,
											void * TE_NULLABLE info);

TE_EXPORT HANDLE TEDXGITextureGetHandle(const TEDXGITexture *texture);

/*
 Sets 'callback' to be invoked for object use and lifetime events - see TEObjectEvent in TEObject.h.
 This replaces (or cancels) any callback previously set on the TEDXGITexture
 */
TE_EXPORT TEResult TEDXGITextureSetCallback(TEDXGITexture *texture, TEDXGITextureCallback TE_NULLABLE callback, void * TE_NULLABLE info);

/*
 Some older versions of TouchDesigner require textures backed by DXGI keyed mutexes are released to 0 before they are
 	used by the instance. If this function returns true, always release to 0 rather than any other value, and add a
 	corresponding texture transfer.
 This may change during configuration of an instance, and must be queried after receiving TEEventInstanceReady
 See also TEInstanceAddTextureTransfer() in TEInstance.h 
 */
TE_EXPORT bool TEInstanceRequiresDXGIReleaseToZero(TEInstance *instance);

#endif // _WIN32

TE_ASSUME_NONNULL_END

#ifdef __cplusplus
}
#endif

#endif // TEDXGI_h
