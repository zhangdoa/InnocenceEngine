#pragma once
#include "../Interface/IService.h"

#include "../Common/ClassTemplate.h"

namespace Inno
{
	// When outputWidth and outputHeight are both non-zero they are used directly;
	// otherwise the output is sized to screenResolution / downsampleDenominator.
	struct RayTracerConfig : public IServiceConfig
	{
		uint32_t outputWidth          = 0;
		uint32_t outputHeight         = 0;
		uint32_t downsampleDenominator = 8;
	};

	class IRayTracer : public IService
	{
	public:
		INNO_CLASS_INTERFACE_NON_COPYABLE(IRayTracer);

		virtual bool Execute() = 0;
	};
}