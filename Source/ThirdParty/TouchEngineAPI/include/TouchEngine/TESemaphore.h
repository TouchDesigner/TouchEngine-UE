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


#ifndef TESemaphore_h
#define TESemaphore_h

#include <TouchEngine/TEObject.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

TE_ASSUME_NONNULL_BEGIN

typedef TEObject TESemaphore;

typedef TE_ENUM(TESemaphoreType, int32_t) 
{
	TESemaphoreTypeVulkan,
    TESemaphoreTypeMetal,
    TESemaphoreTypeD3DFence,
};

/*
 Returns the type (Vulkan, etc) of semaphore
 */
TE_EXPORT TESemaphoreType TESemaphoreGetType(const TESemaphore *semaphore);

TE_ASSUME_NONNULL_END

#ifdef __cplusplus
}
#endif

#endif /* TESemaphore_h */
