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


#ifndef TEOpenGL_h
#define TEOpenGL_h

#include <TouchEngine/TETexture.h>
#include <TouchEngine/TEGraphicsContext.h>
#ifdef __APPLE__
	#include <OpenGL/OpenGL.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

TE_ASSUME_NONNULL_BEGIN

#ifdef _WIN32

typedef unsigned int GLenum;
typedef int GLint;
typedef unsigned int GLuint;

typedef struct HGLRC__ *HGLRC;
typedef struct HDC__ *HDC;

typedef unsigned int UINT;

#endif

typedef struct TEOpenGLTexture_ TEOpenGLTexture;

/*
 OpenGL Textures - see TETexture.h for functions common to all textures
 */

typedef void (*TEOpenGLTextureCallback)(GLuint texture, TEObjectEvent event, void * TE_NULLABLE info);

/*
Create a texture from an OpenGL texture
'internaFormat' must be a sized format (eg GL_RGBA8, not GL_RGBA)
'callback' will be invoked for object use and lifetime events - see TEObjectEvent in TEObject.h - the 
	OpenGL texture should remain valid until receiving TEObjectEventRelease.
'origin' is the position in 2D space of the 0th texel of the texture
'map' describes how components are to be mapped when the texture is read. If components are not swizzled, you
	can pass kTETextureComponentMapIdentity
The caller is responsible for releasing the returned TEOpenGLTexture using TERelease()
*/
TE_EXPORT TEOpenGLTexture *TEOpenGLTextureCreate(GLuint texture,
	GLenum target,
	GLint internalFormat,
	int32_t width,
	int32_t height,
	TETextureOrigin origin,
	TETextureComponentMap map,
	TEOpenGLTextureCallback TE_NULLABLE callback, void * TE_NULLABLE info);

/*
 Returns the underlying OpenGL texture.
 This texture is owned by the TEOpenGLTexture and should not be used beyond the lifetime of its owner.
 */
TE_EXPORT GLuint TEOpenGLTextureGetName(const TEOpenGLTexture *texture);

TE_EXPORT GLenum TEOpenGLTextureGetTarget(const TEOpenGLTexture *texture);

TE_EXPORT GLint TEOpenGLTextureGetInternalFormat(const TEOpenGLTexture *texture);

TE_EXPORT int32_t TEOpenGLTextureGetWidth(const TEOpenGLTexture *texture);

TE_EXPORT int32_t TEOpenGLTextureGetHeight(const TEOpenGLTexture *texture);

#ifdef _WIN32
/*
 Acquires an exclusive lock for access to the texture
 */
TE_EXPORT TEResult TEOpenGLTextureLock(TEOpenGLTexture *texture);

/*
 Releases a previously acquired exclusive lock for access to the texture
 */
TE_EXPORT TEResult TEOpenGLTextureUnlock(TEOpenGLTexture *texture);
#endif

/*
 Sets 'callback' to be invoked for object use and lifetime events - see TEObjectEvent in TEObject.h.
 This replaces (or cancels) any callback previously set on the TEOpenGLTexture
 */
TE_EXPORT TEResult TEDOpenGLTextureSetCallback(TEOpenGLTexture *texture, TEOpenGLTextureCallback TE_NULLABLE callback, void * TE_NULLABLE info);

/*
 OpenGL Adapter
 */

#ifdef _WIN32
/*
 Creates an adapter for an AMD GPU using the WGL_AMD_gpu_association OpenGL extension.
 
 'gpuID' is the ID of a GPU obtained using the extension.
 'adapter' will be set to a TEAdapter on return, or NULL if an adapter could not be created.
	The caller is responsible for releasing the returned TEAdapter using TERelease()
 Returns TEResultSucccess or an error
 */
TE_EXPORT TEResult TEAdapterCreateForAMDGPUAssociation(UINT gpuID, TEAdapter * TE_NULLABLE * TE_NONNULL adapter);
#endif

/*
 OpenGL Graphics Contexts - see TEGraphicsContext.h for functions common to all graphics contexts
 */

typedef struct TEOpenGLContext_ TEOpenGLContext;

#ifdef _WIN32

typedef struct TED3DSharedTexture_ TED3DSharedTexture;

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
 The current OpenGL framebuffer and texture bindings may be changed during this call.
 The caller is responsible for releasing the returned TETexture using TERelease() -
	work may be done in the graphics context associated with the instance by the final
	call to TERelease() for the returned texture.
 */
TE_EXPORT TEResult TEOpenGLContextGetTexture(TEOpenGLContext *context, TED3DSharedTexture *source, TEOpenGLTexture * TE_NULLABLE * TE_NONNULL texture);

#endif

#ifdef __APPLE__

TE_EXPORT TEResult TEOpenGLContextCreate(CGLContextObj cgl, TEOpenGLContext * TE_NULLABLE * TE_NONNULL context);

TE_EXPORT CGLContextObj TEOpenGLContextGetCGLContext(TEOpenGLContext *context);

TE_EXPORT TEResult TEOpenGLContextGetTexture(TEOpenGLContext *context, TEIOSurfaceTexture *source, TEOpenGLTexture * TE_NULLABLE * TE_NONNULL texture);

#endif

TE_ASSUME_NONNULL_END

#ifdef __cplusplus
}
#endif

#endif // TEOpenGL_h
