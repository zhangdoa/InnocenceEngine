#pragma once
#include "../../engine/common/InnoType.h"
#include "exports/InnoGame_Export.h"
#include "../../engine/common/InnoClassTemplate.h"
#include "../../engine/common/ComponentHeaders.h"

INNO_INTERFACE IGameInstance
{
public:
	INNO_CLASS_INTERFACE_NON_COPYABLE(IGameInstance);

	INNO_GAME_EXPORT virtual bool setup() = 0;
	INNO_GAME_EXPORT virtual bool initialize() = 0;
	INNO_GAME_EXPORT virtual bool update() = 0;
	INNO_GAME_EXPORT virtual bool terminate() = 0;

	INNO_GAME_EXPORT virtual ObjectStatus getStatus() = 0;

	INNO_GAME_EXPORT virtual std::string getGameName() = 0;
};