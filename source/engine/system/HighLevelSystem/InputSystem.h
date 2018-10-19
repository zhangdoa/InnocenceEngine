#pragma once
#include "../../component/WindowSystemSingletonComponent.h"

namespace InnoInputSystem 
{
	__declspec(dllexport) void setup();
	__declspec(dllexport) void initialize();
	__declspec(dllexport) void update();
	__declspec(dllexport) void terminate();

	__declspec(dllexport) void addButtonStatusCallback(button boundButton, std::function<void()>* buttonStatusCallbackFunctor);
	__declspec(dllexport) void addButtonStatusCallback(button boundButton, std::vector<std::function<void()>*>& buttonStatusCallbackFunctor);
	__declspec(dllexport) void addButtonStatusCallback(buttonStatusCallbackMap & buttonStatusCallbackFunctor);
	__declspec(dllexport) void addMouseMovementCallback(int mouseCode, std::function<void(double)>* mouseMovementCallback);
	__declspec(dllexport) void addMouseMovementCallback(int mouseCode, std::vector<std::function<void(double)>*>& mouseMovementCallback);
	__declspec(dllexport) void addMouseMovementCallback(mouseMovementCallbackMap& mouseMovementCallback);

	__declspec(dllexport) void framebufferSizeCallback(int width, int height);
	__declspec(dllexport) void mousePositionCallback(double mouseXPos, double mouseYPos);
	__declspec(dllexport) void scrollCallback(double xoffset, double yoffset);

	__declspec(dllexport) objectStatus getStatus();
};