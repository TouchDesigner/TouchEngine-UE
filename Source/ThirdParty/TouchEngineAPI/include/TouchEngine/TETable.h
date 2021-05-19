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


#ifndef TETable_h
#define TETable_h

#include <TouchEngine/TEObject.h>
#include <TouchEngine/TEResult.h>

#ifdef __cplusplus
extern "C" {
#endif

TE_ASSUME_NONNULL_BEGIN

typedef struct TETable_ TETable;

/*
 Creates a new TETable.
	The caller is responsible for releasing the returned TETable using TERelease()
 */
TE_EXPORT TETable *TETableCreate();

/*
 Creates a new table, duplicating an existing one.
	The caller is responsible for releasing the returned TETable using TERelease()
 */
TE_EXPORT TETable *TETableCreateCopy(const TETable *table);

/*
 Returns the number of rows in the table.
 */
TE_EXPORT int32_t TETableGetRowCount(const TETable *table);

/*
 Returns the number of columns in the table.
 */
TE_EXPORT int32_t TETableGetColumnCount(const TETable *table);

/*
 Returns the UTF-8 encoded string at the cell at the given row and column indices, or NULL
 if an error occurs.
 The returned string is invalidated by any subsequent modification of the table.
 */
TE_EXPORT const char *TETableGetStringValue(const TETable *table, int32_t row, int32_t column);

/*
 Resizes the table, adding or deleting rows and columns as necessary.
 */
TE_EXPORT void TETableResize(TETable *table, int32_t rows, int32_t columns);

/*
 Sets the string value of the cell at the given row and column indices.
 'value' is a UTF-8 encoded string, or NULL to remove any existing value.
 */
TE_EXPORT TEResult TETableSetStringValue(TETable *table, int32_t row, int32_t column, const char *value);

TE_ASSUME_NONNULL_END

#ifdef __cplusplus
}
#endif

#endif
