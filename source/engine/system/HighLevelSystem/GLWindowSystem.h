#pragma once
#include "../../common/InnoType.h"
#include "../../component/WindowSystemSingletonComponent.h"

namespace GLWindowSystem
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

		__declspec(dllexport) void hideMouseCursor();
		__declspec(dllexport) void showMouseCursor();

		static Instance& get()
		{
			static Instance instance;
			return instance;
		}

	private:
		Instance() {};
	};
};