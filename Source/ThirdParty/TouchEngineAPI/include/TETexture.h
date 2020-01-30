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

#ifndef TETexture_h
#define TETexture_h

#include "TEBase.h"
#include "TETypes.h"

#ifdef __cplusplus
extern "C" {
#endif

TE_ASSUME_NONNULL_BEGIN

typedef TEObject TETexture;

/*
 Returns the type (OpenGL, D3D, DXGI) of texture
 Depending on the type, use functions in
 TEOpenGLTexture.h
 TEDXGITexture.h
 TED3DTexture.h
 To work with the texture object.
 */
TE_EXPORT TETextureType TETextureGetType(TETexture *texture);

/*
 Returns true if the texture is vertically flipped in its native coordindate space.
*/
TE_EXPORT bool TETextureIsVerticallyFlipped(TETexture *texture);

TE_ASSUME_NONNULL_END

#ifdef __cplusplus
}
#endif

#endif /* TETexture_h */
