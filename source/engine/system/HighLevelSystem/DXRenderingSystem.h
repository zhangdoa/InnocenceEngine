#pragma once
#include "../../exports/HighLevelSystem_Export.h"
#include "../../common/InnoType.h"

namespace DXRenderingSystem
{
	class Instance
	{
	public:
		InnoHighLevelSystem_EXPORT bool setup();
		InnoHighLevelSystem_EXPORT bool initialize();
		InnoHighLevelSystem_EXPORT bool update();
		InnoHighLevelSystem_EXPORT bool terminate();

		InnoHighLevelSystem_EXPORT objectStatus getStatus();

		static Instance& get()
		{
			static Instance instance;
			return instance;
		}

	private:
		Instance() {};
	};
};
