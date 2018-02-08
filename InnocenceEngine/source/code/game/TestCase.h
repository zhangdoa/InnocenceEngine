#pragma once
#include "../engine/interface/IGameData.h"
#include "../engine/interface/BaseEntity.h"
#include "../engine/manager/CoreManager.h"

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

