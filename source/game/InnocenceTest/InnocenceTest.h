#pragma once
#include "../../engine/system/IGameInstance.h"

class InnocenceTest : INNO_IMPLEMENT IGameInstance
{
public:
	InnocenceTest(void);
	~InnocenceTest(void) = default;
	InnocenceTest(const InnocenceTest& rhs) = delete;
	InnocenceTest& operator=(const InnocenceTest& rhs) = delete;
	InnocenceTest(InnocenceTest&& other) = default;
	InnocenceTest& operator=(InnocenceTest&& other) = default;

	INNO_SYSTEM_EXPORT bool setup() override;
	INNO_SYSTEM_EXPORT bool initialize() override;
	INNO_SYSTEM_EXPORT bool update() override;
	INNO_SYSTEM_EXPORT bool terminate() override;

	INNO_SYSTEM_EXPORT objectStatus getStatus() override;

	INNO_SYSTEM_EXPORT std::string getGameName() override;
};