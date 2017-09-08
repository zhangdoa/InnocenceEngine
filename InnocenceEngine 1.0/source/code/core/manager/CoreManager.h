#pragma once

#include "IEventManager.h"

#include "TimeManager.h"
#include "../manager/graphic/GLWindowManager.h"
#include "InputManager.h"
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
	GLWindowManager& getWindowManager() const;
	InputManager& getInputManager() const;
	RenderingManager& getRenderingManager() const;
	// SceneGraphManager& getTimeManager() const;
	LogManager& getLogManager() const;

private:
	~CoreManager();

	void init() override;
	void update() override;
	void shutdown() override;

	std::vector<std::unique_ptr<IEventManager>> m_childEventManager;

	IGameData* m_gameData;
};

