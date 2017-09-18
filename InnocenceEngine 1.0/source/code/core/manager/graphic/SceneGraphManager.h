#pragma once
#include "../../interface/IEventManager.h"
#include "../LogManager.h"
#include "../../interface/IGameEntity.h"
#include "../../component/VisibleComponent.h"
class SceneGraphManager : public IEventManager
{
public:
	~SceneGraphManager();

	static SceneGraphManager& getInstance()
	{
		static SceneGraphManager instance;
		return instance;
	}

	void addToRenderingQueue(VisibleComponent* visibleComponent);
	std::vector<VisibleComponent*>& getRenderingQueue();
private:
	SceneGraphManager();

	void initialize() override;
	void update() override;
	void shutdown() override;

	std::vector<VisibleComponent*> m_visibleComponents;
};

