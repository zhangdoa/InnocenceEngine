#pragma once
#include "IGUISystem.h"

class InnoGUISystem : public IGUISystem
{
public:
	INNO_CLASS_CONCRETE_NON_COPYABLE(InnoGUISystem);

	bool setup() override;
	bool initialize() override;
	bool update() override;
	bool terminate() override;

	ObjectStatus getStatus() override;
};