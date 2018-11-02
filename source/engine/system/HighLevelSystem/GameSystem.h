#pragma once
#include "../../exports/HighLevelSystem_Export.h"
#include "../LowLevelSystem/MemorySystem.h"
#include "../../common/ComponentHeaders.h"

namespace InnoGameSystem
{
	InnoHighLevelSystem_EXPORT bool setup();
	InnoHighLevelSystem_EXPORT bool initialize();
	InnoHighLevelSystem_EXPORT bool update();
	InnoHighLevelSystem_EXPORT bool terminate();

	void registerComponents(TransformComponent* transformComponent);
	void registerComponents(VisibleComponent* visibleComponent);
	void registerComponents(LightComponent* lightComponent);
	void registerComponents(CameraComponent* cameraComponent);
	void registerComponents(InputComponent* inputComponent);
	void registerComponents(EnvironmentCaptureComponent* environmentCaptureComponent);

	template <typename T> T * spawn()
	{
		auto l_ptr = InnoMemorySystem::spawn<T>();
		if (l_ptr)
		{
			registerComponents(l_ptr);
		}
	};

	std::string getGameName();
	TransformComponent* getTransformComponent(EntityID parentEntity);

	void registerButtonStatusCallback(InputComponent* inputComponent, button boundButton, std::function<void()>* function);
	void registerMouseMovementCallback(InputComponent* inputComponent, int mouseCode, std::function<void(double)>* function);
	
	void saveComponentsCapture();

	InnoHighLevelSystem_EXPORT objectStatus getStatus();
};
