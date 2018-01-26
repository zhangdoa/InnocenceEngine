#pragma once
#include "../core/interface/IGameData.h"
#include "../core/interface/BaseEntity.h"
#include "../core/manager/CoreManager.h"

#include "PlayerCharacter.h"
class MathTest : public IGameData
{
public:
	MathTest();
	~MathTest();

	void setup() override;
	void initialize() override;
	void update() override;
	void shutdown() override;

private:
	BaseActor rootActor;

	PlayerCharacter playCharacter;
};

