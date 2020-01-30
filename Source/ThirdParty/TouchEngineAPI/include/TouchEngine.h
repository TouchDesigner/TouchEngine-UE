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

#include <TEBase.h>
#include <TETypes.h>
#include <TEResult.h>
#include <TEStructs.h>
#include <TEInstance.h>
#include <TETexture.h>
#include <TEOpenGLTexture.h>
#if defined(_WIN32)
	#include <TED3DTexture.h>
	#include <TEDXGITexture.h>
#endif
#include <TEGraphicsContext.h>
#include <TEAdapter.h>
