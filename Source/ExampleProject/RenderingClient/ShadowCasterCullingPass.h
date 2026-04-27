#pragma once
#include "ComputeCullingPass.h"

namespace Inno
{
	class ShadowCasterCullingPass : public ComputeCullingPass
	{
	public:
		INNO_CLASS_SINGLETON(ShadowCasterCullingPass)

	protected:
		const char* GetPassName() const override { return "ShadowCasterCullingPass"; }
		const char* GetComputeShaderPath() const override { return "shadowCasterCulling.comp"; }
	};
}
