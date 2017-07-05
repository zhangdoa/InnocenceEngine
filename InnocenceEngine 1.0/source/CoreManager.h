#pragma once
#include "IEventManager.h"

#include "TimeManager.h"
#include "GraphicManager.h"


class CoreManager : public IEventManager
{
public:
	CoreManager();
	~CoreManager();

private:
	void init() override;
	void update() override;
	void shutdown() override;
	
	std::vector<std::auto_ptr<IEventManager>> m_childEventManager;

	TimeManager m_timeManager;
	nmsp_GraphicManager::GraphicManager m_graphicManager;
};

