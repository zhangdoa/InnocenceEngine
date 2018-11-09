#pragma once
#include "../../engine/system/IGameInstance.h"

class InnocenceEditor : INNO_IMPLEMENT IGameInstance
{
public:
	InnocenceEditor(void);
	~InnocenceEditor(void) = default;
	InnocenceEditor(const InnocenceEditor& rhs) = delete;
	InnocenceEditor& operator=(const InnocenceEditor& rhs) = delete;
	InnocenceEditor(InnocenceEditor&& other) = default;
	InnocenceEditor& operator=(InnocenceEditor&& other) = default;

	INNO_SYSTEM_EXPORT bool setup() override;
	INNO_SYSTEM_EXPORT bool initialize() override;
	INNO_SYSTEM_EXPORT bool update() override;
	INNO_SYSTEM_EXPORT bool terminate() override;

	INNO_SYSTEM_EXPORT objectStatus getStatus() override;

	INNO_SYSTEM_EXPORT std::string getGameName() override;
};
