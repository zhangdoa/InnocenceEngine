#pragma once
#include "../../exports/HighLevelSystem_Export.h"
#include "../../common/ComponentHeaders.h"

namespace InnoGameSystem
{
	InnoHighLevelSystem_EXPORT bool setup();
	InnoHighLevelSystem_EXPORT bool initialize();
	InnoHighLevelSystem_EXPORT bool update();
	InnoHighLevelSystem_EXPORT bool terminate();

	void addTransformComponent(TransformComponent* rhs);
	void addVisibleComponent(VisibleComponent* rhs);
	void addLightComponent(LightComponent* rhs);
	void addCameraComponent(CameraComponent* rhs);
	void addInputComponent(InputComponent* rhs);
	void addEnvironmentCaptureComponent(EnvironmentCaptureComponent* rhs);

	std::string getGameName();
	TransformComponent* getTransformComponent(EntityID parentEntity);

	void registerButtonStatusCallback(InputComponent* inputComponent, button boundButton, std::function<void()>* function);
	void registerMouseMovementCallback(InputComponent* inputComponent, int mouseCode, std::function<void(double)>* function);
	
	void updateTransform();

	InnoHighLevelSystem_EXPORT objectStatus getStatus();
};
