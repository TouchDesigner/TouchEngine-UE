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

#ifndef TEInstance_h
#define TEInstance_h

#include "TEObject.h"
#include "TEResult.h"
#include "TETexture.h"
#include <stdint.h>
#include <stddef.h>
#include <assert.h>

#ifdef __cplusplus
extern "C" {
#endif

TE_ASSUME_NONNULL_BEGIN

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
	TEParameterTypeFloatBuffer,

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
	 A true value is considered transient but may have duration, as from a button held down
	 */
	TEParameterIntentMomentary,

	/*
	 Applies to TEParameterTypeBoolean
	 A true value is considered transient, as from a single button press
	 */
	TEParameterIntentPulse
};

typedef TE_ENUM(TEParameterValue, int32_t) 
{
	TEParameterValueMinimum,
	TEParameterValueMaximum,
	TEParameterValueDefault,
	TEParameterValueCurrent
};

typedef TEObject TEInstance;
typedef TEObject TEAdapter;
typedef TEObject TEGraphicsContext;
typedef TEObject TETable;
typedef TEObject TEFloatBuffer;

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
	int32_t										count;

	/*
	 The array of strings, each entry being a null-terminated UTF-8 encoded string
	 */
	const char * TE_NONNULL const * TE_NULLABLE	strings;
};

/*
 This callback is used to signal events related to an instance.
 Note callbacks may be invoked from any thread.
*/
typedef void (*TEInstanceEventCallback)(TEInstance *instance,
										TEEvent event,
										TEResult result,
										int64_t start_time_value,
										int32_t start_time_scale,
										int64_t end_time_value,
										int32_t end_time_scale,
										void * TE_NULLABLE info);

/*
 This callback is used to signal changes to parameter values.
 Note callbacks may be invoked from any thread.
*/
typedef void (*TEInstanceParameterValueCallback)(TEInstance *instance, const char *identifier, void *info);


/*
 On return, extensions is a list of file extensions supported by TEInstanceCreate
 The caller is responsible for releasing the returned TEStringArray using TERelease()
 */
TE_EXPORT TEResult TEInstanceGetSupportedFileExtensions(struct TEStringArray * TE_NULLABLE * TE_NONNULL extensions);

/*
 Creates an instance.
 
 'event_callback' will be called to deliver TEEvents related to loading and rendering the instance.
 'callback_info' will be passed into the callbacks as the 'info' argument.
 'instance' will be set to a TEInstance on return, or NULL if an instance could not be created.
	The caller is responsible for releasing the returned TEInstance using TERelease()
 Returns TEResultSucccess or an error.
 */
TE_EXPORT TEResult TEInstanceCreate(TEInstanceEventCallback event_callback,
									TEInstanceParameterValueCallback prop_value_callback,
									void * TE_NULLABLE callback_info,
									TEInstance * TE_NULLABLE * TE_NONNULL instance);

/*
 Loads a .tox file.
 Any currently loaded instance will be unloaded.
 The instance is loaded and put into a suspended state. Call TEInstanceResume() to allow rendering.

 'path' is a UTF-8 encoded string. The file is loaded asynchronously after this function returns.
 'mode' see TETimeMode above.
 */
TE_EXPORT TEResult TEInstanceLoad(TEInstance *instance, const char *path, TETimeMode mode);

/*
 Any in-progress frame is cancelled.
 Any currently loaded instance is unloaded.
 */
TE_EXPORT TEResult TEInstanceUnload(TEInstance *instance);

/*
 Returns true if a file is waiting to be loaded, being loaded, or has been loaded and not unloaded.
 */
TE_EXPORT bool TEInstanceHasFile(TEInstance *instance);

TE_EXPORT const char *TEInstanceGetPath(TEInstance *instance);

TE_EXPORT TETimeMode TEInstanceGetTimeMode(TEInstance *instance);

/*
 Associates an instance with a graphics context. This optional association permits optimizations when
 working with texture inputs and outputs.
 To most clearly communicate your intentions to the instance, call this prior to the initial call to
	TEInstanceResume(), and subsequently if you require support for a different context.
 One context may be shared between several instances.
 This function will call TEInstanceAssociateAdapter on your behalf.
 */
TE_EXPORT TEResult TEInstanceAssociateGraphicsContext(TEInstance *instance, TEGraphicsContext *context);

/*
 Associates an instance with an adapter. This optional association may guide the choice of adapter
 used by the instance.
 To most clearly communicate your intentions to the instance, call this prior to the initial call to
	TEInstanceResume(), and subsequently if you require support for a different adapter.
 */
TE_EXPORT TEResult TEInstanceAssociateAdapter(TEInstance *instance, TEAdapter * TE_NULLABLE adapter);

/*
 Resumes an instance
 */
TE_EXPORT TEResult TEInstanceResume(TEInstance *instance);

/*
 Suspends an instance
 Callbacks for any activities started prior to suspension may be received while suspended.
 */
TE_EXPORT TEResult TEInstanceSuspend(TEInstance *instance);

/*
 Sets a frame-rate for an instance.

 This is an indication of the rate the instance can expect to be rendered at, in frames per second.
 'numerator' is the numerator of the rate expressed as a rational number. For example for 60 FPS this value could be 60.
 'denominator' is the denominator of the rate expressed as a rational number. For example for 60 FPS this value could be 1.
 */
TE_EXPORT TEResult TEInstanceSetFrameRate(TEInstance *instance, int64_t numerator, int32_t denominator);

/*
 Gets the frame-rate for an instance
 'numerator' and 'denominator' are filled out when the function completes.
 */
TE_EXPORT TEResult TEInstanceGetFrameRate(TEInstance *instance, int64_t *numerator, int32_t *denominator);

/*
 Rendering
 */

/*
 Initiates rendering of a frame. 
 'time_value' and 'time_scale' are ignored for TETimeInternal unless 'discontinuity' is true
 	Excessive use of this method to set times on an instance in TETimeInternal mode will degrade performance.
 'discontinuity' if true indicates the frame does not follow naturally from the previously requested frame
 The frame is rendered asynchronously after this function returns.
 TEInstanceParameterValueCallback is called for any outputs affected by the rendered frame.
 TEInstanceEventCallback is called with TEEventFrameDidFinish when the frame completes.
 Returns TEResultSuccess or an error.
 	If an error is returned, the frame will not be rendered and the TEInstanceEventCallback will not be invoked.
 */
TE_EXPORT TEResult TEInstanceStartFrameAtTime(TEInstance *instance, int64_t time_value, int32_t time_scale, bool discontinuity);

/*
 Requests the cancellation of an in-progress frame.
 If the frame was successfully cancelled, TEInstanceEventCallback is called with TEEventFrameDidFinish and TEResultCancelled,
 alternatively the frame may complete as usual before the cancellation request is delivered.
*/
TE_EXPORT TEResult TEInstanceCancelFrame(TEInstance *instance);

/*
 Parameter Layout
 */

/*
 On return 'children' is a list of parameter identifiers for the children of the parent parameter denoted by 'identifier'.
 If 'identifier' is NULL or an empty string, the top level parameters are returned.
 'identifier' should denote a parameter of type TEParameterTypeGroup or TEParameterTypeComplex.
 The caller is responsible for releasing the returned TEStringArray using TERelease().
 */
TE_EXPORT TEResult TEInstanceParameterGetChildren(TEInstance *instance, const char * TE_NULLABLE identifier, struct TEStringArray * TE_NULLABLE * TE_NONNULL children);

/*
 On return 'groups' is a list of parameter identifiers for the top level parameters in the given scope.
 The caller is responsible for releasing the returned TEStringArray using TERelease().
 */
TE_EXPORT TEResult TEInstanceGetParameterGroups(TEInstance *instance, TEScope scope, struct TEStringArray * TE_NULLABLE * TE_NONNULL groups);

/*
 Parameter Basics
 */

/*
 On return 'info' describes the parameter denoted by 'identifier'.
 The caller is responsible for releasing the returned TEParameterInfo using TERelease().
 */
TE_EXPORT TEResult TEInstanceParameterGetInfo(TEInstance *instance, const char *identifier, struct TEParameterInfo * TE_NULLABLE * TE_NONNULL info);

/*
 On return 'labels' is a list of labels suitable for presentation to the user as options for choosing a value for the parameter denoted by 'identifier'.
 If 'identifier' does not offer a list of options then 'labels' will be set to NULL.
 Only TEParameterTypeInt and TEParameterTypeString may have a list of choices. For TEParameterTypeInt, the corresponding value is the index of the label
 in the list. For TEParameterTypeString, the corresponding value is the entry at the same index in the list returned by TEInstanceParameterGetChoiceValues().
 The caller is responsible for releasing the returned TEStringArray using TERelease().
*/
TE_EXPORT TEResult TEInstanceParameterGetChoiceLabels(TEInstance *instance, const char *identifier, struct TEStringArray * TE_NULLABLE * TE_NONNULL labels);

/*
 On return 'values' is a list of values which may be set on the parameter denoted by 'identifier'. Each value will have a corresponding label for
 presentation in UI (see TEInstanceParameterGetChoiceLabels()).
 If 'identifier' does not offer a list of value options then 'values' will be set to NULL.
 Only TEParameterTypeString may have a list of value options. This list should not be considered exhaustive and users should be allowed to enter their own
 values as well as those in this list.
 The caller is responsible for releasing the returned TEStringArray using TERelease().
*/
TE_EXPORT TEResult TEInstanceParameterGetChoiceValues(TEInstance *instance, const char *identifier, struct TEStringArray * TE_NULLABLE * TE_NONNULL values);

/*
 Getting Parameter Values
 */

TE_EXPORT TEResult TEInstanceParameterGetBooleanValue(TEInstance *instance, const char *identifier, TEParameterValue which, bool *value);

TE_EXPORT TEResult TEInstanceParameterGetDoubleValue(TEInstance *instance, const char *identifier, TEParameterValue which, double *value, int32_t count);

TE_EXPORT TEResult TEInstanceParameterGetIntValue(TEInstance *instance, const char *identifier, TEParameterValue which, int32_t *value, int32_t count);

/*
 The caller is responsible for releasing the returned TEString using TERelease().
 */
TE_EXPORT TEResult TEInstanceParameterGetStringValue(TEInstance *instance, const char *identifier, TEParameterValue which, struct TEString * TE_NULLABLE * TE_NONNULL string);

/*
 On successful completion 'value' is set to a TETexture or NULL if no value is set.
 A TEGraphicsContext can be used to convert the returned texture to any desired type.
 The caller is responsible for releasing the returned TETexture using TERelease()
 */
TE_EXPORT TEResult TEInstanceParameterGetTextureValue(TEInstance *instance, const char *identifier, TEParameterValue which, TETexture * TE_NULLABLE * TE_NONNULL value);

/*
 On successful completion 'value' is set to a TETable or NULL if no value is set.
 The caller is responsible for releasing the returned TETable using TERelease()
*/
TE_EXPORT TEResult TEInstanceParameterGetTableValue(TEInstance *instance, const char *identifier, TEParameterValue which, TETable * TE_NULLABLE * TE_NONNULL value);

/*
 On successful completion 'value' is set to a TEFloatBuffer or NULL if no value is set.
 The caller is responsible for releasing the returned TEFloatBuffer using TERelease()
*/
TE_EXPORT TEResult TEInstanceParameterGetFloatBufferValue(TEInstance *instance, const char *identifier, TEParameterValue which, TEFloatBuffer * TE_NULLABLE * TE_NONNULL value);

/*
 On successful completion 'value' is set to a TEObject or NULL if no value is set.
 An error will be returned if the parameter cannot be represented as a TEObject.
 The type of the returned object can be determined using TEGetType().
 The caller is responsible for releasing the returned TEObject using TERelease()
*/
TE_EXPORT TEResult TEInstanceParameterGetObjectValue(TEInstance *instance, const char *identifier, TEParameterValue which, TEObject * TE_NULLABLE * TE_NONNULL value);


/*
 Setting Input Parameter Values
 */

TE_EXPORT TEResult TEInstanceParameterSetBooleanValue(TEInstance *instance, const char *identifier, bool value);

TE_EXPORT TEResult TEInstanceParameterSetDoubleValue(TEInstance *instance, const char *identifier, const double *value, int32_t count);

TE_EXPORT TEResult TEInstanceParameterSetIntValue(TEInstance *instance, const char *identifier, const int32_t *value, int32_t count);

/*
 Sets the value of a string input parameter.
 'value' is a UTF-8 encoded string
 */
TE_EXPORT TEResult TEInstanceParameterSetStringValue(TEInstance *instance, const char *identifier, const char * TE_NULLABLE value);

/*
 Sets the value of a texture input parameter
 'texture' may be retained by the instance
 'context' is a valid TEGraphicsContext of a type suitable for working with the provided texture.
	NULL may be passed ONLY if 'texture' is of type TETextureTypeDXGI or TETextureTypeIOSurface
	Work may be done in the provided graphics context by this call.
 	An OpenGL context may change the current framebuffer and GL_TEXTURE_2D bindings during this call.
	This may be a different context than any previously passed to TEInstanceAssociateGraphicsContext().
 */
TE_EXPORT TEResult TEInstanceParameterSetTextureValue(TEInstance *instance, const char *identifier, TETexture *TE_NULLABLE texture, TEGraphicsContext * TE_NULLABLE context);

/*
 Sets the value of a float buffer input parameter, replacing any previously set value.
 See also TEInstanceParameterAddFloatBuffer().
 This call is required even if a single buffer is used but modified between frames.
 'buffer' may be retained by the instance.
 */
TE_EXPORT TEResult TEInstanceParameterSetFloatBufferValue(TEInstance *instance, const char *identifier, TEFloatBuffer * TE_NULLABLE buffer);

/*
 Appends a float buffer for an input parameter receiving time-dependent values.
 The start time of the TEFloatBuffer is used to match samples to frames using the time passed to
 	TEInstanceStartFrameAtTime().
 'buffer' must be a time-dependent TEFloatBuffer, and may be retained by the instance.
 */
TE_EXPORT TEResult TEInstanceParameterAddFloatBuffer(TEInstance *instance, const char *identifier, TEFloatBuffer * TE_NULLABLE buffer);

/*
 Sets the value of a table input parameter.

 This call is required even if a single table is used but modified between frames.
 'table' may be retained by the instance.
 */
TE_EXPORT TEResult TEInstanceParameterSetTableValue(TEInstance *instance, const char *identifier, TETable * TE_NULLABLE table);

/*
 Sets the value of an input parameter.

 An error will be returned if the parameter cannot be set using the provided TEObject.
 'value' may be retained by the instance
 */
TE_EXPORT TEResult TEInstanceParameterSetObjectValue(TEInstance *instance, const char *identifier, TEObject * TE_NULLABLE object);

#define kStructAlignmentError "struct misaligned for library"

static_assert(offsetof(struct TEParameterInfo, intent) == 4, kStructAlignmentError);
static_assert(offsetof(struct TEParameterInfo, type) == 8, kStructAlignmentError);
static_assert(offsetof(struct TEParameterInfo, count) == 12, kStructAlignmentError);
static_assert(offsetof(struct TEParameterInfo, label) == 16, kStructAlignmentError);
static_assert(offsetof(struct TEParameterInfo, identifier) == 24, kStructAlignmentError);
static_assert(offsetof(struct TEStringArray, strings) == 8, kStructAlignmentError);

TE_ASSUME_NONNULL_END

#ifdef __cplusplus
}
#endif

#endif /* TEInstance_h */
