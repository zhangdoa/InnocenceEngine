#pragma once
#include "IGameInstance.h"

class GameInstance : public IGameInstance
{
public:
	GameInstance(void) = default;
	~GameInstance(void) = default;
	GameInstance(const GameInstance& rhs) = delete;
	GameInstance& operator=(const GameInstance& rhs) = delete;
	GameInstance(GameInstance&& other) = default;
	GameInstance& operator=(GameInstance&& other) = default;

	bool setup() override;
	bool initialize() override;
	bool update() override;
	bool terminate() override;

	ObjectStatus getStatus() override;

	std::string getGameName() override;
};
