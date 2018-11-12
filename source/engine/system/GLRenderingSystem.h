#pragma once
#include "IRenderingSystem.h"

class GLRenderingSystem : INNO_IMPLEMENT IRenderingSystem
{
public:
	INNO_CLASS_CONCRETE_NON_COPYABLE(GLRenderingSystem);

	INNO_SYSTEM_EXPORT bool setup() override;
	INNO_SYSTEM_EXPORT bool initialize() override;
	INNO_SYSTEM_EXPORT bool update() override;
	INNO_SYSTEM_EXPORT bool terminate() override;

	INNO_SYSTEM_EXPORT objectStatus getStatus() override;
};