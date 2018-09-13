#pragma once
#include "../../common/InnoType.h"
#include "../../component/WindowSystemSingletonComponent.h"

namespace GLWindowSystem
{
	void setup();
	void initialize();
	void update();
	void shutdown();

	void swapBuffer();

	void hideMouseCursor();
	void showMouseCursor();

	objectStatus getStatus();
};