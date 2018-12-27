#pragma once
#include "IGameInstance.h"

class GameInstance : INNO_IMPLEMENT IGameInstance
{
public:
	GameInstance(void) = default;
	~GameInstance(void) = default;
	GameInstance(const GameInstance& rhs) = delete;
	GameInstance& operator=(const GameInstance& rhs) = delete;
	GameInstance(GameInstance&& other) = default;
	GameInstance& operator=(GameInstance&& other) = default;

	INNO_GAME_EXPORT bool setup() override;
	INNO_GAME_EXPORT bool initialize() override;
	INNO_GAME_EXPORT bool update(bool pause) override;
	INNO_GAME_EXPORT bool terminate() override;

	INNO_GAME_EXPORT ObjectStatus getStatus() override;

	INNO_GAME_EXPORT std::string getGameName() override;
};
