/* Shared Use License: This file is owned by Derivative Inc. (Derivative) and
 * can only be used, and/or modified for use, in conjunction with 
 * Derivative's TouchDesigner software, and only if you are a licensee who has
 * accepted Derivative's TouchDesigner license or assignment agreement (which
 * also govern the use of this file).  You may share a modified version of this
 * file with another authorized licensee of Derivative's TouchDesigner software.
 * Otherwise, no redistribution or sharing of this file, with or without
 * modification, is permitted.
 *
 * TouchEngine
 *
 * Copyright © 2018 Derivative. All rights reserved.
 *
 */

#ifndef TEObject_h
#define TEObject_h

#include "TEBase.h"
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
	TEObjectTypeParameterInfo,
	TEObjectTypeString,
	TEObjectTypeStringArray,
	TEObjectTypeTable,
	TEObjectTypeFloatBuffer
};

typedef struct TEObject_ TEObject;

/*
 Retains a TEObject, incrementing its reference count. If you retain an object,
 you are responsible for releasing it (using TERelease()).
 Returns the input value.
 */
#define TERetain(x) TERetain_((TEObject *)(x))
TE_EXPORT TEObject *TERetain_(TEObject *object);

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
#define TEGetType(x) TEGetType_((TEObject *)(x))
TE_EXPORT TEObjectType TEGetType_(TEObject *object);

TE_ASSUME_NONNULL_END

#ifdef __cplusplus
}
#endif

#endif
