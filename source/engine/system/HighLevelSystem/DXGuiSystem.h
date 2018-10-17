#pragma once
#include "../../common/InnoType.h"

namespace DXGuiSystem
{
	class Instance
	{
	public:
		__declspec(dllexport) void setup();
		__declspec(dllexport) void initialize();
		__declspec(dllexport) void update();
		__declspec(dllexport) void shutdown();

		__declspec(dllexport) objectStatus getStatus();

		static Instance& get()
		{
			static Instance instance;
			return instance;
		}

	private:
		Instance() {};
	};
};

