#pragma once
#include "../../common/InnoType.h"

namespace GLRenderingSystem
{
	void setup();
	void initialize();
	void update();
	void shutdown();

	void initializeDefaultGraphicPrimtives();
	void initializeGraphicPrimtivesOfComponents();

	objectStatus getStatus();
};
