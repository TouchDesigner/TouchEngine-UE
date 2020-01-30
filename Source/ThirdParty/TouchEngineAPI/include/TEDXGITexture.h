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

#ifndef TEDXGITexture_h
#define TEDXGITexture_h

#include "TEBase.h"
#include "TETexture.h"

typedef void *HANDLE;

#ifdef __cplusplus
extern "C" {
#endif

TE_ASSUME_NONNULL_BEGIN

typedef TETexture TEDXGITexture;
typedef TETexture TED3D11Texture;

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
// TODO: this function may be removed from the API
TE_EXPORT TEDXGITexture *TEDXGITextureCreateFromD3D(TED3D11Texture *texture);

TE_EXPORT HANDLE TEDXGITextureGetHandle(TEDXGITexture *texture);

TE_ASSUME_NONNULL_END

#ifdef __cplusplus
}
#endif

#endif /* TEDXGITexture_h */
