#pragma once

#include "../../component/DXWindowSystemSingletonComponent.h"

namespace DXWindowSystem
{
	class Instance
	{
	public:
		__declspec(dllexport) void setup();
		__declspec(dllexport) void initialize();
		__declspec(dllexport) void update();
		__declspec(dllexport) void shutdown();
		__declspec(dllexport) objectStatus getStatus();

		__declspec(dllexport) void swapBuffer();

		static Instance& get()
		{
			static Instance instance;
			return instance;
		}

	private:
		Instance() {};
	};
};

class windowCallbackWrapper
{
public:
	~windowCallbackWrapper() {};

	static windowCallbackWrapper& getInstance()
	{
		static windowCallbackWrapper instance;
		return instance;
	}

	LRESULT CALLBACK MessageHandler(HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam);

private:
	windowCallbackWrapper() {};
};

static windowCallbackWrapper* ApplicationHandle = 0;

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);