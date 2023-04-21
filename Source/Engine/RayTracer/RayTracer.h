#pragma once
#include "IRayTracer.h"

namespace Inno
{
	class RayTracer : public IRayTracer
	{
	public:
		INNO_CLASS_CONCRETE_NON_COPYABLE(RayTracer);

		bool Setup(ISystemConfig* systemConfig) override;
		bool Initialize() override;
		bool Execute() override;
		bool Terminate() override;

		ObjectStatus GetStatus() override;
	};
}