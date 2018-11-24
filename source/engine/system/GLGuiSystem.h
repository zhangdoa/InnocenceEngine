#pragma once
#include "IGuiSystem.h"

class GLGuiSystem : INNO_IMPLEMENT IGuiSystem
{
public:
	INNO_CLASS_CONCRETE_NON_COPYABLE(GLGuiSystem);

	INNO_SYSTEM_EXPORT bool setup() override;
	INNO_SYSTEM_EXPORT bool initialize() override;
	INNO_SYSTEM_EXPORT bool update() override;
	INNO_SYSTEM_EXPORT bool terminate() override;

	INNO_SYSTEM_EXPORT ObjectStatus getStatus() override;
};

