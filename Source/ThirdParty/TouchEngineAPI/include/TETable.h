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

#ifndef TETable_h
#define TETable_h

#include "TEObject.h"

#ifdef __cplusplus
extern "C" {
#endif

TE_ASSUME_NONNULL_BEGIN

typedef TEObject TETable;

// TODO: document all of these

TE_EXPORT TETable *TETableCreate();
// TODO: 
/*
TE_EXPORT TETable *TETableCopy(TETable *table);
*/
TE_EXPORT int32_t TETableGetRowCount(TETable *table);
TE_EXPORT int32_t TETableGetColumnCount(TETable *table);

TE_EXPORT const char *TETableGetStringValue(TETable *table, int32_t row, int32_t column);

TE_EXPORT void TETableResize(TETable *table, int32_t rows, int32_t columns);
TE_EXPORT TEResult TETableSetStringValue(TETable *table, int32_t row, int32_t column, const char *value);

// TODO: add/delete rows/columns

TE_ASSUME_NONNULL_END

#ifdef __cplusplus
}
#endif

#endif
