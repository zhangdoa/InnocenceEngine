#pragma once
#include "InnoType.h"

namespace InnoApplication
{
	void setup();
	void initialize();
	void update();
	void shutdown();

	objectStatus m_objectStatus = objectStatus::SHUTDOWN;
};