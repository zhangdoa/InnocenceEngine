#pragma once
#include "BaseApplication.h"
#include "../manager/CoreManager.h"
#include "../../game/InnocenceGarden.h"

class Win32Application : public BaseApplication
{
public:
	Win32Application() {};
	~Win32Application() {};

	virtual void setup() override;
	virtual void initialize() override;
	virtual void update() override;
	virtual void shutdown() override;

	InnocenceGarden m_gameData;
	IGameData* m_ipGameData;
};

