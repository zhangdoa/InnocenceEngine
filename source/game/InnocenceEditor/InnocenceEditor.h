#pragma once

#include "system/GameSystem.h"

class InnocenceEditor : public GameSystem
{
public:
	InnocenceEditor() {};
	~InnocenceEditor() {};

	void setup() override;
	void initialize() override;
	void update() override;
	void terminate() override;
	const objectStatus& getStatus() const override;

	std::string getGameName() const override;
	std::vector<TransformComponent*>& getTransformComponents() override;
	std::vector<CameraComponent*>& getCameraComponents() override;
	std::vector<InputComponent*>& getInputComponents() override;
	std::vector<LightComponent*>& getLightComponents() override;
	std::vector<VisibleComponent*>& getVisibleComponents() override;


private:
	objectStatus m_objectStatus = objectStatus::SHUTDOWN;

	std::vector<TransformComponent*> m_transformComponents;
	std::vector<CameraComponent*> m_cameraComponents;
	std::vector<InputComponent*> m_inputComponents;
	std::vector<LightComponent*> m_lightComponents;
	std::vector<VisibleComponent*> m_visibleComponents;
};
