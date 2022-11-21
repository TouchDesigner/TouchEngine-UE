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

#ifdef __cplusplus
extern "C" {
#endif

TE_ASSUME_NONNULL_BEGIN

typedef TEObject TEGraphicsContext;
typedef struct TEAdapter_ TEAdapter;

/*
 Returns the TEAdapter associated with a context.
 	The caller is responsible for releasing the returned TEAdapter using TERelease()
 */
TE_EXPORT TEAdapter *TEGraphicsContextGetAdapter(TEGraphicsContext *context);

/*
 See TEOpenGL.h, TED3D11.h, etc, to create and use TEGraphicsContexts
 */

TE_ASSUME_NONNULL_END

#ifdef __cplusplus
}
#endif

#endif
