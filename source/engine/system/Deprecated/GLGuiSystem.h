#pragma once
#include "../IGuiSystem.h"

class GLGuiSystem : INNO_IMPLEMENT IGuiSystem
{
public:
	INNO_CLASS_CONCRETE_NON_COPYABLE(GLGuiSystem);

	bool setup() override;
	bool initialize() override;
	bool update() override;
	bool terminate() override;

	ObjectStatus getStatus() override;
};
