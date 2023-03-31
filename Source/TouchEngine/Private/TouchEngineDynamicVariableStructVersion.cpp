// Copyright © Derivative Inc. 2022


#include "TouchEngineDynamicVariableStructVersion.h"
#include "Serialization/CustomVersion.h"

const FGuid FTouchEngineDynamicVariableStructVersion::GUID(0x42C318CE, 0xCE1911ED, 0xB79F8F5A, 0x0ED0B06D);

// Register the custom version with core
FCustomVersionRegistration GRegisterRemoteControlCustomVersion(FTouchEngineDynamicVariableStructVersion::GUID, FTouchEngineDynamicVariableStructVersion::LatestVersion, TEXT("TouchEngineDynamicVariableStructVer"));