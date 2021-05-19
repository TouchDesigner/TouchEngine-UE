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


#ifndef TEInstance_h
#define TEInstance_h

#include <TouchEngine/TEObject.h>
#include <TouchEngine/TEResult.h>
#include <TouchEngine/TETexture.h>
#include <stdint.h>
#include <stddef.h>
#include <assert.h>

#ifdef __cplusplus
extern "C" {
#endif

TE_ASSUME_NONNULL_BEGIN

typedef TE_ENUM(TEEvent, int32_t) 
{
	/*
	An error not associated with any other action has occurred.
	*/
	TEEventGeneral,

	/*
	Loading an instance has completed. When this is received, all links
	will have been added to the instance. You can begin requesting frames from the
	instance.
	*/
	TEEventInstanceDidLoad,

	/*
	A frequested frame has finished or been cancelled.
	*/
	TEEventFrameDidFinish
};

typedef TE_ENUM(TELinkEvent, int32_t)
{
	/*
	A link was added to the instance.
	*/
	TELinkEventAdded,

	/*
	A link was removed from the instance.
	*/
	TELinkEventRemoved,

	/*
	An existing link was modified. The modification could affect:
	- entries in the associated TELinkInfo
	- minimum, maximum or default values
	- choice names or labels
	*/
	TELinkEventModified,

	/*
	An existing link was moved. The move could be:
	- a change of parent container
	- a change of order in the parent container
	*/
	TELinkEventMoved,

	/*
	An existing link changed state.
	See TEInstanceLinkGetState().
	*/
	TELinkEventStateChange,

	/*
	The number or order of a link's children changed.
	*/
	TELinkEventChildChange,

	/*
	The link's current value changed.
	*/
	TELinkEventValueChange,
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

typedef TE_ENUM(TELinkType, int32_t) 
{
	/*
	 Multiple linked collected according to user preference
	 */
	TELinkTypeGroup,

	/*
	 Multiple links which collectively form a single complex link
	 */
	TELinkTypeComplex,

	/*
	 bool
	*/
	TELinkTypeBoolean,

	/*
	 double
	 */
	TELinkTypeDouble,

	/*
	 int32_t
	 */
	TELinkTypeInt,

	/*
	 UTF-8 char * (input)
	 TEString * (output)
	 */
	TELinkTypeString,


	/*
	 TETexture *
	 */
	TELinkTypeTexture,

	/*
	 planar float sample buffers
	 */
	TELinkTypeFloatBuffer,

	/*
	 TETable *
	  or
	 UTF-8 char * (input)
	 TEString * (output)
	 */
	TELinkTypeStringData,

	/*
	 A division between grouped links
	 */
	TELinkTypeSeparator
};

typedef TE_ENUM(TELinkIntent, int32_t) 
{
	TELinkIntentNotSpecified,
	TELinkIntentColorRGBA,
	TELinkIntentPositionXYZW,
	TELinkIntentSizeWH,
	TELinkIntentUVW,
	
	/*
	 Applies to TELinkTypeString
	 */
	TELinkIntentFilePath,

	/*
	 Applies to TELinkTypeString
	 */
	TELinkIntentDirectoryPath,

	/*
	 Applies to TELinkTypeBoolean
	 A true value is considered transient but may have duration, as from a button held down
	 */
	TELinkIntentMomentary,

	/*
	 Applies to TELinkTypeBoolean
	 A true value is considered transient, as from a single button press
	 */
	TELinkIntentPulse
};

typedef TE_ENUM(TELinkValue, int32_t) 
{
	TELinkValueMinimum,
	TELinkValueMaximum,
	TELinkValueDefault,
	TELinkValueCurrent
};

typedef TE_ENUM(TELinkDomain, int32_t)
{
	/*
	 The TouchEngine link has no one-to-one relationship with a TouchDesigner entity.
	*/
	TELinkDomainNone,

	/*
	 The TouchEngine link represents a TouchDesigner parameter.
	 */
	TELinkDomainParameter,

	/*
	 The TouchEngine link represents a TouchDesigner parameter page.
	 */
	TELinkDomainParameterPage,

	/*
	 The TouchEngine link represents a TouchDesigner operator.
	 */
	TELinkDomainOperator,
};

typedef struct TEInstance_ TEInstance;
typedef struct TEAdapter_ TEAdapter;
typedef TEObject TEGraphicsContext;
typedef struct TETable_ TETable;
typedef struct TEFloatBuffer_ TEFloatBuffer;

struct TELinkInfo
{
	/*
	 The scope (input or output) of the link.
	 */
	TEScope				scope;

	/*
	 How the link is intended to be used.
	 */
	TELinkIntent	intent;

	/*
	 The type of link
	 */
	TELinkType		type;

	/*
	 The domain of the link. The domain describes the link's relationship
	 to a type of entity within TouchDesigner.
	 */
	TELinkDomain	domain;

	/*
	 For value links, the number of values associated with the link
	 eg a colour may have four values for red, green, blue and alpha.

	 For group or complex links, the number of children.
	 */
	int32_t				count;

	/*
	 The human readable label for the link.
	 This may not be unique.
	 */
	const char *		label;

	/*
	 The human readable name for the link. When present, the name is a way
	 for the user to uniquely reference a link within its domain: no two links
	 in the same domain will have the same name.
	 */
	const char *		name;

	/*
	 A unique identifier for the link. If the underlying file is unchanged this
	 will persist through instantiations and will be the same for any given link
	 in multiple instances of the same file.
	 */
	const char *		identifier;
};

struct TELinkState
{
	bool enabled;
	bool editable;
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
 This callback is used to signal changes to links.
 Note callbacks may be invoked from any thread.
*/
typedef void (*TEInstanceLinkCallback)(TEInstance *instance, TELinkEvent event, const char *identifier, void *info);

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
									TEInstanceLinkCallback link_callback,
									void * TE_NULLABLE callback_info,
									TEInstance * TE_NULLABLE * TE_NONNULL instance);

/*
 Loads a .tox file.
 Any currently loaded instance will be unloaded.
 The instance is loaded and put into a suspended state. During loading, your TEInstanceLinkCallback
 may receive events as links are added to the instance. Once loading is complete, your
 TEInstanceEventCallback will receive TEEventInstanceDidLoad and a TEResult indicating success or
 failure.
 Once loading is complete, call TEInstanceResume() when you are ready to allow rendering.

 'path' is a UTF-8 encoded string. The file is loaded asynchronously after this function returns.
 'mode' see TETimeMode above.
 */
TE_EXPORT TEResult TEInstanceLoad(TEInstance *instance, const char *path, TETimeMode mode);

/*
 Any in-progress frame is cancelled.
 Any currently loaded instance is unloaded.
 During unload your TEInstanceLinkCallback will receive events for any links as they are
 removed from the instance.
 Once this function returns you will receive no further callbacks until another file is loaded.
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

 This is an indication of the rate the instance can expect to be rendered at, in frames per second,
 expressed as a rational number.
 Some possible values for numerator/denominator for 60 FPS are 60/1 or 60000/1000.
 See also TEInstanceSetFloatFrameRate().
 'numerator' is the numerator of the rate expressed as a rational number.
 'denominator' is the denominator of the rate expressed as a rational number.
 */
TE_EXPORT TEResult TEInstanceSetFrameRate(TEInstance *instance, int64_t numerator, int32_t denominator);

/*
 Sets a frame-rate for an instance, expressed as a floating-point number.
 This is an indication of the rate the instance can expect to be rendered at, in frames per second.
 */
TE_EXPORT TEResult TEInstanceSetFloatFrameRate(TEInstance *instance, float rate);

/*
 Gets the frame-rate for an instance
 'numerator' and 'denominator' are filled out when the function completes.
 */
TE_EXPORT TEResult TEInstanceGetFrameRate(TEInstance *instance, int64_t *numerator, int32_t *denominator);

/*
 Gets the frame-rate for an instance, expressed as a floating-point number.
 'rate' is filled out when the function completes.
 */
TE_EXPORT TEResult TEInstanceGetFloatFrameRate(TEInstance* instance, float* rate);
/*
 Rendering
 */

/*
 Initiates rendering of a frame. 
 'time_value' and 'time_scale' are ignored for TETimeInternal unless 'discontinuity' is true
 	Excessive use of this method to set times on an instance in TETimeInternal mode will degrade performance.
 'discontinuity' if true indicates the frame does not follow naturally from the previously requested frame
 The frame is rendered asynchronously after this function returns.
 TEInstanceLinkCallback is called for any outputs affected by the rendered frame.
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
 Link Layout
 */

/*
 On return 'children' is a list of link identifiers for the children of the parent link denoted by 'identifier'.
 If 'identifier' is NULL or an empty string, the top level links are returned.
 'identifier' should denote a link of type TELinkTypeGroup or TELinkTypeComplex.
 The caller is responsible for releasing the returned TEStringArray using TERelease().
 */
TE_EXPORT TEResult TEInstanceLinkGetChildren(TEInstance *instance, const char * TE_NULLABLE identifier, struct TEStringArray * TE_NULLABLE * TE_NONNULL children);

/*
 On return 'string' is the link identifier for the TELinkTypeGroup or TELinkTypeComplex which contains the
	link denoted by 'identifier', or an empty string if 'identifier' denotes a top level link.
 The caller is responsible for releasing the returned TEString using TERelease(). 
 */
TE_EXPORT TEResult TEInstanceLinkGetParent(TEInstance *instance, const char * TE_NULLABLE identifier, struct TEString * TE_NULLABLE * TE_NONNULL string);

/*
 On return 'groups' is a list of link identifiers for the top level links in the given scope.
 The caller is responsible for releasing the returned TEStringArray using TERelease().
 */
TE_EXPORT TEResult TEInstanceGetLinkGroups(TEInstance *instance, TEScope scope, struct TEStringArray * TE_NULLABLE * TE_NONNULL groups);

/*
 Link Basics
 */

/*
 On return 'info' describes the link denoted by 'identifier'.
 The caller is responsible for releasing the returned TELinkInfo using TERelease().
 */
TE_EXPORT TEResult TEInstanceLinkGetInfo(TEInstance *instance, const char *identifier, struct TELinkInfo * TE_NULLABLE * TE_NONNULL info);

/*
 On return 'state' describes the state of the link denoted by 'identifier'.
 The caller is responsible for releasing the returned TELinkState using TERelease().
 */
TE_EXPORT TEResult TEInstanceLinkGetState(TEInstance *instance, const char *identifier, struct TELinkState * TE_NULLABLE * TE_NONNULL state);

/*
 On return 'labels' is a list of labels suitable for presentation to the user as options for choosing a value for the link denoted by 'identifier'.
 If 'identifier' does not offer a list of options then 'labels' will be set to NULL.
 Only TELinkTypeInt and TELinkTypeString may have a list of choices. For TELinkTypeInt, the corresponding value is the index of the label
 in the list. For TELinkTypeString, the corresponding value is the entry at the same index in the list returned by TEInstanceLinkGetChoiceValues().
 The caller is responsible for releasing the returned TEStringArray using TERelease().
*/
TE_EXPORT TEResult TEInstanceLinkGetChoiceLabels(TEInstance *instance, const char *identifier, struct TEStringArray * TE_NULLABLE * TE_NONNULL labels);

/*
 On return 'values' is a list of values which may be set on the link denoted by 'identifier'. Each value will have a corresponding label for
 presentation in UI (see TEInstanceLinkGetChoiceLabels()).
 If 'identifier' does not offer a list of value options then 'values' will be set to NULL.
 Only TELinkTypeString may have a list of value options. This list should not be considered exhaustive and users should be allowed to enter their own
 values as well as those in this list.
 The caller is responsible for releasing the returned TEStringArray using TERelease().
*/
TE_EXPORT TEResult TEInstanceLinkGetChoiceValues(TEInstance *instance, const char *identifier, struct TEStringArray * TE_NULLABLE * TE_NONNULL values);

/*
 Getting Link Values
 */

TE_EXPORT TEResult TEInstanceLinkGetBooleanValue(TEInstance *instance, const char *identifier, TELinkValue which, bool *value);

TE_EXPORT TEResult TEInstanceLinkGetDoubleValue(TEInstance *instance, const char *identifier, TELinkValue which, double *value, int32_t count);

TE_EXPORT TEResult TEInstanceLinkGetIntValue(TEInstance *instance, const char *identifier, TELinkValue which, int32_t *value, int32_t count);

/*
 The caller is responsible for releasing the returned TEString using TERelease().
 */
TE_EXPORT TEResult TEInstanceLinkGetStringValue(TEInstance *instance, const char *identifier, TELinkValue which, struct TEString * TE_NULLABLE * TE_NONNULL string);

/*
 On successful completion 'value' is set to a TETexture or NULL if no value is set.
 A TEGraphicsContext can be used to convert the returned texture to any desired type.
 The caller is responsible for releasing the returned TETexture using TERelease()
 */
#if defined(__APPLE__)
TE_EXPORT TEResult TEInstanceLinkGetTextureValue(TEInstance *instance, const char *identifier, TELinkValue which, TEIOSurfaceTexture * TE_NULLABLE * TE_NONNULL value);
#else
TE_EXPORT TEResult TEInstanceLinkGetTextureValue(TEInstance *instance, const char *identifier, TELinkValue which, TEDXGITexture * TE_NULLABLE * TE_NONNULL value);
#endif
/*
 On successful completion 'value' is set to a TETable or NULL if no value is set.
 The caller is responsible for releasing the returned TETable using TERelease()
*/
TE_EXPORT TEResult TEInstanceLinkGetTableValue(TEInstance *instance, const char *identifier, TELinkValue which, TETable * TE_NULLABLE * TE_NONNULL value);

/*
 On successful completion 'value' is set to a TEFloatBuffer or NULL if no value is set.
 The caller is responsible for releasing the returned TEFloatBuffer using TERelease()
*/
TE_EXPORT TEResult TEInstanceLinkGetFloatBufferValue(TEInstance *instance, const char *identifier, TELinkValue which, TEFloatBuffer * TE_NULLABLE * TE_NONNULL value);

/*
 On successful completion 'value' is set to a TEObject or NULL if no value is set.
 An error will be returned if the link cannot be represented as a TEObject.
 The type of the returned object can be determined using TEGetType().
 The caller is responsible for releasing the returned TEObject using TERelease()
*/
TE_EXPORT TEResult TEInstanceLinkGetObjectValue(TEInstance *instance, const char *identifier, TELinkValue which, TEObject * TE_NULLABLE * TE_NONNULL value);


/*
 Setting Input Link Values
 */

TE_EXPORT TEResult TEInstanceLinkSetBooleanValue(TEInstance *instance, const char *identifier, bool value);

TE_EXPORT TEResult TEInstanceLinkSetDoubleValue(TEInstance *instance, const char *identifier, const double *value, int32_t count);

TE_EXPORT TEResult TEInstanceLinkSetIntValue(TEInstance *instance, const char *identifier, const int32_t *value, int32_t count);

/*
 Sets the value of a string input link.
 'value' is a UTF-8 encoded string
 */
TE_EXPORT TEResult TEInstanceLinkSetStringValue(TEInstance *instance, const char *identifier, const char * TE_NULLABLE value);

/*
 Sets the value of a texture input link
 'texture' may be retained by the instance
 'context' is a valid TEGraphicsContext of a type suitable for working with the provided texture.
	NULL may be passed ONLY if 'texture' is of type TETextureTypeDXGI or TETextureTypeIOSurface
	Work may be done in the provided graphics context by this call.
 	An OpenGL context may change the current framebuffer and GL_TEXTURE_2D bindings during this call.
	This may be a different context than any previously passed to TEInstanceAssociateGraphicsContext().
 */
TE_EXPORT TEResult TEInstanceLinkSetTextureValue(TEInstance *instance, const char *identifier, const TETexture *TE_NULLABLE texture, TEGraphicsContext * TE_NULLABLE context);

/*
 Sets the value of a float buffer input link, replacing any previously set value.
 See also TEInstanceLinkAddFloatBuffer().
 This call is required even if a single buffer is used but modified between frames.
 'buffer' may be retained by the instance.
 */
TE_EXPORT TEResult TEInstanceLinkSetFloatBufferValue(TEInstance *instance, const char *identifier, const TEFloatBuffer * TE_NULLABLE buffer);

/*
 Appends a float buffer for an input link receiving time-dependent values.
 The start time of the TEFloatBuffer is used to match samples to frames using the time passed to
 	TEInstanceStartFrameAtTime().
 'buffer' must be a time-dependent TEFloatBuffer, and may be retained by the instance.
 */
TE_EXPORT TEResult TEInstanceLinkAddFloatBuffer(TEInstance *instance, const char *identifier, const TEFloatBuffer * TE_NULLABLE buffer);

/*
 Sets the value of a table input link.

 This call is required even if a single table is used but modified between frames.
 'table' may be retained by the instance.
 */
TE_EXPORT TEResult TEInstanceLinkSetTableValue(TEInstance *instance, const char *identifier, const TETable * TE_NULLABLE table);

/*
 Sets the value of an input link.

 An error will be returned if the link cannot be set using the provided TEObject.
 'value' may be retained by the instance
 */
TE_EXPORT TEResult TEInstanceLinkSetObjectValue(TEInstance *instance, const char *identifier, const TEObject * TE_NULLABLE object);

#define kStructAlignmentError "struct misaligned for library"

static_assert(offsetof(struct TELinkInfo, intent) == 4, kStructAlignmentError);
static_assert(offsetof(struct TELinkInfo, type) == 8, kStructAlignmentError);
static_assert(offsetof(struct TELinkInfo, count) == 16, kStructAlignmentError);
static_assert(offsetof(struct TELinkInfo, label) == 24, kStructAlignmentError);
static_assert(offsetof(struct TELinkInfo, identifier) == 40, kStructAlignmentError);
static_assert(offsetof(struct TEStringArray, strings) == 8, kStructAlignmentError);

TE_ASSUME_NONNULL_END

#ifdef __cplusplus
}
#endif

#endif /* TEInstance_h */
