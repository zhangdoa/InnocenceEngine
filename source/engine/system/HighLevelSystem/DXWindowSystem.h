#pragma once
#include "../../exports/HighLevelSystem_Export.h"
#include "../../component/DXWindowSystemSingletonComponent.h"

namespace DXWindowSystem
{
	class Instance
	{
	public:
		InnoHighLevelSystem_EXPORT bool setup();
		InnoHighLevelSystem_EXPORT bool initialize();
		InnoHighLevelSystem_EXPORT bool update();
		InnoHighLevelSystem_EXPORT bool terminate();
		InnoHighLevelSystem_EXPORT objectStatus getStatus();

		void swapBuffer();

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