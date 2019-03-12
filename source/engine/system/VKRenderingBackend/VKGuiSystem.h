#pragma once
#include "../IGuiSystem.h"

class VKGuiSystem : INNO_IMPLEMENT IGuiSystem
{
public:
	INNO_CLASS_CONCRETE_NON_COPYABLE(VKGuiSystem);

	bool setup() override;
	bool initialize() override;
	bool update() override;
	bool terminate() override;

	ObjectStatus getStatus() override;
};