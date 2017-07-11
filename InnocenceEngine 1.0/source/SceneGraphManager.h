#pragma once
#include "IEventManager.h"
#include "LogManager.h"
#include "IGameEntity.h"

class SceneGraphManager : public IEventManager
{
public:
	~SceneGraphManager();

	static SceneGraphManager& getInstance()
	{
		static SceneGraphManager instance;
		return instance;
	}

private:
	SceneGraphManager();

	void init() override;
	void update() override;
	void shutdown() override;

	BaseActor m_rootActor;
};

