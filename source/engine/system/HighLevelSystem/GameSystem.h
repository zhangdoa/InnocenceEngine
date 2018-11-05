#pragma once
#include "../LowLevelSystem/MemorySystem.h"
#include "../../common/ComponentHeaders.h"
#include "../../game/InnocenceGarden/PlayerCharacter.h"

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

	void registerComponents(PlayerComponent* playerComponent);

	template <typename T> T * spawn()
	{
		auto l_ptr = InnoMemorySystem::spawn<T>();
		if (l_ptr)
		{
			registerComponents(l_ptr);
			return l_ptr;
		}
		else
		{
			return nullptr;
		}
	};

	std::string getGameName();
	TransformComponent* getTransformComponent(EntityID parentEntity);

	void registerButtonStatusCallback(InputComponent* inputComponent, button boundButton, std::function<void()>* function);
	void registerMouseMovementCallback(InputComponent* inputComponent, int mouseCode, std::function<void(float)>* function);
	
	void saveComponentsCapture();

	InnoHighLevelSystem_EXPORT objectStatus getStatus();
};
