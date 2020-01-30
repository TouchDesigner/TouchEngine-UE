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

#ifndef TED3DTexture_h
#define TED3DTexture_h

#include "TEBase.h"
#include "TETexture.h"
struct ID3D11Texture2D;
struct ID3D11Device;

#ifdef __cplusplus
extern "C" {
#endif

TE_ASSUME_NONNULL_BEGIN

typedef TETexture TED3DTexture;

/*
 'flipped' is true if the texture is vertically flipped, with its origin in the bottom-left corner.
 The caller is responsible for releasing the returned TED3DTexture using TERelease()
 */
TE_EXPORT TED3DTexture *TED3DTextureCreate(ID3D11Texture2D *texture, bool flipped);

/*
 Returns the underlying ID3D11Texture2D.
 This texture should be considered to be owned by the TED3DTexture and should not be retained beyond
 the lifetime of its owner.
 */
TE_EXPORT ID3D11Texture2D *TED3DTextureGetTexture(TED3DTexture *texture);

TE_ASSUME_NONNULL_END

#ifdef __cplusplus
}
#endif

#endif /* TED3DTexture_h */
