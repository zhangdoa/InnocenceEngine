#pragma once
#include "../manager/IEventManager.h"
#include "../manager/LogManager.h"
#include "../interface/IGameEntity.h"

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

