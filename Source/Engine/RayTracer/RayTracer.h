#pragma once
#include "IRayTracer.h"

class InnoRayTracer : public IRayTracer
{
public:
	INNO_CLASS_CONCRETE_NON_COPYABLE(InnoRayTracer);

	bool Setup() override;
	bool Initialize() override;
	bool Execute() override;
	bool Terminate() override;

	ObjectStatus GetStatus() override;
};