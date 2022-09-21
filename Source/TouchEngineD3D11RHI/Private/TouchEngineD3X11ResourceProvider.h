// Copyright © Derivative Inc. 2022

#pragma once

#include "CoreMinimal.h"

namespace UE::TouchEngine
{
	class FTouchResourceProvider;
	struct FResourceProviderInitArgs;
}

namespace UE::TouchEngine::D3DX11
{
	TSharedPtr<FTouchResourceProvider> MakeD3DX11ResourceProvider(const FResourceProviderInitArgs& InitArgs);
}
