#pragma once
#include "IEventManager.h"
#include "LogManager.h"
#include "IGameEntity.h"

class SceneGraphManager : public IEventManager
{
public:
	SceneGraphManager();
	~SceneGraphManager();

private:
	Actor m_rootActor;

	void init() override;
	void update() override;
	void shutdown() override;

};

