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

#ifndef TEInstance_h
#define TEInstance_h

#include "TEBase.h"
#include "TETypes.h"
#include "TEStructs.h"
#include "TETexture.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

TE_ASSUME_NONNULL_BEGIN

typedef TEObject TEInstance;
typedef TEObject TEAdapter;
typedef TEObject TEGraphicsContext;


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
 Creates an instance for a .tox file located at 'path'.
 
 'path' is a UTF-8 encoded string. The file is loaded asynchronously after this function returns.
 'mode' see TETimeMode in TETypes.h
 'event_callback' will be called to deliver TEEvents related to loading and rendering the instance.
 'callback_info' will be passed into the callbacks as the 'info' argument.
 'instance' will be set to a TEInstance on return, or NULL if an instance could not be created.
	The caller is responsible for releasing the returned TEInstance using TERelease()
 Returns TEResultSucccess or an error.
 The instance is created in a suspended state. Call TEInstanceResume() to allow loading and rendering.
 */
TE_EXPORT TEResult TEInstanceCreate(const char *path,
									TETimeMode mode,
									TEInstanceEventCallback event_callback,
									TEInstanceParameterValueCallback prop_value_callback,
									void * TE_NULLABLE callback_info,
									TEInstance * TE_NULLABLE * TE_NONNULL instance);

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
 Rendering
 */

/*
 Initiates rendering of a frame. 
 'time_value', 'time_scale' and 'discontinuity' are ignored for TETimeInternal
 'discontinuity' if true indicates the frame does not follow naturally from the previously requested frame
 The frame is rendered asynchronously after this function returns.
 TEInstanceParameterValueCallback is called for any outputs affected by the rendered frame.
 TEInstanceEventCallback is called with TEEventFrameDidFinish when the frame completes.
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
 Stream Parameter Configuration
 A stream is an array of float values that are buffered both on input and output
 from the Engine. They are used to pass data such as motion or audio data
 that needs continuity in it's data.
 */

/*
 On return 'description' describes the stream parameter denoted by 'identifier'.
 The caller is responsible for releasing the returned TEStreamDescription using TERelease().
 */
TE_EXPORT TEResult TEInstanceParameterGetStreamDescription(TEInstance *instance, const char *identifier, struct TEStreamDescription * TE_NULLABLE * TE_NONNULL description);

/*
 Configures the input stream denoted by 'identifier' according to the content of 'description'.
 Input streams must have their description set prior to use.
 Changing the description once rendering has begun may result in dropped samples.
 */
TE_EXPORT TEResult TEInstanceParameterSetInputStreamDescription(TEInstance *instance, const char *identifier, const struct TEStreamDescription *description);

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
 Copies stream samples from an output stream.
 'buffers' is an array of pointers to 'count' buffers where each buffer receives values for one channel
 Prior to calling this function, set 'length' to the capacity (in samples) of a single channel buffer
 On return, 'start' is set to the instance time, expressed in stream sample rate, of the first returned sample,
  and 'length' is set to the actual number of samples copied to each channel buffer
 Calls to this function and TEInstanceParameterAppendStreamValues() can be made at any point after an
  instance has loaded, including during rendering
*/
TE_EXPORT TEResult TEInstanceParameterGetOutputStreamValues(TEInstance *instance,
															const char *identifier,
															float * TE_NONNULL * TE_NONNULL buffers,
															int32_t count,
															int64_t *start,
															int64_t *length);

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
	NULL may be passed ONLY if 'texture' is of type TETextureTypeDXGI
	Work may be done in the provided graphics context by this call.
 	An OpenGL context may change the current framebuffer binding during this call.
	This may be a different context than any previously passed to TEInstanceAssociateGraphicsContext().
 */
TE_EXPORT TEResult TEInstanceParameterSetTextureValue(TEInstance *instance, const char *identifier, TETexture *TE_NULLABLE texture, TEGraphicsContext * TE_NULLABLE context);

/*
 Copies stream samples to an input stream.
 'buffers' is an array of pointers to 'count' buffers where each buffer contains values for one channel
 'start' is the start time of the samples, expressed in stream sample rate
 Prior to calling this function, set 'length' to the size (in samples) of a single channel buffer
 On return, 'length' is set to the actual number of samples copied to each channel buffer
 Calls to this function and TEInstanceParameterGetOutputStreamValues() can be made at any point after an instance has loaded, including during rendering
*/
TE_EXPORT TEResult TEInstanceParameterAppendStreamValues(TEInstance *instance,
															const char *identifier,
															const float * TE_NONNULL * TE_NONNULL buffers,
															int32_t count,
															int64_t start,
															int64_t *length);

TE_ASSUME_NONNULL_END

#ifdef __cplusplus
}
#endif

#endif /* TEInstance_h */
