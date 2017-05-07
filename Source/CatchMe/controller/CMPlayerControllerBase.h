// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameFramework/PlayerController.h"
#include "CMPlayerControllerBase.generated.h"

/**
 * 
 */
UCLASS(meta=(lua=1))
class CATCHME_API ACMPlayerControllerBase : public APlayerController
{
	GENERATED_BODY()
public:
	virtual void BeginPlay() override;

	virtual void PlayerTick(float DeltaTime) override;

	/** Method called prior to processing input */
	virtual void PreProcessInput(const float DeltaTime, const bool bGamePaused) override;

	/** Method called after processing input */
	virtual void PostProcessInput(const float DeltaTime, const bool bGamePaused) override;

	/** update input detection */
	virtual void ProcessPlayerInput(const float DeltaTime, const bool bGamePaused) override;

	virtual void SetPawn(APawn* aPawn) override;

	
	UFUNCTION()
	TArray<float> GetInputState();

	UFUNCTION(BlueprintCallable, Category = "PlayerControllerBase")
	bool GetHitResult(float x, float y, FHitResult& HitResult, int TraceChannel = -1, bool bTraceComplex = true);
};
