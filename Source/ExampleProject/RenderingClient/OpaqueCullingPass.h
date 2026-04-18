#pragma once
#include "ComputeCullingPass.h"

namespace Inno
{
	class OpaqueCullingPass : public ComputeCullingPass
	{
	public:
		INNO_CLASS_SINGLETON(OpaqueCullingPass)

	protected:
		const char* GetPassName() const override { return "OpaqueCullingPass"; }
		const char* GetComputeShaderPath() const override { return "opaqueGPUCulling.comp"; }
	};
}
