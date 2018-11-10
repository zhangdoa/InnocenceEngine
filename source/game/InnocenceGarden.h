#pragma once
#include "../../engine/system/IGameInstance.h"

class InnocenceGarden : INNO_IMPLEMENT IGameInstance
{
public:
	InnocenceGarden(void);
	~InnocenceGarden(void) = default;
	InnocenceGarden(const InnocenceGarden& rhs) = delete;
	InnocenceGarden& operator=(const InnocenceGarden& rhs) = delete;
	InnocenceGarden(InnocenceGarden&& other) = default;
	InnocenceGarden& operator=(InnocenceGarden&& other) = default;

	INNO_GAME_EXPORT bool setup() override;
	INNO_GAME_EXPORT bool initialize() override;
	INNO_GAME_EXPORT bool update() override;
	INNO_GAME_EXPORT bool terminate() override;

	INNO_GAME_EXPORT objectStatus getStatus() override;

	INNO_GAME_EXPORT std::string getGameName() override;
};
