// Fill out your copyright notice in the Description page of Project Settings.

#include "CatchMe.h"
#include "CMFogMgr.h"


const int32 Size = 128;

void UCMFogMgr::Init(FColor Color)
{
	int32 ArrSize = Size*Size;
	Data.Init(Color, ArrSize);
	FogColor = Color;
	Tx_Fog = UTexture2D::CreateTransient(Size, Size);
	textureRegions = new FUpdateTextureRegion2D(0, 0, 0, 0, Size, Size);
	UpdateTexture();
}

void UCMFogMgr::UpdateTextureRegions(UTexture2D* Texture, int32 MipIndex, uint32 NumRegions, FUpdateTextureRegion2D* Regions, uint32 SrcPitch, uint32 SrcBpp, uint8* SrcData, bool bFreeData)
{
	if (Texture && Texture->Resource)
	{
		struct FUpdateTextureRegionsData
		{
			FTexture2DResource* Texture2DResource;
			int32 MipIndex;
			uint32 NumRegions;
			FUpdateTextureRegion2D* Regions;
			uint32 SrcPitch;
			uint32 SrcBpp;
			uint8* SrcData;
		};

		FUpdateTextureRegionsData* RegionData = new FUpdateTextureRegionsData;

		RegionData->Texture2DResource = (FTexture2DResource*)Texture->Resource;
		RegionData->MipIndex = MipIndex;
		RegionData->NumRegions = NumRegions;
		RegionData->Regions = Regions;
		RegionData->SrcPitch = SrcPitch;
		RegionData->SrcBpp = SrcBpp;
		RegionData->SrcData = SrcData;

		ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER(
			UpdateTextureRegionsData,
			FUpdateTextureRegionsData*, RegionData, RegionData,
			bool, bFreeData, bFreeData,
			{
				for (uint32 RegionIndex = 0; RegionIndex < RegionData->NumRegions; ++RegionIndex)
				{
					int32 CurrentFirstMip = RegionData->Texture2DResource->GetCurrentFirstMip();
					if (RegionData->MipIndex >= CurrentFirstMip)
					{
						RHIUpdateTexture2D(
							RegionData->Texture2DResource->GetTexture2DRHI(),
							RegionData->MipIndex - CurrentFirstMip,
							RegionData->Regions[RegionIndex],
							RegionData->SrcPitch,
							RegionData->SrcData
							+ RegionData->Regions[RegionIndex].SrcY * RegionData->SrcPitch
							+ RegionData->Regions[RegionIndex].SrcX * RegionData->SrcBpp
						);
					}
				}
		if (bFreeData)
		{
			FMemory::Free(RegionData->Regions);
			FMemory::Free(RegionData->SrcData);
		}
		delete RegionData;
			});
	}
}

void UCMFogMgr::UpdateTexture()
{
	Tx_Fog->UpdateResource();
	UpdateTextureRegions(Tx_Fog, (int32)0, (uint32)1, textureRegions, (uint32)(4 * Size), (uint32)4, (uint8*)Data.GetData(), false);
}

void UCMFogMgr::UpdateFOV(FVector CharacterPos)
{
	const int32 len = 10;
	const int32 Sqrt = len*len;
	int32 y = CharacterPos.X / 100 + 64;
	int32 x = CharacterPos.Y / 100 + 64;

	int32 ArrSize = Size*Size;
	Data.Init(FogColor, ArrSize);

	for (int32 i = FMath::Max(x - len, 0); i <= FMath::Min(x + len, 127); i++)
		for (int32 j = FMath::Max(y - len, 0); j <= FMath::Min(y + len, 127); j++)
		{
			if ( FMath::Pow((i-x),2) + FMath::Pow((j - y), 2) <= Sqrt)
				Data[i * 128 + j] = FColor(0, 0, 0, 0);
		}
	UpdateTexture();
}
