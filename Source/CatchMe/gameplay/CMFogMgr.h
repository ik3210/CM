// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "UObject/NoExportTypes.h"
#include "CMFogMgr.generated.h"

UCLASS(meta=(lua=1))
class CATCHME_API UCMFogMgr : public UObject
{
	GENERATED_BODY()
public:
	UPROPERTY()
		UTexture2D *Tx_Fog;

	TArray<FColor> Data;
	FUpdateTextureRegion2D* textureRegions;
	FColor FogColor;
	UFUNCTION()
	void Init(FColor Color);

	void UpdateTextureRegions(
		UTexture2D* Texture,
		int32 MipIndex,
		uint32 NumRegions,
		FUpdateTextureRegion2D* Regions,
		uint32 SrcPitch,
		uint32 SrcBpp,
		uint8* SrcData,
		bool bFreeData);

	UFUNCTION()
	void UpdateTexture();
	
	UFUNCTION()
	void UpdateFOV(FVector CharacterPos);
};
