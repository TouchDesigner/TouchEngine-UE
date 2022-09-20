// Copyright © Derivative Inc. 2022

#pragma once

#include "CoreMinimal.h"
#include "TouchEngineResourceProvider.h"

namespace UE::TouchEngine
{
	struct FResourceProviderInitArgs;
}

namespace UE::TouchEngine::D3DX11
{
	TSharedPtr<FTouchEngineResourceProvider> MakeD3DX11ResourceProvider(const FResourceProviderInitArgs& InitArgs);
}
