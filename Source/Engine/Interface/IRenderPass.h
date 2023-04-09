#pragma once
#include "ISystem.h"
#include "../Component/RenderPassComponent.h"
#include "../Component/GPUResourceComponent.h"
#include "../Component/SamplerComponent.h"
#include "../Component/GPUBufferComponent.h"
#include "../Common/InnoMath.h"

namespace Inno
{
	class IRenderingContext {};

	class IRenderPass : public ISystem
	{
	public:
		INNO_CLASS_INTERFACE_NON_COPYABLE(IRenderPass);

        virtual bool PrepareCommandList(IRenderingContext* renderingContext = nullptr) = 0;
		virtual RenderPassComponent* GetRenderPassComp() = 0;
	};
}