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

	INNO_SYSTEM_EXPORT bool setup() override;
	INNO_SYSTEM_EXPORT bool initialize() override;
	INNO_SYSTEM_EXPORT bool update() override;
	INNO_SYSTEM_EXPORT bool terminate() override;

	INNO_SYSTEM_EXPORT objectStatus getStatus() override;

	INNO_SYSTEM_EXPORT std::string getGameName() override;
};
