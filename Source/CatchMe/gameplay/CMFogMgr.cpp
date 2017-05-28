// Fill out your copyright notice in the Description page of Project Settings.

#include "CatchMe.h"
#include "CMFogMgr.h"
#include "TableUtil.h"
#include "catchmeplayercontroller.h"
UCMFogMgr::UCMFogMgr()
{
	LuaCtor("gameplay.cmfogmgr", this);
}

void UCMFogMgr::Init(FColor Color)
{
	int32 ArrSize = MapSize*MapSize;
	Data.Init(Color, ArrSize);
	FogColor = Color;
	Tx_Fog = UTexture2D::CreateTransient(MapSize, MapSize);
	Tx_Last_Fog = UTexture2D::CreateTransient(MapSize, MapSize);
	textureRegions = new FUpdateTextureRegion2D(0, 0, 0, 0, MapSize, MapSize);
	LasttextureRegions = new FUpdateTextureRegion2D(0, 0, 0, 0, MapSize, MapSize);
	UpdateTexture();
	UpdateLastTexture();
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

bool UCMFogMgr::CanSee(FVector& Pos, int32 Targetx, int32 Targety)
{
// 	return LuaCallr(bool,"TestXY",this, Pos, Targetx, Targety);
// 	return true;
	FVector TargetPos(Targety, Targetx, Pos.Z);
	FHitResult Hit;
	TArray<AActor*> Ignore;
	Ignore.Add(Cast<ACatchMePlayerController>(Controller)->PlayCharacter);
	if (UKismetSystemLibrary::LineTraceSingle_NEW(Controller, Pos, TargetPos, ETraceTypeQuery::TraceTypeQuery1, true, Ignore, EDrawDebugTrace::None, Hit, true))
		return false;
	else
		return true;
}

void UCMFogMgr::UpdateTexture()
{
	Tx_Fog->UpdateResource();
	UpdateTextureRegions(Tx_Fog, (int32)0, (uint32)1, textureRegions, (uint32)(4 * MapSize), (uint32)4, (uint8*)Data.GetData(), false);
}

void UCMFogMgr::UpdateLastTexture()
{
	Tx_Last_Fog->UpdateResource();
	UpdateTextureRegions(Tx_Last_Fog, (int32)0, (uint32)1, LasttextureRegions, (uint32)(4 * MapSize), (uint32)4, (uint8*)Data.GetData(), false);
}

void UCMFogMgr::UpdateFOV(FVector CharacterPos)
{
	int32 len = EyeLen;
	int32 Sqrt = len*len;
	int32 FogMapFactor = MapSize / LandscapeSize;
	int32 y = (CharacterPos.X / 100 + LandscapeSize/2)* FogMapFactor;
	int32 x = (CharacterPos.Y / 100 + LandscapeSize/2)* FogMapFactor;

	int32 ArrSize = MapSize*MapSize;
	UpdateLastTexture();
	Data.Init(FogColor, ArrSize);

	for (int32 i = FMath::Max(x - len, 0); i <= FMath::Min(x + len, MapSize -1); i++)
		for (int32 j = FMath::Max(y - len, 0); j <= FMath::Min(y + len, MapSize - 1); j++)
		{
			if ( FMath::Pow((i-x),2) + FMath::Pow((j - y), 2) <= Sqrt)
				if (CanSee(CharacterPos, (float(i)/FogMapFactor - LandscapeSize/2)*100, (float(j) / FogMapFactor - LandscapeSize / 2) * 100))
					Data[i * MapSize + j] = ThroughColor;
		}
	UpdateTexture();
}
