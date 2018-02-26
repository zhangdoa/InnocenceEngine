#pragma once
#include "common/stdafx.h"
#include "common/BaseGame.hpp"
#include "entity/BaseEntity.h"
#include "manager/CoreManager.h"

#include "PlayerCharacter.h"
class TestCase : public BaseGame
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

