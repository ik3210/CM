// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.
#pragma once
#include "GameFramework/PlayerController.h"
#include "controller/CMPlayerControllerBase.h"
#include "CatchMePlayerController.generated.h"

UCLASS(meta=(lua=1), minimalapi)
class ACatchMePlayerController : public ACMPlayerControllerBase
{
	GENERATED_BODY()

public:
	ACatchMePlayerController();
	
	virtual void PlayerTick(float DeltaTime) override;

	UFUNCTION(reliable, server, WithValidation)
	void S_MoveToLocation(FVector Location);

	virtual void GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const override;


	UPROPERTY(Replicated)
	ACharacter* PlayCharacter;

	UFUNCTION(reliable, server, WithValidation)
	void S_PlaySkill(int32 SkillId, AActor* Target);

	UFUNCTION(reliable, server, WithValidation)
	void S_TapActor(AActor* Target);

	UFUNCTION(reliable, server, WithValidation)
	void S_TapFloor(FVector Pos);
};


