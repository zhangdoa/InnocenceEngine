#pragma once
#include "common/stdafx.h"
#include "interface/IGame.h"
#include "entity/BaseEntity.h"
#include "manager/CoreManager.h"

#include "PlayerCharacter.h"
class TestCase : public IGame
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
	BaseEntity rootActor;

	PlayerCharacter playCharacter;
};

