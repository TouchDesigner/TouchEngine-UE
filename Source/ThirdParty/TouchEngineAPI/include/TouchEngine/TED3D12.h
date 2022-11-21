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


#ifndef TED3D12_h
#define TED3D12_h

#include <TouchEngine/TETexture.h>
#include <TouchEngine/TEGraphicsContext.h>
#include <TouchEngine/TED3D.h>
#ifdef _WIN32
#include <d3d12.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

TE_ASSUME_NONNULL_BEGIN

#ifdef _WIN32

/*
 See TED3D.h for shared textures and fences
 */

/*
 D3D12 Graphics Contexts - see TEGraphicsContext.h for functions common to all graphics contexts
 */

typedef struct TED3D12Context_ TED3D12Context;

/*
 Creates a graphics context for use with Direct3D 12. Passing a TED3D12Context to TEInstanceAssociateGraphicsContext() will
 - associate the instance with the same physical device
 - cause the instance to return D3D12 textures for output link values
 
 'device' is the Direct3D device to be used for texture creation.
 'context' will be set to a TED3D12Context on return, or NULL if a context could not be created.
	The caller is responsible for releasing the returned TED3D12Context using TERelease()
 Returns TEResultSucccess or an error
 */
TE_EXPORT TEResult TED3D12ContextCreate(ID3D12Device * TE_NULLABLE device,
										TED3D12Context * TE_NULLABLE * TE_NONNULL context);

/*
Returns the ID3D12Device associated with a Direct3D context, or NULL.
*/
TE_EXPORT ID3D12Device *TED3D12ContextGetDevice(TED3D12Context *context);

#endif // _WIN32

TE_ASSUME_NONNULL_END

#ifdef __cplusplus
}
#endif

#endif // TED3D12_h