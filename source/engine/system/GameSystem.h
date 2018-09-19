#pragma once
#include "../common/ComponentHeaders.h"

namespace InnoGameSystem
{
	void setup();
	void addComponentsToMap();
	void initialize();
	void update();
	void shutdown();

	void addTransformComponent(TransformComponent* rhs);
	void addVisibleComponent(VisibleComponent* rhs);
	void addLightComponent(LightComponent* rhs);
	void addCameraComponent(CameraComponent* rhs);
	void addInputComponent(InputComponent* rhs);
	void addEnvironmentCaptureComponent(EnvironmentCaptureComponent* rhs);

	std::vector<TransformComponent*>& getTransformComponents();
	std::vector<VisibleComponent*>& getVisibleComponents();
	std::vector<LightComponent*>& getLightComponents();
	std::vector<CameraComponent*>& getCameraComponents();
	std::vector<InputComponent*>& getInputComponents();
	std::vector<EnvironmentCaptureComponent*>& getEnvironmentCaptureComponents();
	std::string getGameName();

	TransformComponent* getTransformComponent(EntityID parentEntity);

	void registerButtonStatusCallback(InputComponent* inputComponent, button boundButton, std::function<void()>* function);
	void registerMouseMovementCallback(InputComponent* inputComponent, int mouseCode, std::function<void(double)>* function);
	bool needRender();
	EntityID createEntityID();

	objectStatus getStatus();
};
