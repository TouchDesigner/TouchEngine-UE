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
#include <TouchEngine/TEResult.h>
#include <stdint.h>
#include <stdbool.h>
#ifdef __APPLE__
	#include <IOSurface/IOSurface.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

TE_ASSUME_NONNULL_BEGIN

typedef TE_ENUM(TETextureType, int32_t) 
{
	TETextureTypeOpenGL,
	TETextureTypeD3D11,
	TETextureTypeD3DShared,
	TETextureTypeIOSurface,
	TETextureTypeVulkan,
	TETextureTypeMetal,
};

/*
	Support for other formats may be possible, please ask if you need them.
*/
typedef TE_ENUM(TETextureFormat, int32_t)
{
	TETextureFormatInvalid,
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
	TETextureFormatRGB11F,
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

typedef TE_ENUM(TETextureComponentSource, int32_t)
{
	TETextureComponentSourceZero,
	TETextureComponentSourceOne,
	TETextureComponentSourceRed,
	TETextureComponentSourceGreen,
	TETextureComponentSourceBlue,
	TETextureComponentSourceAlpha
};

typedef struct TETextureComponentMap
{
	TETextureComponentSource r;
	TETextureComponentSource g;
	TETextureComponentSource b;
	TETextureComponentSource a;
} TETextureComponentMap;

typedef TE_ENUM(TETextureOrigin, int32_t)
{
	TETextureOriginTopLeft,
	TETextureOriginBottomLeft
};

extern TE_EXPORT const struct TETextureComponentMap kTETextureComponentMapIdentity;

typedef TEObject TETexture;

#ifdef __APPLE__
typedef struct TEIOSurfaceTexture_ TEIOSurfaceTexture;
#endif

/*
 Returns the type (OpenGL, D3D, DXGI, IOSurface, Vulkan) of texture
 */
TE_EXPORT TETextureType TETextureGetType(const TETexture *texture);

/*
 Returns the position in 2D space of the 0th texel of the texture
*/
TE_EXPORT TETextureOrigin TETextureGetOrigin(const TETexture *texture);

TE_EXPORT TETextureComponentMap TETextureGetComponentMap(TETexture* texture);

/*
 See TEOpenGL.h, TED3D11.h, etc, to create and use other TETexture types
 */

#ifdef __APPLE__

/*
 IOSurface Textures
 */
/*
 Release callback for IOSurface Textures
 When this callback is invoked the IOSurface is no longer being used by TouchEngine and
	it may be re-used. TouchEngine retains the texture through this callback, and issues its
	final CFRelease() after the callback completes.
 */
typedef void (*TEIOSurfaceTextureCallback)(IOSurfaceRef surface, TEObjectEvent event, void * TE_NULLABLE info);

/*
 'origin' is the position in 2D space of the 0th texel of the texture
 'plane' is the plane of the IOSurface to use or 0 if the IOSurface is not planar.
 The caller is responsible for releasing the returned TEIOSurfaceTexture using TERelease()
 */
TE_EXPORT TEIOSurfaceTexture *TEIOSurfaceTextureCreate(IOSurfaceRef surface,
	TETextureFormat format,
	int plane,
	TETextureOrigin origin,
	TETextureComponentMap map,
	TEIOSurfaceTextureCallback TE_NULLABLE callback,
	void * TE_NULLABLE info);

/*
 Returns the underlying IOSurface.
 This surface should be considered to be owned by the TEIOSurfaceTexture and should not be retained beyond
 the lifetime of its owner.
 */
TE_EXPORT IOSurfaceRef TEIOSurfaceTextureGetSurface(const TEIOSurfaceTexture *texture);

/*
 Returns the format of the texture.
 */
TE_EXPORT TETextureFormat TEIOSurfaceTextureGetFormat(const TEIOSurfaceTexture *texture);

TE_EXPORT TEResult TEIOSurfaceTextureSetCallback(TEIOSurfaceTexture *texture,
													TEIOSurfaceTextureCallback TE_NULLABLE callback,
													void * TE_NULLABLE info);

#endif

TE_ASSUME_NONNULL_END

#ifdef __cplusplus
}
#endif

#endif /* TETexture_h */
