#pragma once
#include "../../interface/IEventManager.h"
#include "../LogManager.h"
#include "../../interface/IGameEntity.h"

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

	void initialize() override;
	void update() override;
	void shutdown() override;

	BaseActor m_rootActor;
};

