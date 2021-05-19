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


#ifndef TEGraphicsContext_h
#define TEGraphicsContext_h

#include <TouchEngine/TEObject.h>
#include <TouchEngine/TEResult.h>
#include <TouchEngine/TETexture.h>

#ifdef __APPLE__
	#include <OpenGL/OpenGL.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

TE_ASSUME_NONNULL_BEGIN

typedef TEObject TEGraphicsContext;
typedef struct TEOpenGLContext_ TEOpenGLContext;
typedef struct TED3D11Context_ TED3D11Context;
typedef struct TEAdapter_ TEAdapter;

#ifdef _WIN32
struct ID3D11Device;
typedef struct HGLRC__ *HGLRC;
typedef struct HDC__ *HDC;
#endif

/*
 Returns the TEAdapter associated with a context.

 */
TE_EXPORT TEAdapter *TEGraphicsContextGetAdapter(TEGraphicsContext *context);

#ifdef _WIN32

/*
 Creates a graphics context for use with Direct3D.
 
 'device' is the Direct3D device to be used for texture creation.
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
 Work may be done in the graphics context by this call.
 The caller is responsible for releasing the returned TETexture using TERelease() -
	work may be done in the graphics context by the final call to TERelease()
	for the returned texture.
 */
TE_EXPORT TEResult TED3D11ContextCreateTexture(TED3D11Context *context, TEDXGITexture *source, TED3D11Texture * TE_NULLABLE * TE_NONNULL texture);

/*
 Creates a graphics context for use with OpenGL.
 
 'dc' is a valid device context to be used for OpenGL commands
	This value can be changed later using TEOpenGLContextSetDC()
 'rc' is a valid OpenGL render context to be used for OpenGL commands
 'context' will be set to a TEOpenGLContext on return, or NULL if a context could not be created.
	The caller is responsible for releasing the returned TEOpenGLContext using TERelease()
 Returns TEResultSucccess or an error
 */
TE_EXPORT TEResult TEOpenGLContextCreate(HDC dc,
									  		HGLRC rc,
											TEOpenGLContext * TE_NULLABLE * TE_NONNULL context);

/*
 Creates a graphics context for use with OpenGL where the context is bound
 to particular device(s) with the WGL_NV_gpu_affinity extension.
 
 'dc' is a valid device context to be used for OpenGL commands
	This value can be changed later using TEOpenGLContextSetDC()
 'rc' is a valid OpenGL render context to be used for OpenGL commands
 'context' will be set to a TEOpenGLContext on return, or NULL if a context could not be created.
	The caller is responsible for releasing the returned TEOpenGLContext using TERelease()
 Returns TEResultSucccess or an error
 */
TE_EXPORT TEResult TEOpenGLNVAffinityContextCreate(HDC dc,
													HGLRC rc,
													TEOpenGLContext * TE_NULLABLE * TE_NONNULL context);

/*
 Creates a graphics context for use with OpenGL where the context is bound
 to a particular device with the WGL_AMD_gpu_association extension.
 
 The supplied HGLRC must have been created with a GPU association and must be active at the time
 of this call.

 'rc' is a valid OpenGL render context to be used for OpenGL commands
 'context' will be set to a TEOpenGLContext on return, or NULL if a context could not be created.
	The caller is responsible for releasing the returned TEOpenGLContext using TERelease()
 Returns TEResultSucccess or an error
 */
TE_EXPORT TEResult TEOpenGLAMDAssociatedContextCreate(HGLRC rc,
														TEOpenGLContext * TE_NULLABLE * TE_NONNULL context);

/*
 Returns the device context associated with an OpenGL context, or NULL.
 */
TE_EXPORT HDC TEOpenGLContextGetDC(TEOpenGLContext *context);

/*
 Returns the OpenGL render context associated with an OpenGL context, or NULL.
 */
TE_EXPORT HGLRC TEOpenGLContextGetRC(TEOpenGLContext *context);

/*
 Change the device context used by an OpenGL context.
 Returns TEResultSuccess on success, or an error.
 */
TE_EXPORT TEResult TEOpenGLContextSetDC(TEOpenGLContext *context, HDC dc);

/*
 Work may be done in the graphics context by this call.
 The current OpenGL texture binding may be changed during this call.
 The caller is responsible for releasing the returned TETexture using TERelease() -
	work may be done in the graphics context associated with the instance by the final
	call to TERelease() for the returned texture.
 */
TE_EXPORT TEResult TEOpenGLContextCreateTexture(TEOpenGLContext *context, TEDXGITexture *source, TEOpenGLTexture * TE_NULLABLE * TE_NONNULL texture);

#endif

#ifdef __APPLE__

TE_EXPORT TEResult TEOpenGLContextCreate(CGLContextObj cgl, TEOpenGLContext * TE_NULLABLE * TE_NONNULL context);

TE_EXPORT CGLContextObj TEOpenGLContextGetCGLContext(TEOpenGLContext *context);

TE_EXPORT TEResult TEOpenGLContextCreateTexture(TEOpenGLContext *context, TEIOSurfaceTexture *source, TEOpenGLTexture * TE_NULLABLE * TE_NONNULL texture);

#endif

TE_ASSUME_NONNULL_END

#ifdef __cplusplus
}
#endif

#endif
