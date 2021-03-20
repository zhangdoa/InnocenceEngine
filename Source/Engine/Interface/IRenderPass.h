#pragma once
#include "ISystem.h"
#include "../Component/RenderPassDataComponent.h"
#include "../Component/GPUResourceComponent.h"
#include "../Component/SamplerDataComponent.h"
#include "../Component/GPUBufferDataComponent.h"
#include "../Common/InnoMath.h"

namespace Inno
{
	class IRenderingContext {};

	class IRenderPass : public ISystem
	{
	public:
		INNO_CLASS_INTERFACE_NON_COPYABLE(IRenderPass);

        virtual bool PrepareCommandList(IRenderingContext* renderingContext = nullptr) = 0;
		virtual RenderPassDataComponent* GetRPDC() = 0;
	};
}