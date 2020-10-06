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

#ifdef __cplusplus
}
#endif

#endif /* TEBase_h */
