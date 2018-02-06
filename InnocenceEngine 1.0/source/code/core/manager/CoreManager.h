#pragma once

#include "../interface/IManager.h"

#include "TimeManager.h"
#include "MemoryManager.hpp"
#include "TaskManager.h"
#include "../manager/graphic/RenderingManager.h"
#include "../manager/graphic/SceneGraphManager.h"

#include "LogManager.h"

#include "../interface/IGameData.h"

class CoreManager : public IManager
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

	void setGameData(IGameData* gameData);

private:
	~CoreManager() {};

	std::vector<std::unique_ptr<IManager>> m_childEventManager;
	IGameData* m_gameData;
};

