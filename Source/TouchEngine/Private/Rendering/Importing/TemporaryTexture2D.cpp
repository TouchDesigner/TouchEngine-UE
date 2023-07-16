// Copyright © Derivative Inc. 2022


#include "Rendering/Importing/TemporaryTexture2D.h"

// UTemporaryTexture2D* UTemporaryTexture2D::CreateTransient(int32 InSizeX, int32 InSizeY, EPixelFormat InFormat, const FName InName)
// {
// 	// Copy of UTexture2D::CreateTransient, replacing UTexture2D by UTemporaryTexture2D
// 	
// 	LLM_SCOPE(ELLMTag::Textures);
//
// 	UTemporaryTexture2D* NewTexture = nullptr;
// 	if (InSizeX > 0 && InSizeY > 0 &&
// 		(InSizeX % GPixelFormats[InFormat].BlockSizeX) == 0 &&
// 		(InSizeY % GPixelFormats[InFormat].BlockSizeY) == 0)
// 	{
// 		NewTexture = NewObject<UTemporaryTexture2D>(
// 			GetTransientPackage(),
// 			InName,
// 			RF_Transient
// 		);
//
// 		NewTexture->SetPlatformData(new FTexturePlatformData());
// 		NewTexture->GetPlatformData()->SizeX = InSizeX;
// 		NewTexture->GetPlatformData()->SizeY = InSizeY;
// 		NewTexture->GetPlatformData()->SetNumSlices(1);
// 		NewTexture->GetPlatformData()->PixelFormat = InFormat;
//
// 		// Allocate first mipmap.
// 		int32 NumBlocksX = InSizeX / GPixelFormats[InFormat].BlockSizeX;
// 		int32 NumBlocksY = InSizeY / GPixelFormats[InFormat].BlockSizeY;
// 		FTexture2DMipMap* Mip = new FTexture2DMipMap();
// 		NewTexture->GetPlatformData()->Mips.Add(Mip);
// 		Mip->SizeX = InSizeX;
// 		Mip->SizeY = InSizeY;
// 		Mip->SizeZ = 1;
// 		Mip->BulkData.Lock(LOCK_READ_WRITE);
// 		Mip->BulkData.Realloc((int64)NumBlocksX * NumBlocksY * GPixelFormats[InFormat].BlockBytes);
// 		Mip->BulkData.Unlock();
// 	}
// 	else
// 	{
// 		UE_LOG(LogTexture, Warning, TEXT("Invalid parameters specified for UTexture2D::CreateTransient()"));
// 	}
// 	return NewTexture;
// }
//
// void UTemporaryTexture2D::SetIsTemporary(bool InIsTemporary)
// {
// 	if (IsTemporary == InIsTemporary)
// 	{
// 		return;
// 	}
// 	IsTemporary = InIsTemporary;
// 	if (IsTemporary)
// 	{
// 		TickDelegate = FTickerDelegate::CreateUObject(this, &UTemporaryTexture2D::TickToDeath);
// 		TickDelegateHandle = FTSTicker::GetCoreTicker().AddTicker(TickDelegate);
// 	}
// 	else
// 	{
// 		FTSTicker::GetCoreTicker().RemoveTicker(TickDelegateHandle);
// 	}
// }
//
// bool UTemporaryTexture2D::GetIsTemporary() const
// {
// 	return IsTemporary;
// }
//
// bool UTemporaryTexture2D::TickToDeath(float DeltaTime)
// {
// 	FTSTicker::GetCoreTicker().RemoveTicker(TickDelegateHandle);
// 	ReleaseResource();
// 	ConditionalBeginDestroy();
//
// 	return false;
// }
