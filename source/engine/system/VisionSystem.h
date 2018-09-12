#pragma once
#include "../common/InnoType.h"

namespace InnoVisionSystem
{
	void setup();
	void initialize();
	void update();
	void shutdown();

	objectStatus m_VisionSystemStatus = objectStatus::SHUTDOWN;
};
