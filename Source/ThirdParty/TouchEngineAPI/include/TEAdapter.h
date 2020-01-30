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

#ifndef TEAdapter_h
#define TEAdapter_h

#include "TEBase.h"
#include "TETypes.h"

#ifdef __cplusplus
extern "C" {
#endif

TE_ASSUME_NONNULL_BEGIN

typedef TEObject TEAdapter;

#ifdef _WIN32
struct IDXGIDevice;
typedef unsigned int UINT;
#endif

#ifdef _WIN32

/*
 Creates an adapter from a DXGI device.
 
 'device' is a DXGI device. If NULL is passed, an adapter based on system defaults will be created.
 'adapter' will be set to a TEAdapter on return, or NULL if an adapter could not be created.
	The caller is responsible for releasing the returned TEAdapter using TERelease()
 Returns TEResultSucccess or an error
 */
TE_EXPORT TEResult TEAdapterCreateForDevice(IDXGIDevice * TE_NULLABLE device,
											TEAdapter * TE_NULLABLE * TE_NONNULL adapter);


/*
 Creates an adapter for an AMD GPU using the WGL_AMD_gpu_association OpenGL extension.
 
 'gpuID' is the ID of a GPU obtained using the extension.
 'adapter' will be set to a TEAdapter on return, or NULL if an adapter could not be created.
	The caller is responsible for releasing the returned TEAdapter using TERelease()
 Returns TEResultSucccess or an error
 */
TE_EXPORT TEResult TEAdapterCreateForAMDGPUAssociation(UINT gpuID,
														TEAdapter * TE_NULLABLE * TE_NONNULL adapter);

#else

// TODO:

#endif

TE_ASSUME_NONNULL_END

#ifdef __cplusplus
}
#endif

#endif // TEAdapter_h
