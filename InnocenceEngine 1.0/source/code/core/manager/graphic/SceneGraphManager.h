#pragma once
#include "../../interface/IEventManager.h"
#include "../LogManager.h"
#include "../../interface/IGameEntity.h"
#include "../../component/VisibleComponent.h"
#include "../../component/LightComponent.h"

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
	void addToLightQueue(LightComponent* lightComponent);

	std::vector<VisibleComponent*>& getRenderingQueue();
	std::vector<LightComponent*>& getLightQueue();

private:
	SceneGraphManager();

	void initialize() override;
	void update() override;
	void shutdown() override;

	std::vector<VisibleComponent*> m_visibleComponents;
	std::vector<LightComponent*> m_LightComponents;
};

