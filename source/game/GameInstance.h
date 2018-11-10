#pragma once
#include "../../engine/system/IGameInstance.h"

class GameInstance : INNO_IMPLEMENT IGameInstance
{
public:
	GameInstance(void);
	~GameInstance(void) = default;
	GameInstance(const GameInstance& rhs) = delete;
	GameInstance& operator=(const GameInstance& rhs) = delete;
	GameInstance(GameInstance&& other) = default;
	GameInstance& operator=(GameInstance&& other) = default;

	INNO_GAME_EXPORT bool setup() override;
	INNO_GAME_EXPORT bool initialize() override;
	INNO_GAME_EXPORT bool update() override;
	INNO_GAME_EXPORT bool terminate() override;

	INNO_GAME_EXPORT objectStatus getStatus() override;

	INNO_GAME_EXPORT std::string getGameName() override;
};
