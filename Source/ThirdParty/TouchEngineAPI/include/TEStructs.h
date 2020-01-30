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


#ifndef TEStructs_h
#define TEStructs_h

#include "TETypes.h"
#include <assert.h>
#include <stddef.h>

struct TEParameterInfo
{
	/*
	 The scope (input or output) of the parameter.
	 */
	TEScope				scope;

	/*
	 How the parameter is intended to be used.
	 */
	TEParameterIntent	intent;

	/*
	  The type of parameter
	 */
	TEParameterType		type;

	/*
	 For value parameters, the number of values associated with the parameter
	 eg a colour may have four values for red, green, blue and alpha.

	 For group or complex parameters, the number of children.
	 */
	int32_t				count;

	/*
	 The human readable label for the parameter.
	 This may not be unique
	 */
	const char *		label;

	/*
 	 A unique identifier for the parameter. If the underlying file is unchanged this
 	 will persist through instantiations and will be the same for any given parameter
 	 in multiple instances of the same file.
 	 */
	const char *		identifier;
};

struct TEString
{
	/*
	 A null-terminated UTF-8 encoded string
	 */
	const char *string;
};

struct TEStringArray
{
	/*
	 The number of strings in the array
	 */
	int32_t					count;

	/*
	 The array of strings, each entry being a null-terminated UTF-8 encoded string
	 */
	const char * const *	strings;
};

struct TEStreamDescription
{
	/*
	 Sample rate
	 */
	double 		rate;

	/*
	 The maximum number of samples to be accommodated per channel
	 */
	int64_t 	maxSamples;

	/*
	 The number of channels
	 */
	int32_t 	numChannels;
};


#define kStructAlignmentError "struct misaligned for library"

static_assert(offsetof(struct TEParameterInfo, intent) == 4, kStructAlignmentError);
static_assert(offsetof(struct TEParameterInfo, type) == 8, kStructAlignmentError);
static_assert(offsetof(struct TEParameterInfo, count) == 12, kStructAlignmentError);
static_assert(offsetof(struct TEParameterInfo, label) == 16, kStructAlignmentError);
static_assert(offsetof(struct TEParameterInfo, identifier) == 24, kStructAlignmentError);

static_assert(offsetof(struct TEStringArray, strings) == 8, kStructAlignmentError);

static_assert(offsetof(struct TEStreamDescription, maxSamples) == 8, kStructAlignmentError);
static_assert(offsetof(struct TEStreamDescription, numChannels) == 16, kStructAlignmentError);

#endif

