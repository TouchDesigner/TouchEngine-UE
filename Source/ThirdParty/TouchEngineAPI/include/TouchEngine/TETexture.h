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


#ifndef TETexture_h
#define TETexture_h

#include <TouchEngine/TEObject.h>
#include <stdint.h>
#ifdef __APPLE__
	#include <OpenGL/gltypes.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

TE_ASSUME_NONNULL_BEGIN

#ifdef _WIN32

typedef void *HANDLE;

struct ID3D11Texture2D;
struct ID3D11Device;
enum DXGI_FORMAT;

typedef unsigned int GLenum;
typedef int GLint;
typedef unsigned int GLuint;

#endif

typedef TE_ENUM(TETextureType, int32_t) 
{
	TETextureTypeOpenGL,
	TETextureTypeD3D,
	TETextureTypeDXGI,
	TETextureTypeIOSurface
};

/*
	Used by TEIOSurfaceTexture

	Support for other formats may be possible, please ask if you need them.
*/
typedef TE_ENUM(TETextureFormat, int32_t)
{
	TETextureFormatR8,
	TETextureFormatR16,
	TETextureFormatR16F,
	TETextureFormatR32,
	TETextureFormatR32F,
	TETextureFormatRG8,
	TETextureFormatRG16,
	TETextureFormatRG16F,
	TETextureFormatRG32,
	TETextureFormatRG32F,
	TETextureFormatRGB8,
	TETextureFormatRGB16,
	TETextureFormatRGB16F,
	TETextureFormatRGB32,
	TETextureFormatRGB32F,
	TETextureFormatRGB10_A2,
	TETextureFormatRGBA8,
	TETextureFormatSRGB8,
	TETextureFormatSRGBA8,
	TETextureFormatRGBA16,
	TETextureFormatRGBA16F,
	TETextureFormatRGBA32,
	TETextureFormatRGBA32F
};

typedef TEObject TETexture;
typedef struct TEOpenGLTexture_ TEOpenGLTexture;

#ifdef _WIN32
typedef struct TEDXGITexture_ TEDXGITexture;
typedef struct TED3D11Texture_ TED3D11Texture;
#endif

#ifdef __APPLE__
typedef struct TEIOSurfaceTexture_ TEIOSurfaceTexture;
#endif

/*
 Returns the type (OpenGL, D3D, DXGI, IOSurface) of texture
 */
TE_EXPORT TETextureType TETextureGetType(const TETexture *texture);

/*
 Returns true if the texture is vertically flipped in its native coordinate space.
*/
TE_EXPORT bool TETextureIsVerticallyFlipped(const TETexture *texture);


#ifdef _WIN32

/*
 DXGI Textures
 */
typedef void (*TEDXGITextureReleaseCallback)(HANDLE handle, void * TE_NULLABLE info);

/*
Create a texture from a shared handle
'handle' must be the result of a call to IDXGIResource::GetSharedHandle(), and not IDXGIResource1::CreateSharedHandle()
'flipped' is true if the texture is vertically flipped, with its origin in the bottom-left corner.
'callback' will be called with the values passed to 'handle' and 'info' when the texture is released
The caller is responsible for releasing the returned TEDXGITexture using TERelease()
*/
TE_EXPORT TEDXGITexture *TEDXGITextureCreate(HANDLE handle, bool flipped, TEDXGITextureReleaseCallback TE_NULLABLE callback, void *info);

/*
 Create a texture from a TED3D11Texture. Depending on the source texture, this may involve copying
 the texture to permit sharing.
 The caller is responsible for releasing the returned TEDXGITexture using TERelease()
 */
TE_EXPORT TEDXGITexture *TEDXGITextureCreateFromD3D(TED3D11Texture *texture);

TE_EXPORT HANDLE TEDXGITextureGetHandle(const TEDXGITexture *texture);

/*
 D3D11 Textures
 */

/*
 'flipped' is true if the texture is vertically flipped, with its origin in the bottom-left corner.
 The caller is responsible for releasing the returned TED3D11Texture using TERelease()
 */
TE_EXPORT TED3D11Texture *TED3D11TextureCreate(ID3D11Texture2D *texture, bool flipped);

/*
 'flipped' is true if the texture is vertically flipped, with its origin in the bottom-left corner.
 'typedFormat' is a typed texture format specifying how the typeless format of the texture is to be interpreted.
 The caller is responsible for releasing the returned TED3D11Texture using TERelease()
 */
TE_EXPORT TED3D11Texture *TED3D11TextureCreateTypeless(ID3D11Texture2D *texture, bool flipped, DXGI_FORMAT typedFormat);


/*
 Returns the underlying ID3D11Texture2D.
 This texture should be considered to be owned by the TED3D11Texture and should not be retained beyond
 the lifetime of its owner.
 */
TE_EXPORT ID3D11Texture2D *TED3D11TextureGetTexture(const TED3D11Texture *texture);

#endif

/*
 OpenGL Textures
 */

typedef void (*TEOpenGLTextureReleaseCallback)(GLuint texture, void * TE_NULLABLE info);

/*
Create a texture from an OpenGL texture
'internaFormat' must be a sized format (eg GL_RGBA8, not GL_RGBA)
'callback' will be called with the values passed to 'texture' and 'info' when the texture is released - the 
texture should remain valid until that happens.
'flipped' is true if the texture is vertically flipped, with its origin in the top-left corner.
The caller is responsible for releasing the returned TEOpenGLTexture using TERelease()
*/
TE_EXPORT TEOpenGLTexture *TEOpenGLTextureCreate(GLuint texture, GLenum target, GLint internalFormat, int32_t width, int32_t height, bool flipped, TEOpenGLTextureReleaseCallback TE_NULLABLE callback, void * TE_NULLABLE info);

/*
 Returns the underlying OpenGL texture.
 This texture is owned by the TEOpenGLTexture and should not be used beyond the lifetime of its owner.
 */
TE_EXPORT GLuint TEOpenGLTextureGetName(const TEOpenGLTexture *texture);

TE_EXPORT GLenum TEOpenGLTextureGetTarget(const TEOpenGLTexture *texture);

TE_EXPORT GLint TEOpenGLTextureGetInternalFormat(const TEOpenGLTexture *texture);

TE_EXPORT int32_t TEOpenGLTextureGetWidth(const TEOpenGLTexture *texture);

TE_EXPORT int32_t TEOpenGLTextureGetHeight(const TEOpenGLTexture *texture);

#ifdef __APPLE__

/*
 IOSurface Textures
 */

/*
 'flipped' is true if the texture is vertically flipped, with its origin in the top-left corner.
 'plane' is the plane of the IOSurface to use or 0 if the IOSurface is not planar.
 The caller is responsible for releasing the returned TEIOSurfaceTexture using TERelease()
 */
TE_EXPORT TEIOSurfaceTexture *TEIOSurfaceTextureCreate(IOSurfaceRef surface, TETextureFormat format, int plane, bool flipped);

/*
 Returns the underlying IOSurface.
 This surface should be considered to be owned by the TEIOSurfaceTexture and should not be retained beyond
 the lifetime of its owner.
 */
TE_EXPORT IOSurfaceRef TEIOSurfaceTextureGetSurface(const TEIOSurfaceTexture *texture);

#endif

TE_ASSUME_NONNULL_END

#ifdef __cplusplus
}
#endif

#endif /* TETexture_h */
