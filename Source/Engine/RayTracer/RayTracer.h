#pragma once
#include "IRayTracer.h"

INNO_CONCRETE InnoRayTracer : INNO_IMPLEMENT IRayTracer
{
public:
	INNO_CLASS_CONCRETE_NON_COPYABLE(InnoRayTracer);

	bool Setup() override;
	bool Initialize() override;
	bool Execute() override;
	bool Terminate() override;

	ObjectStatus GetStatus() override;
};