#pragma once
#include "../../common/ComponentHeaders.h"

namespace InnoGameSystem
{
	void setup();
	void initialize();
	void update();
	void shutdown();

	void addTransformComponent(TransformComponent* rhs);
	void addVisibleComponent(VisibleComponent* rhs);
	void addLightComponent(LightComponent* rhs);
	void addCameraComponent(CameraComponent* rhs);
	void addInputComponent(InputComponent* rhs);
	void addEnvironmentCaptureComponent(EnvironmentCaptureComponent* rhs);

	__declspec(dllexport) std::string getGameName();

	__declspec(dllexport) TransformComponent* getTransformComponent(EntityID parentEntity);

	void registerButtonStatusCallback(InputComponent* inputComponent, button boundButton, std::function<void()>* function);
	void registerMouseMovementCallback(InputComponent* inputComponent, int mouseCode, std::function<void(double)>* function);
	EntityID createEntityID();

	objectStatus getStatus();
};
