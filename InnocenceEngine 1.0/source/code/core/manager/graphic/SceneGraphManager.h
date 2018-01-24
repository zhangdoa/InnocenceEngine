#pragma once
#include "../../interface/IManager.h"
#include "../LogManager.h"
#include "../../component/VisibleComponent.h"
#include "../../component/LightComponent.h"
#include "../../component/CameraComponent.h"
#include "../../component/InputComponent.h"

class SceneGraphManager : public IManager
{
public:
	~SceneGraphManager() {};

	void setup() override;
	void initialize() override;
	void update() override;
	void shutdown() override;

	static SceneGraphManager& getInstance()
	{
		static SceneGraphManager instance;
		return instance;
	}

	void addToRenderingQueue(VisibleComponent* visibleComponent);
	void addToLightQueue(LightComponent* lightComponent);
	void addToCameraQueue(CameraComponent* cameraComponent);
	void addToInputQueue(InputComponent* inputComponent);

	std::vector<VisibleComponent*>& getRenderingQueue();
	std::vector<LightComponent*>& getLightQueue();
	std::vector<CameraComponent*>& getCameraQueue();
	std::vector<InputComponent*>& getInputQueue();

private:
	SceneGraphManager() {};

	std::vector<VisibleComponent*> m_VisibleComponents;
	std::vector<LightComponent*> m_LightComponents;
	std::vector<CameraComponent*> m_CameraComponents;
	std::vector<InputComponent*> m_InputComponents;
};

