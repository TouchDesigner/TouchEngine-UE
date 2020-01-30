/* Shared Use License: This file is owned by Derivative Inc. (Derivative) and
 * can only be used, and/or modified for use, in conjunction with 
 * Derivative's TouchDesigner software, and only if you are a licensee who has
 * accepted Derivative's TouchDesigner license or assignment agreement (which
 * also govern the use of this file).  You may share a modified version of this
 * file with another authorized licensee of Derivative's TouchDesigner software.
 * Otherwise, no redistribution or sharing of this file, with or without
 * modification, is permitted.
 *
 * TouchPlugIn
 *
 * Copyright © 2018 Derivative. All rights reserved.
 *
 */

#ifndef TEOpenGLTexture_h
#define TEOpenGLTexture_h

#include "TEBase.h"
#include "TETexture.h"
#ifdef __APPLE__
#include <OpenGL/gltypes.h>
#else
typedef unsigned int GLenum;
typedef int GLint;
typedef unsigned int GLuint;
#endif
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

TE_ASSUME_NONNULL_BEGIN

typedef TETexture TEOpenGLTexture;

typedef void (*TEOpenGLTextureReleaseCallback)(GLuint texture, void * TE_NULLABLE info);

/*
Create a texture from an OpenGL texture
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
TE_EXPORT GLuint TEOpenGLTextureGetName(TEOpenGLTexture *texture);

TE_EXPORT GLenum TEOpenGLTextureGetTarget(TEOpenGLTexture *texture);

TE_EXPORT GLint TEOpenGLTextureGetInternalFormat(TEOpenGLTexture *texture);

TE_EXPORT int32_t TEOpenGLTextureGetWidth(TETexture *texture);

TE_EXPORT int32_t TEOpenGLTextureGetHeight(TETexture *texture);

TE_ASSUME_NONNULL_END

#ifdef __cplusplus
}
#endif

#endif /* TEOpenGLTexture_h */
