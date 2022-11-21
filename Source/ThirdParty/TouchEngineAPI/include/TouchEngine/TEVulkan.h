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

#ifndef TEVulkan_h
#define TEVulkan_h

#include <TouchEngine/TEObject.h>
#include <TouchEngine/TESemaphore.h>
#include <TouchEngine/TETexture.h>
#include <TouchEngine/TEGraphicsContext.h>
#include <TouchEngine/TEInstance.h>
#include <vulkan/vulkan.h>

#ifdef __cplusplus
extern "C" {
#endif

TE_ASSUME_NONNULL_BEGIN

#ifdef _WIN32

typedef struct TEVulkanSemaphore_ TEVulkanSemaphore;

typedef void (*TEVulkanSemaphoreCallback)(HANDLE semaphore, TEObjectEvent event, void * TE_NULLABLE info);

/*
 Create a semaphore from a handle exported from a Vulkan semaphore
 'type' is the Vulkan semphore type
 'handle' is a HANDLE exported from a Vulkan semaphore.
 	If the handle is a NT handle, it will be duplicated and TouchEngine will manage its lifetime - consequently the value returned from
	TEVulkanSemaphoreGetHandle() will not match this value
 'handleType' is the type of handle
 'callback' will be invoked for object use and lifetime events - see TEObjectEvent in TEObject.h
 'info' will be passed as an argument when 'callback' is invoked
 The caller is responsible for releasing the returned TEVulkanSemaphore using TERelease()
 */
TE_EXPORT TEVulkanSemaphore *TEVulkanSemaphoreCreate(VkSemaphoreType type, HANDLE handle, VkExternalSemaphoreHandleTypeFlagBits handleType, TEVulkanSemaphoreCallback callback, void * TE_NULLABLE info);

/*
 Returns the HANDLE to be imported to instantiate a Vulkan semaphore. The lifetime of this handle is managed by TouchEngine and it
 	should not be passed to a call to CloseHandle()
 */
TE_EXPORT HANDLE TEVulkanSemaphoreGetHandle(TEVulkanSemaphore *semaphore);

/*
 Returns the Vulkan semaphore type
 */
TE_EXPORT VkSemaphoreType TEVulkanSemaphoreGetType(TEVulkanSemaphore *semaphore);

/*
 Returns the handle type
 */
TE_EXPORT VkExternalSemaphoreHandleTypeFlagBits TEVulkanSemaphoreGetHandleType(TEVulkanSemaphore *semaphore);

/*
 Sets 'callback' to be invoked for object use and lifetime events - see TEObjectEvent in TEObject.h.
 This replaces (or cancels) any callback previously set on the TEVulkanSemaphore
 */
TE_EXPORT TEResult TEVulkanSemaphoreSetCallback(TEVulkanSemaphore *semaphore, TEVulkanSemaphoreCallback TE_NULLABLE callback, void * TE_NULLABLE info);

/*
 Vulkan Textures - see TETexture.h for functions common to all textures
 macOS: see TETexture.h for TEIOSurfaceTexture and MoltenVK for vkUseIOSurfaceMVK()/vkGetIOSurfaceMVK()
 		or TEMetal.h for TEMetalTexture and MoltevnVK for vkSetMTLTextureMVK()/vkGetMTLTextureMVK
 			(or VK_EXT_metal_objects)
 */

static const struct VkComponentMapping kTEVkComponentMappingIdentity = {
	VK_COMPONENT_SWIZZLE_IDENTITY,
	VK_COMPONENT_SWIZZLE_IDENTITY,
	VK_COMPONENT_SWIZZLE_IDENTITY,
	VK_COMPONENT_SWIZZLE_IDENTITY
};

typedef struct TEVulkanTexture_ TEVulkanTexture;

typedef void (*TEVulkanTextureCallback)(HANDLE texture, TEObjectEvent event, void * TE_NULLABLE info);

/*
Create a texture from a handle exported from a Vulkan texture
'texture' is the HANDLE for the exportable texture
	If the handle is a NT handle, it will be duplicated and TouchEngine will manage its lifetime - consequently the value returned from
	TEVulkanTextureGetHandle() will not match this value
'type' must be a valid TEVulkanHandleType
'callback' will be invoked for object use and lifetime events - see TEObjectEvent in TEObject.h - if the handle is not an NT handle, the 
	underlying texture should remain valid until receiving TEObjectEventRelease
'origin' is the position in 2D space of the 0th texel of the texture
'map' describes how components are to be mapped when the texture is read. If components are not swizzled, you
	can pass kTEVkComponentMappingIdentity
The caller is responsible for releasing the returned TEVulkanTexture using TERelease()
*/
TE_EXPORT TEVulkanTexture *TEVulkanTextureCreate(HANDLE texture,
	VkExternalMemoryHandleTypeFlagBits handleType,
	VkFormat format,
	int32_t width,
	int32_t height,
	TETextureOrigin origin,
	VkComponentMapping map,
	TEVulkanTextureCallback TE_NULLABLE callback, void * TE_NULLABLE info);

/*
 Returns the HANDLE to be imported to instantiate a Vulkan texture. The lifetime of this handle is managed by TouchEngine and it
 	should not be passed to a call to CloseHandle()
 */
TE_EXPORT HANDLE TEVulkanTextureGetHandle(const TEVulkanTexture *texture);

/*
 Returns the handle type
 */
TE_EXPORT VkExternalMemoryHandleTypeFlagBits TEVulkanTextureGetHandleType(const TEVulkanTexture *texture);

/*
 Returns the format of the texture
 */
TE_EXPORT VkFormat TEVulkanTextureGetFormat(const TEVulkanTexture *texture);

/*
 Returns the VkComponentMapping for the texture
 */
TE_EXPORT VkComponentMapping TEVulkanTextureGetVkComponentMapping(const TEVulkanTexture *texture);

/*
 Returns the width of the texture
 */
TE_EXPORT int TEVulkanTextureGetWidth(const TEVulkanTexture *texture);

/*
 Returns the height of the texture
 */
TE_EXPORT int TEVulkanTextureGetHeight(const TEVulkanTexture *texture);

/*
 Sets 'callback' to be invoked for object use and lifetime events - see TEObjectEvent in TEObject.h.
 This replaces (or cancels) any callback previously set on the TEVulkanTexture.
 */
TE_EXPORT TEResult TEVulkanTextureSetCallback(TEVulkanTexture *texture, TEVulkanTextureCallback TE_NULLABLE callback, void * TE_NULLABLE info);


/*
 Supported Vulkan Texture Formats
 */

/*
 Returns via 'formats' the VkFormat supported by the instance.
 This may change during configuration of an instance, and must be queried after receiving TEEventInstanceReady
 'formats' is an array of VkFormat, or NULL, in which case the value at counts is set to the number of available formats.
 'count' is a pointer to an int32_t which should be set to the number of elements in 'formats'.
 If this function returns TEResultSuccess, 'count' is set to the number of VkFormats filled in 'formats'
 If this function returns TEResultInsufficientMemory, the value at 'count' was too small to return all the formats, and
 	'count' has been set to the number of available formats. Resize 'formats' appropriately and call the function again to
 	retrieve the full array of formats. 
 */
TE_EXPORT TEResult TEInstanceGetSupportedVkFormats(TEInstance *instance, VkFormat formats[TE_NULLABLE], int32_t *count);

#endif // _WIN32

/*
 Vulkan Image Ownership Transfer Operations
 */

/*
 Returns true if the instance requires Vulkan ownership transfer via image memory barriers.
 This may change during configuration of an instance, and must be queried after receiving TEEventInstanceReady
 'instance' is an instance which has previously had a TEVulkanContext associated with it.
 */
TE_EXPORT bool TEInstanceDoesVulkanTextureOwnershipTransfer(TEInstance *instance);

/*
 Returns the VkImageLayout to be used by the caller as the 'newLayout' member of a VkImageMemoryBarrier(2)
	when transferring ownership of a texture to the instance.
 This may change during configuration of an instance, and must be queried after receiving TEEventInstanceReady
 'instance' is an instance which has previously had a TEVulkanContext associated with it.
 */
TE_EXPORT VkImageLayout TEInstanceGetVulkanInputReleaseImageLayout(TEInstance *instance);

/*
 Set the VkImageLayout to be used by the instance as the 'newLayout' member of a VkImageMemoryBarrier(2)
  as part of the release operation after using a texture
 'instance' is an instance which has previously had a TEVulkanContext associated with it.
 */
TE_EXPORT TEResult TEInstanceSetVulkanOutputAcquireImageLayout(TEInstance *instance, VkImageLayout newLayout);

/*
 Provide the instance with values it will need for an acquire operation to transfer ownership to the instance
	prior to using a texture. Note that the texture may not be used, in which case the acquire operation will
	not take place.
 'instance' is an instance which has previously had a TEVulkanContext associated with it
 'texture' is a texture of an appropriate type (TEVulkanTexture or TEIOSurfaceTexture)
 'acquireOldLayout' is the VkImageLayout to be set as the 'oldLayout' member of a VkImageMemoryBarrier(2)
	when completing the ownership transfer
 'acquireNewLayout' is the VkImageLayout to be set as the 'newLayout' member of a VkImageMemoryBarrier(2)
	when completing the ownership transfer
 'semaphore' is a TESemaphore to synchronize the acquire operation
 	The instance will wait for this semaphore prior to using the texture
 	If this semaphore is a Vulkan binary semaphore, the instance will also signal this semaphore after waiting
 'waitValue' is, if appropriate for the semaphore type, a value for the semaphore wait operation
 */
TE_EXPORT TEResult TEInstanceAddVulkanTextureTransfer(TEInstance *instance,
														const TETexture *texture,
														VkImageLayout acquireOldLayout,
														VkImageLayout acquireNewLayout,
														TESemaphore *semaphore,
														uint64_t waitValue);

/*
 Returns true if 'instance' has a pending texture transfer for 'texture'
 */
TE_EXPORT bool TEInstanceHasVulkanTextureTransfer(TEInstance *instance, const TETexture *texture);

/*
 Get the values needed for an acquire operation to transfer ownership from the instance prior to using a texture, if
 such an operation is pending.
 'instance' is an instance which has previously had a TEVulkanContext associated with it
 'texture' is a texture of an appropriate type (TEVulkanTexture or TEIOSurfaceTexture) 
 'acquireOldLayout' is, on successful return, the VkImageLayout to be set as the 'oldLayout' member of a
	VkImageMemoryBarrier(2) when completing the ownership transfer
 'acquireNewLayout' is, on successful return, the VkImageLayout to be set as the 'newLayout' member of a
	VkImageMemoryBarrier(2) when completing the ownership transfer
 'semaphore' is, on successful return, a TESemaphore to synchronize the acquire operation on the GPU
 	The caller must wait for this semaphore prior to using the texture
 	If this semaphore is a Vulkan binary semaphore, the caller must also signal this semaphore after waiting
	The caller is responsible for releasing the returned TESemaphore using TERelease()
 'waitValue' is, on successful return and if appropriate for the semaphore type, a value for the semaphore wait operation
 */
TE_EXPORT TEResult TEInstanceGetVulkanTextureTransfer(TEInstance *instance,
														const TETexture *texture,
														VkImageLayout *acquireOldLayout,
														VkImageLayout *acquireNewLayout,
														TESemaphore * TE_NULLABLE * TE_NONNULL semaphore,
														uint64_t *waitValue);


/*
 Vulkan Graphics Contexts - see TEGraphicsContext.h for functions common to all graphics contexts
 */

typedef struct TEVulkanContext_ TEVulkanContext;

/*
 Creates a TEVulkanContext. Passing a TEVulkanContext to TEInstanceAssociateGraphicsContext() will
 - associate the instance with the same physical device
 - cause the instance to use Vulkan texture transfers when supported by the user's installed version
 	of TouchDesigner
 - on Windows, cause the instance to return TEVulkanTextures for output link values
 */
TE_EXPORT TEResult TEVulkanContextCreate(const uint8_t deviceUUID[TE_NONNULL VK_UUID_SIZE],
											const uint8_t driverUUID[TE_NONNULL VK_UUID_SIZE],
											const uint8_t luid[TE_NONNULL VK_LUID_SIZE],
											bool luidValid,
											TETextureOrigin origin,
											TEVulkanContext * TE_NULLABLE * TE_NONNULL context);

TE_ASSUME_NONNULL_END

#ifdef __cplusplus
}
#endif

#endif // TEVulkan_h
