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


#ifndef TEObject_h
#define TEObject_h

#include <TouchEngine/TEBase.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

TE_ASSUME_NONNULL_BEGIN

typedef TE_ENUM(TEObjectType, int32_t)
{
	TEObjectTypeUnknown,
	TEObjectTypeInstance,
	TEObjectTypeTexture,
	TEObjectTypeAdapter,
	TEObjectTypeGraphicsContext,
	TEObjectTypeLinkInfo,
	TEObjectTypeLinkState,
	TEObjectTypeString,
	TEObjectTypeStringArray,
	TEObjectTypeTable,
	TEObjectTypeFloatBuffer
};

typedef void TEObject;

/*
 Retains a TEObject, incrementing its reference count. If you retain an object,
 you are responsible for releasing it (using TERelease()).
 Returns the input value.
 */
TE_EXPORT TEObject *TERetain(TEObject *object);

/*
 Releases a TEObject, decrementing its reference count. When the last reference
 to an object is released, it is deallocated and destroyed.
 Sets the passed pointer to NULL.
 */
#define TERelease(x) TERelease_((TEObject **)(x))
TE_EXPORT void TERelease_(TEObject * TE_NULLABLE * TE_NULLABLE object);

/*
 Returns the type of a TEObject.
 */
TE_EXPORT TEObjectType TEGetType(const TEObject *object);

TE_ASSUME_NONNULL_END

#ifdef __cplusplus
}
#endif

#endif
