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

#ifndef TEFloatBuffer_h
#define TEFloatBuffer_h

#include <TouchEngine/TEObject.h>
#include <TouchEngine/TEResult.h>

#ifdef __cplusplus
extern "C" {
#endif

TE_ASSUME_NONNULL_BEGIN

typedef TE_ENUM(TEFloatBufferExtend, int32_t) 
{
	TEFloatBufferExtendHold,
	TEFloatBufferExtendSlope,
	TEFloatBufferExtendCycle,
	TEFloatBufferExtendMirror,
	TEFloatBufferExtendConstant
};

typedef struct TEFloatBuffer_ TEFloatBuffer;

/*
 Creates a TEFloatBuffer instance for values which are not time-dependent.
 
 'rate' is the number of values (or samples) per second, if this applies to the data.
 	Pass -1.0 to indicate no rate applies to the data.
 'channels' is the number of channels
 'capacity' is the maximum number of float values per channel
 'names' is an array of UTF-8 encoded strings denoting names for the channels, or NULL if the channels aren't named.
 	The names are copied into the buffer, and needn't last beyond this function call
 Returns a new TEFloatBuffer instance, or NULL if an error prevented one being created.
	The caller is responsible for releasing the returned TEFloatBuffer using TERelease()
 */
TE_EXPORT TEFloatBuffer *TEFloatBufferCreate(double rate, int32_t channels, uint32_t capacity, const char * TE_NULLABLE const * TE_NULLABLE names);

/*
 Creates a TEFloatBuffer instance for values which are time-dependent.
 
 'rate' is the number of values (or samples) per second
 'channels' is the number of channels
 'capacity' is the maximum number of float values per channel
 'names' is an array of UTF-8 encoded strings denoting names for the channels, or NULL if the channels aren't named
 	The names are copied into the buffer, and needn't last beyond this function call
 Returns a new TEFloatBuffer instance, or NULL if an error prevented one being created.
	The caller is responsible for releasing the returned TEFloatBuffer using TERelease()
 */
TE_EXPORT TEFloatBuffer *TEFloatBufferCreateTimeDependent(double rate, int32_t channels, uint32_t capacity, const char * TE_NULLABLE const * TE_NULLABLE names);

/*
 Creates a new TEFloatBuffer instance sharing the same properties as an existing one.

 'data' is an existing TEFloatBuffer instance
 Returns a new TEFloatBuffer instance, or NULL if an error prevented one being created.
 	The caller is responsible for releasing the returned TEFloatBuffer using TERelease()
 */
TE_EXPORT TEFloatBuffer *TEFloatBufferCreateCopy(const TEFloatBuffer *buffer);

/*
 Sets the float values of a buffer, copying the values into the instance.

 'values' is an array of pointers to float values to be set, one pointer per channel
 'count' is the number of values present per channel, and must not exceed the buffer's capacity
 */
TE_EXPORT TEResult TEFloatBufferSetValues(TEFloatBuffer *buffer, const float * TE_NONNULL * TE_NONNULL values, uint32_t count);

/*
 Sets the time associated with the values of a TEFloatBuffer.

 For time-dependent TEFloatBuffer, this is the time of the first value (or sample) present, expressed
 in the buffer's sample rate.
 When adding time-dependent TEFloatBuffers to a TEInstance as input, the start time is used to match
 samples to frame start times.
 If this function is not called, the default value is 0
 */
TE_EXPORT TEResult TEFloatBufferSetStartTime(TEFloatBuffer *buffer, int64_t start);

/*
 Access the values in a TEFloatBuffer instance.

 The size of the returned array can be determined with TEFloatBufferGetChannelCount().
 These buffers are read-only: do not use them to change the content of the TEFloatBuffer instance.
 The returned array is invalidated by any subsequent modification of the TEFloatBuffer instance -
 	for example by calls to TEFloatBufferSetValues()
 Returns an array of pointers to float values, one pointer per channel, or NULL on an error. 
 */
TE_EXPORT const float * TE_NONNULL const * TE_NULLABLE TEFloatBufferGetValues(const TEFloatBuffer *buffer);

/*
 Returns true if the contained values are time-dependent.
 */
TE_EXPORT bool TEFloatBufferIsTimeDependent(const TEFloatBuffer *buffer);

/*
 Returns the time associated with the contained values. For time-dependent values, this
 is the time of the first value (or sample) present.
 */
TE_EXPORT int64_t TEFloatBufferGetStartTime(const TEFloatBuffer *buffer);

/*
 Returns the time associated with the last contained value (or sample). This value only has
 meaning for time-dependent values.
 */
TE_EXPORT int64_t TEFloatBufferGetEndTime(const TEFloatBuffer *buffer);

/*
 Returns the maximum number of values the TEFloatBuffer instance can contain per channel.
 */
TE_EXPORT uint32_t TEFloatBufferGetCapacity(const TEFloatBuffer *buffer);

/*
 Returns the sample rate of the TEFloatBuffer instance, where applicable.
 */
TE_EXPORT double TEFloatBufferGetRate(const TEFloatBuffer *buffer);

/*
 Returns the number of channels.
 */
TE_EXPORT int32_t TEFloatBufferGetChannelCount(const TEFloatBuffer *buffer);

/*
 Returns the number of values currently in the TEFloatBuffer instance.
 */
TE_EXPORT uint32_t TEFloatBufferGetValueCount(const TEFloatBuffer *buffer);

/*
 Returns an array of UTF-8 encoded strings, one per channel, denoting the names of the channels,
 or NULL if no names are set.
 */
TE_EXPORT const char * TE_NULLABLE const * TE_NULLABLE TEFloatBufferGetChannelNames(const TEFloatBuffer *buffer);

/*
 Returns the behaviour that should be applied to determine values coming before those in the TEFloatBuffer instance.
 */
TE_EXPORT TEFloatBufferExtend TEFloatBufferGetExtendBefore(const TEFloatBuffer *buffer);

/*
 Returns the behaviour that should be applied to determine values coming after those in the TEFloatBuffer instance.
 */
TE_EXPORT TEFloatBufferExtend TEFloatBufferGetExtendAfter(const TEFloatBuffer *buffer);

/*
 Where TEFloatBufferExtend indicates TEFloatBufferExtendConstant, this returns the constant value to be applied.
 */
TE_EXPORT float TEFloatBufferGetExtendConstantValue(const TEFloatBuffer *buffer);

/*
 Sets the behaviour to be applied to determine values coming before or after those in the TEFloatBuffer instance.

 If this function is never called for an instance, the default behaviour is TEFloatBufferExtendHold.
 'constant' is the value to be applied in the case of TEFloatBufferExtendConstant
 */
TE_EXPORT void TEFloatBufferSetExtend(TEFloatBuffer *buffer, TEFloatBufferExtend before, TEFloatBufferExtend after, float constant);

TE_ASSUME_NONNULL_END

#ifdef __cplusplus
}
#endif

#endif
