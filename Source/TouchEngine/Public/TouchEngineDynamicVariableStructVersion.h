// Copyright © Derivative Inc. 2022

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
