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
 * Copyright © 2019 Derivative. All rights reserved.
 *
 */

#ifndef TEIOSurfaceTexture_h
#define TEIOSurfaceTexture_h

#include "TETexture.h"

#ifdef __APPLE__

#include "TETypes.h"

#ifdef __cplusplus
extern "C" {
#endif

TE_ASSUME_NONNULL_BEGIN

typedef TETexture TEIOSurfaceTexture;

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
TE_EXPORT IOSurfaceRef TEIOSurfaceTextureGetSurface(TEIOSurfaceTexture *texture);

TE_ASSUME_NONNULL_END

#ifdef __cplusplus
}
#endif

#endif // __APPLE__
#endif // TEIOSurfaceTexture_h
