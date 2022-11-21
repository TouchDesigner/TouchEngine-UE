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


#ifndef TED3D11_h
#define TED3D11_h

#include <TouchEngine/TETexture.h>
#include <TouchEngine/TEGraphicsContext.h>
#include <TouchEngine/TED3D.h>
#ifdef _WIN32
#include <d3d11.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

TE_ASSUME_NONNULL_BEGIN

#ifdef _WIN32

/*
 D3D11 Textures - see TETexture.h for functions common to all textures, and TEDXGI.h for DXGI textures
 */

typedef struct TED3D11Texture_ TED3D11Texture;
typedef struct TEInstance_ TEInstance;

/*
 Event callback for D3D11 Textures
 When this callback is invoked with TEObjectEventRelease the ID3D11Texture2D is no longer being used by TouchEngine
 	and it may be re-used. TouchEngine keeps a reference to the texture through this callback, and issues its
	final ID3D11Texture2D::Release() after the callback completes.
 */
typedef void (*TED3D11TextureCallback)(ID3D11Texture2D *texture, TEObjectEvent event, void * TE_NULLABLE info);


/*
 'origin' is the position in 2D space of the 0th texel of the texture
 'map' describes how components are to be mapped when the texture is read. If components are not swizzled, you
	can pass kTETextureComponentMapIdentity
 'callback' will be invoked for object use and lifetime events - see TEObjectEvent in TEObject.h
 The caller is responsible for releasing the returned TED3D11Texture using TERelease()
 */
TE_EXPORT TED3D11Texture *TED3D11TextureCreate(ID3D11Texture2D *texture,
												TETextureOrigin origin, TETextureComponentMap map,
												TED3D11TextureCallback TE_NULLABLE callback, void * TE_NULLABLE info);

/*
 'origin' is the position in 2D space of the 0th texel of the texture
 'map' describes how components are to be mapped when the texture is read. If components are not swizzled, you
	can pass kTETextureComponentMapIdentity
 'typedFormat' is a typed texture format specifying how the typeless format of the texture is to be interpreted.
 'callback' will be called with the values passed to 'texture' and 'info' when the texture is released
 The caller is responsible for releasing the returned TED3D11Texture using TERelease()
 */
TE_EXPORT TED3D11Texture *TED3D11TextureCreateTypeless(ID3D11Texture2D *texture,
														TETextureOrigin origin, TETextureComponentMap map, DXGI_FORMAT typedFormat,
														TED3D11TextureCallback TE_NULLABLE callback, void * TE_NULLABLE info);

/*
 Returns the underlying ID3D11Texture2D.
 This texture should be considered to be owned by the TED3D11Texture and should not be retained beyond
 the lifetime of its owner.
 */
TE_EXPORT ID3D11Texture2D *TED3D11TextureGetTexture(const TED3D11Texture *texture);

/*
 Sets 'callback' to be invoked for object use and lifetime events - see TEObjectEvent in TEObject.h.
 This replaces (or cancels) any callback previously set on the TED3D11Texture
 */
TE_EXPORT TEResult TED3D11TextureSetCallback(TED3D11Texture *texture, TED3D11TextureCallback TE_NULLABLE callback, void * TE_NULLABLE info);

/*
 D3D11 Graphics Contexts - see TEGraphicsContext.h for functions common to all graphics contexts
 */

typedef struct TED3D11Context_ TED3D11Context;

/*
 Creates a graphics context for use with Direct3D 11.
 
 'device' is the Direct3D device to be used for texture creation.
 	If the ID3D11Device was created for a specific adaptor, that adapter must be from a DXGI 1.1 factory
 	(IDXGIFactory1, not IDXGIFactory)
 'context' will be set to a TED3D11Context on return, or NULL if a context could not be created.
	The caller is responsible for releasing the returned TED3D11Context using TERelease()
 Returns TEResultSucccess or an error
 */
TE_EXPORT TEResult TED3D11ContextCreate(ID3D11Device * TE_NULLABLE device,
										TED3D11Context * TE_NULLABLE * TE_NONNULL context);

/*
Returns the ID3D11Device associated with a Direct3D context, or NULL.
*/
TE_EXPORT ID3D11Device *TED3D11ContextGetDevice(TED3D11Context *context);

/*
 Creates a TED3D11Texture from a TED3DSharedTexture

 Both the created TED3D11Texture and the TED3DSharedTexture refer to the same resource.
 	The context maintains an internal cache, so repeated calls to this function for the same texture are faster than
 	instantiating an ID3D11Texture2D from the TED3DSharedTexture manually.
 The caller is responsible for releasing the returned TETexture using TERelease()
 */
TE_EXPORT TEResult TED3D11ContextGetTexture(TED3D11Context *context, TED3DSharedTexture *source, TED3D11Texture * TE_NULLABLE * TE_NONNULL texture);

/*
 Some older versions of TouchDesigner require textures backed by DXGI keyed mutexes are released to 0 before they are
 	used by the instance. If this function returns true, always release to 0 rather than any other value, and add a
 	corresponding texture transfer.
 This may change during configuration of an instance, and must be queried after receiving TEEventInstanceReady
 See also TEInstanceAddTextureTransfer() in TEInstance.h 
 */
TE_EXPORT bool TEInstanceRequiresKeyedMutexReleaseToZero(TEInstance *instance);

#endif // _WIN32

TE_ASSUME_NONNULL_END

#ifdef __cplusplus
}
#endif

#endif // TED3D11_h
