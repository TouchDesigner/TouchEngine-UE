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


#include "TouchEngineDynamicVariableStructVersion.h"
#include "Serialization/CustomVersion.h"

const FGuid FTouchEngineDynamicVariableStructVersion::GUID(0x42C318CE, 0xCE1911ED, 0xB79F8F5A, 0x0ED0B06D);

// Register the custom version with core
FCustomVersionRegistration GRegisterTouchEngineDynamicVariableStructCustomVersion(FTouchEngineDynamicVariableStructVersion::GUID, FTouchEngineDynamicVariableStructVersion::LatestVersion, TEXT("TouchEngineDynamicVariableStructVer"));