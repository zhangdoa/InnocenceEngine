#pragma once
#include "../common/InnoType.h"
#include "../../game/exports/InnoGame_Export.h"
#include "../common/InnoClassTemplate.h"
#include "../common/ComponentHeaders.h"

INNO_INTERFACE IGameInstance
{
public:
	INNO_CLASS_INTERFACE_NON_COPYABLE(IGameInstance);

	INNO_GAME_EXPORT virtual bool setup() = 0;
	INNO_GAME_EXPORT virtual bool initialize() = 0;
	INNO_GAME_EXPORT virtual bool update() = 0;
	INNO_GAME_EXPORT virtual bool terminate() = 0;

	INNO_GAME_EXPORT virtual objectStatus getStatus() = 0;

	INNO_GAME_EXPORT virtual std::string getGameName() = 0;
};
