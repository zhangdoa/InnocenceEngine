#pragma once
#include "../IGuiSystem.h"
#include "../../exports/InnoSystem_Export.h"

class DXGuiSystem : INNO_IMPLEMENT IGuiSystem
{
public:
	INNO_CLASS_CONCRETE_NON_COPYABLE(DXGuiSystem);

	bool setup() override;
	bool initialize() override;
	bool update() override;
	bool terminate() override;

	ObjectStatus getStatus() override;
};
