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

#pragma once

#include "CoreTypes.h"
#include "Misc/Guid.h"

// Custom serialization version for changes to remote control objects
struct FTouchEngineDynamicVariableStructVersion
{
	enum Type
	{
		// Roughly corresponds to 5.1
		BeforeCustomVersionWasAdded = 0,

		// Removed the UObject UTouchEngineCHOP and replaced it with FTouchEngineCHOP 
		RemovedUTouchEngineCHOP,
		
		// -----<new versions can be added above this line>-------------------------------------------------
		VersionPlusOne,
		LatestVersion = VersionPlusOne - 1
	};

	// The GUID for this custom version number
	const static FGuid GUID;

private:
	FTouchEngineDynamicVariableStructVersion() {}
};
