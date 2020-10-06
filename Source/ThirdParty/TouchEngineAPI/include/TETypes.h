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

#ifndef TETypes_h
#define TETypes_h
#include "TEBase.h"
#include <stdint.h>

typedef TE_ENUM(TEResult, int32_t) 
{

	TEResultSuccess,

	TEResultInsufficientMemory,

	TEResultGPUAllocationFailed,

	TEResultInternalError,

	TEResultMissingResource,

	/*
	Samples were dropped during rendering due to full output stream buffers.
	*/
	TEResultDroppedSamples,

	/*
	Insufficient samples were provided during rendering.
	*/
	TEResultMissedSamples,

	/*
	Invalid arguments were given to a function, or a function was called at an improper time.
	*/
	TEResultBadUsage,

	/*
	The requested parameter does not belong to the instance.
	*/
	TEResultNoMatchingParameter,

	/*
	The operation was previously cancelled.
	*/
	TEResultCancelled,

	/*
	A TouchDesigner/Player key was found, but its update date is older than the build of TouchEngine being used.
	*/
	TEResultExpiredKey,

	/*
	No TouchDesigner or TouchPlayer key was found, or the installed key does not permit TouchEngine use.
	*/
	TEResultNoKey,

	/*
	General error trying to determine installed license. Open TouchDesigner to diagnose further.
	*/
	TEResultKeyError,

	/*
	Error reading or accessing a file.
	*/
	TEResultFileError,

	/*
	The version/build of TouchEngine being used is older than the one used to create the file being opened.
	*/
	TEResultNewerFileVersion,

	/*
	Unable to find TouchEngine executable to launch.
	*/
	TEResultTouchEngineNotFound,

	/*
	An instance of TouchEngine was specified but could not be used.
	*/
	TEResultTouchEngineBadPath,

	/*
	Launching the TouchEngine executable failed.
	*/
	TEResultFailedToLaunchTouchEngine,

	/*
	A required feature is not available on the selected system
	(eg the selected graphics adapater does not offer a required command)
	*/
	TEResultFeatureNotSupportedBySystem,

	/*
	The version of TouchEngine being used is older than the TouchEngine library.

	This result should be considered a warning and operation will continue, potentially with limited functionality.	
	*/
	TEResultOlderEngineVersion,

	/*
	The OS denied permission
	*/
	TEResultPermissionDenied,

	/*
	An error with bindings within the file prevented it being loaded.
	*/
	TEResultFileBindingsError,

	/*
	There were problems loading the file but loading continued.
	*/
	TEResultFileLoadWarnings
};

typedef TE_ENUM(TEObjectType, int32_t)
{
	TEObjectTypeUnknown, // TODO: or not? of most use internally
	TEObjectTypeInstance,
	TEObjectTypeTexture,
	TEObjectTypeAdapter,
	TEObjectTypeGraphicsContext,
	TEObjectTypeParameterInfo,
	TEObjectTypeString,
	TEObjectTypeStringArray,
	TEObjectTypeStreamDescription,
	TEObjectTypeTable
};

typedef TE_ENUM(TESeverity, int32_t)
{
	/*
	Success
	*/
	TESeverityNone,
	/*
	The requested action may have been performed partially
	*/
	TESeverityWarning,
	/*
	The requested action could not be performed
	*/
	TESeverityError
};

typedef TE_ENUM(TEEvent, int32_t) 
{
	TEEventGeneral,
	TEEventInstanceDidLoad,
	TEEventParameterLayoutDidChange,
	TEEventFrameDidFinish
};

typedef TE_ENUM(TETimeMode, int32_t) 
{
	/*
	Rendering time is determined by times provided by you.
	*/
	TETimeExternal,

	/*
	An internal clock is used to drive real-time rendering.
	*/
	TETimeInternal
};

typedef TE_ENUM(TEScope, int32_t) 
{
	TEScopeInput,
	TEScopeOutput
};

typedef TE_ENUM(TEParameterType, int32_t) 
{
	/*
	 Multiple parameters collected according to user preference
	 */
	TEParameterTypeGroup,

	/*
	 Multiple parameters which collectively form a single complex parameter
	 */
	TEParameterTypeComplex,

	/*
	 bool
	*/
	TEParameterTypeBoolean,

	/*
	 double
	 */
	TEParameterTypeDouble,

	/*
	 int32_t
	 */
	TEParameterTypeInt,

	/*
	 UTF-8 char * (input)
	 TEString * (output)
	 */
	TEParameterTypeString,


	/*
	 TETexture *
	 */
	TEParameterTypeTexture,

	/*
	 planar float sample buffers
	 */
	TEParameterTypeFloatStream,

	/*
	 TETable *
	  or
	 UTF-8 char * (input)
	 TEString * (output)
	 */
	TEParameterTypeStringData,

	/*
	 A division between grouped parameters
	 */
	TEParameterTypeSeparator
};

typedef TE_ENUM(TEParameterIntent, int32_t) 
{
	TEParameterIntentNotSpecified,
	TEParameterIntentColorRGBA,
	TEParameterIntentPositionXYZW,
	TEParameterIntentSizeWH,
	TEParameterIntentUVW,
	
	/*
	 Applies to TEParameterTypeString
	 */
	TEParameterIntentFilePath,

	/*
	 Applies to TEParameterTypeString
	 */
	TEParameterIntentDirectoryPath,

	/*
	 Applies to TEParameterTypeBoolean
	 A true value is considered transient, as from a button press
	 */
	TEParameterIntentMomentary
};

typedef TE_ENUM(TEParameterValue, int32_t) 
{
	TEParameterValueMinimum,
	TEParameterValueMaximum,
	TEParameterValueDefault,
	TEParameterValueCurrent
};

typedef TE_ENUM(TETextureType, int32_t) 
{
	TETextureTypeOpenGL,
	TETextureTypeD3D,
	TETextureTypeDXGI,
	TETextureTypeIOSurface
};

#ifdef __APPLE__

/*
	Used by TEIOSurfaceTexture

	Support for other formats may be possible, please ask if you need them.
*/
typedef TE_ENUM(TETextureFormat, int32_t)
{
	TETextureFormatR8,
	TETextureFormatR16,
	TETextureFormatR16F,
	TETextureFormatR32,
	TETextureFormatR32F,
	TETextureFormatRG8,
	TETextureFormatRG16,
	TETextureFormatRG16F,
	TETextureFormatRG32,
	TETextureFormatRG32F,
	TETextureFormatRGB8,
	TETextureFormatRGB16,
	TETextureFormatRGB16F,
	TETextureFormatRGB32,
	TETextureFormatRGB32F,
	TETextureFormatRGB10_A2,
	TETextureFormatRGBA8,
	TETextureFormatSRGB8,
	TETextureFormatSRGBA8,
	TETextureFormatRGBA16,
	TETextureFormatRGBA16F,
	TETextureFormatRGBA32,
	TETextureFormatRGBA32F
};

#endif

#endif /* TETypes_h */
