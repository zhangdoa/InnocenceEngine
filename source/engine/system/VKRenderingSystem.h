#pragma once
#include "IRenderingSystem.h"

class VKRenderingSystem : INNO_IMPLEMENT IRenderingSystem
{
public:
	INNO_CLASS_CONCRETE_NON_COPYABLE(VKRenderingSystem);

	INNO_SYSTEM_EXPORT bool setup() override;
	INNO_SYSTEM_EXPORT bool initialize() override;
	INNO_SYSTEM_EXPORT bool update() override;
	INNO_SYSTEM_EXPORT bool terminate() override;

	INNO_SYSTEM_EXPORT ObjectStatus getStatus() override;

	INNO_SYSTEM_EXPORT bool resize() override;
};