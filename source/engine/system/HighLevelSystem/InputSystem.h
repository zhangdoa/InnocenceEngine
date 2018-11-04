#pragma once
#include "../../exports/HighLevelSystem_Export.h"
#include "../../common/InnoType.h"

namespace InnoInputSystem 
{
	InnoHighLevelSystem_EXPORT void setup();
	InnoHighLevelSystem_EXPORT void initialize();
	InnoHighLevelSystem_EXPORT void update();
	InnoHighLevelSystem_EXPORT void terminate();

	InnoHighLevelSystem_EXPORT void addButtonStatusCallback(button boundButton, std::function<void()>* buttonStatusCallbackFunctor);
	InnoHighLevelSystem_EXPORT void addButtonStatusCallback(button boundButton, std::vector<std::function<void()>*>& buttonStatusCallbackFunctor);
	InnoHighLevelSystem_EXPORT void addButtonStatusCallback(buttonStatusCallbackMap & buttonStatusCallbackFunctor);
	InnoHighLevelSystem_EXPORT void addMouseMovementCallback(int mouseCode, std::function<void(float)>* mouseMovementCallback);
	InnoHighLevelSystem_EXPORT void addMouseMovementCallback(int mouseCode, std::vector<std::function<void(float)>*>& mouseMovementCallback);
	InnoHighLevelSystem_EXPORT void addMouseMovementCallback(mouseMovementCallbackMap& mouseMovementCallback);

	InnoHighLevelSystem_EXPORT void framebufferSizeCallback(int width, int height);
	InnoHighLevelSystem_EXPORT void mousePositionCallback(float mouseXPos, float mouseYPos);
	InnoHighLevelSystem_EXPORT void scrollCallback(float xoffset, float yoffset);

	InnoHighLevelSystem_EXPORT objectStatus getStatus();
};