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

#ifndef TEBase_h
#define TEBase_h

#ifdef __cplusplus
extern "C" {
#endif

#if defined(__APPLE__)
	#define TE_ASSUME_NONNULL_BEGIN _Pragma("clang assume_nonnull begin")
	#define TE_ASSUME_NONNULL_END	_Pragma("clang assume_nonnull end")
	#define TE_NONNULL _Nonnull
	#define TE_NULLABLE _Nullable
	#if !defined(TE_EXPORT)
		#define TE_EXPORT __attribute__((visibility("default")))
	#endif
#else
	#define TE_ASSUME_NONNULL_BEGIN
	#define TE_ASSUME_NONNULL_END
	#define TE_NONNULL
	#define TE_NULLABLE
	#if !defined(TE_EXPORT)
		#if defined (TE_BUILD_DLL)
			#define TE_EXPORT __declspec(dllexport)
		#else
			#define TE_EXPORT __declspec(dllimport)
		#endif
	#endif
#endif

// TODO: This form is supported for C by MSVC and LLVM, may need rethink if we require support for other compilers
#define TE_ENUM(_name, _type) enum _name : _type _name; enum _name : _type

TE_ASSUME_NONNULL_BEGIN

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

TE_ASSUME_NONNULL_END

#ifdef __cplusplus
}
#endif

#endif /* TEBase_h */
