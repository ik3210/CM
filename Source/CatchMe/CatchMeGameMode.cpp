// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "CatchMe.h"
#include "CatchMeGameMode.h"
#include "TableUtil.h"

ACatchMeGameMode::ACatchMeGameMode()
{
	LuaCtor("gameplay.gamemode.cmgamemode", this);
}