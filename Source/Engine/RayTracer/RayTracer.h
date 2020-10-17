#pragma once
#include "IRayTracer.h"

namespace Inno
{
	class InnoRayTracer : public IRayTracer
	{
	public:
		INNO_CLASS_CONCRETE_NON_COPYABLE(InnoRayTracer);

		bool Setup(ISystemConfig* systemConfig) override;
		bool Initialize() override;
		bool Execute() override;
		bool Terminate() override;

		ObjectStatus GetStatus() override;
	};
}