// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Engine/GameInstance.h"
#include "CatchMeGameInstance.generated.h"

/**
 * 
 */
UCLASS(meta=(lua=1))
class CATCHME_API UCatchMeGameInstance : public UGameInstance
{
	GENERATED_BODY()
public:
	virtual void Init();
	virtual void Shutdown();
	
	
};
