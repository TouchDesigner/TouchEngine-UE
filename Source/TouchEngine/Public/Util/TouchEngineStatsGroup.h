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

#include "CoreMinimal.h"

DECLARE_STATS_GROUP(TEXT("TouchEngine"), STATGROUP_TouchEngine, STATCAT_Advanced)

DECLARE_DWORD_ACCUMULATOR_STAT(TEXT("Export - Texture Pool - Nb Total Textures"), STAT_TE_ExportedTexturePool_NbTexturesTotal, STATGROUP_TouchEngine)
DECLARE_DWORD_ACCUMULATOR_STAT(TEXT("Export - Texture Pool - Nb Textures in Pool"), STAT_TE_ExportedTexturePool_NbTexturesPool, STATGROUP_TouchEngine)

DECLARE_DWORD_ACCUMULATOR_STAT(TEXT("Import - Texture Pool - Nb Textures in Pool"), STAT_TE_ImportedTexturePool_NbTexturesPool, STATGROUP_TouchEngine)
DECLARE_DWORD_ACCUMULATOR_STAT(TEXT("Import - No Texture2d Created for Import"), STAT_TE_Import_NbTexture2dCreated, STATGROUP_TouchEngine)
