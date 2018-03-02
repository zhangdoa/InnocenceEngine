#pragma once
#include "BaseManager.h"

#include "LogManager.h"
#include "MemoryManager.hpp"
#include "TaskManager.h"
#include "TimeManager.h"
#include "graphic/RenderingManager.h"
#include "AssetManager.h"

#include "interface/IGame.h"

extern IGame* g_pGame;

class CoreManager : public BaseManager
{
public:
	CoreManager() {};
	~CoreManager() {};
	void setup() override;
	void initialize() override;
	void update() override;
	void shutdown() override;

private:
	std::vector<std::unique_ptr<IManager>> m_childEventManager;
	MemoryManager* m_pMemoryManager;
	LogManager* m_pLogManager;
	TaskManager* m_pTaskManager;
	TimeManager* m_pTimeManager;
	RenderingManager* m_pRenderingManager;
	AssetManager* m_pAssetManager;
};

