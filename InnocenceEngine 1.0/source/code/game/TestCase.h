#pragma once
#include "../core/interface/IGameData.h"
#include "../core/interface/BaseEntity.h"
#include "../core/manager/CoreManager.h"

#include "PlayerCharacter.h"
class TestCase : public IGameData
{
public:
	TestCase();
	~TestCase();

	void setup() override;
	void initialize() override;
	void update() override;
	void shutdown() override;
	void testMath();
	void testMemory();
private:
	BaseActor rootActor;

	PlayerCharacter playCharacter;
};

