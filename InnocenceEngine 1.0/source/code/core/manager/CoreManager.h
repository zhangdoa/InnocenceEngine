#pragma once

#include "../interface/IEventManager.h"

#include "TimeManager.h"
#include "../manager/graphic/RenderingManager.h"
#include "../manager/graphic/SceneGraphManager.h"
#include "LogManager.h"

#include "../interface/IGameData.h"

class CoreManager : public IEventManager
{
public:
	CoreManager();

	static CoreManager& getInstance()
	{
		static CoreManager instance;
		return instance;
	}

	void setGameData(IGameData* gameData);
	TimeManager& getTimeManager() const;
	RenderingManager& getRenderingManager() const;
	LogManager& getLogManager() const;

private:
	~CoreManager();

	void initialize() override;
	void update() override;
	void shutdown() override;

	std::vector<std::unique_ptr<IEventManager>> m_childEventManager;
	IGameData* m_gameData;
};

