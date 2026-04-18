#pragma once
#include "ComputeCullingPass.h"

namespace Inno
{
	class SunShadowCullingPass : public ComputeCullingPass
	{
	public:
		INNO_CLASS_SINGLETON(SunShadowCullingPass)

	protected:
		const char* GetPassName() const override { return "SunShadowCullingPass"; }
		const char* GetComputeShaderPath() const override { return "sunShadowCulling.comp"; }
	};
}
