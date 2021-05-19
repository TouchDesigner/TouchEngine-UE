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


#ifndef TEResult_h
#define TEResult_h

#include <TouchEngine/TEBase.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

TE_ASSUME_NONNULL_BEGIN

typedef TE_ENUM(TEResult, int32_t) 
{

	TEResultSuccess,

	TEResultInsufficientMemory,

	TEResultGPUAllocationFailed,

	TEResultTextureFormatNotSupported,

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
	The requested link does not belong to the instance.
	*/
	TEResultNoMatchingLink,

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
	 The version of TouchEngine being used is not compatible with the TouchEngine library.
	 */
	TEResultIncompatibleEngineVersion,

	/*
	The OS denied permission
	*/
	TEResultPermissionDenied,

	/*
	An error with bindings within the file.
	*/
	TEResultBadFileBindings,

	/*
	There warnings or errors within the file, but loading completed.
	*/
	TEResultFileLoadWarnings
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
