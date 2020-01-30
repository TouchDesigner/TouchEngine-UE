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

#ifndef TEResult_h
#define TEResult_h

#include "TEBase.h"
#include "TETypes.h"

#ifdef __cplusplus
extern "C" {
#endif

TE_ASSUME_NONNULL_BEGIN

/* 
 Returns a description of the TEResult as a UTF-8 encoded string in English,
 or NULL if the TEResult was invalid.
 */
TE_EXPORT const char * TE_NULLABLE TEResultGetDescription(TEResult result);

/*
 Returns a TESeverity for a TEResult
 */
TE_EXPORT TESeverity TEResultGetSeverity(TEResult result);

TE_ASSUME_NONNULL_END

#ifdef __cplusplus
}
#endif

#endif /* TEResult_h */
