#pragma once
#include "ISystem.h"
#include "../Component/RenderPassComponent.h"
#include "../Component/GPUResourceComponent.h"
#include "../Component/SamplerComponent.h"
#include "../Component/GPUBufferComponent.h"
#include "../Common/Math.h"

namespace Inno
{
	class IRenderingContext {};

	class IRenderPass : public ISystem
	{
	public:
		INNO_CLASS_INTERFACE_NON_COPYABLE(IRenderPass);

        virtual bool PrepareCommandList(IRenderingContext* renderingContext = nullptr) = 0;
		virtual RenderPassComponent* GetRenderPassComp() = 0;
		CommandListComponent* GetCommandListComp(GPUEngineType gpuEngineType)
		{
			switch (gpuEngineType)
			{
			case GPUEngineType::Graphics:
				return m_CommandListComp_Graphics;
			case GPUEngineType::Compute:
				return m_CommandListComp_Compute;
			case GPUEngineType::Copy:
				return m_CommandListComp_Copy;
			default:
				return nullptr;
			}
		}
	
	protected:
		CommandListComponent* m_CommandListComp_Graphics;
		CommandListComponent* m_CommandListComp_Compute;
		CommandListComponent* m_CommandListComp_Copy;
	};
}