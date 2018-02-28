#pragma once
#include "manager/BaseManager.h"
#include "manager/LogManager.h"

class SceneGraphManager : public BaseManager
{
public:
	SceneGraphManager() {};
	~SceneGraphManager() {};

	void setup() override;
	void initialize() override;
	void update() override;
	void shutdown() override;

	void addToRenderingQueue(VisibleComponent* visibleComponent);
	void addToLightQueue(LightComponent* lightComponent);
	void addToCameraQueue(CameraComponent* cameraComponent);
	void addToInputQueue(InputComponent* inputComponent);

	std::vector<VisibleComponent*>& getRenderingQueue();
	std::vector<LightComponent*>& getLightQueue();
	std::vector<CameraComponent*>& getCameraQueue();
	std::vector<InputComponent*>& getInputQueue();

private:
	std::vector<VisibleComponent*> m_VisibleComponents;
	std::vector<LightComponent*> m_LightComponents;
	std::vector<CameraComponent*> m_CameraComponents;
	std::vector<InputComponent*> m_InputComponents;
};

