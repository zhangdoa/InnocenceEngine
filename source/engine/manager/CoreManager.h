#pragma once
#include "BaseManager.h"

#include "TimeManager.h"
#include "LogManager.h"
#include "MemoryManager.hpp"
#include "TaskManager.h"
#include "graphic/RenderingManager.h"
#include "graphic/SceneGraphManager.h"
#include "AssetManager.h"

#include "interface/IGame.h"

extern IGame* g_pGame;

class CoreManager : public BaseManager
{
public:
	CoreManager() {};

	void setup() override;
	void initialize() override;
	void update() override;
	void shutdown() override;

	static CoreManager& getInstance()
	{
		static CoreManager instance;
		return instance;
	}

private:
	~CoreManager() {};

	std::vector<std::unique_ptr<IManager>> m_childEventManager;
};

