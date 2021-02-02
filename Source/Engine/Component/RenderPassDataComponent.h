#pragma once
#include "../Common/InnoType.h"
#include "../Common/InnoGraphicsPrimitive.h"
#include "../Component/TextureDataComponent.h"
#include "../Component/ShaderProgramComponent.h"

namespace Inno
{
	struct RenderTarget
	{

	};
	
	class RenderPassDataComponent : public InnoComponent
	{
	public:
		static uint32_t GetTypeID() { return 11; };
		static const char* GetTypeName() { return "RenderPassDataComponent"; };

		ShaderProgramComponent* m_ShaderProgram = 0;

		RenderPassDesc m_RenderPassDesc = {};

		std::vector<TextureDataComponent*> m_RenderTargets;
		TextureDataComponent* m_DepthStencilRenderTarget = 0;
		std::vector<ResourceBindingLayoutDesc> m_ResourceBindingLayoutDescs;

		IPipelineStateObject* m_PipelineStateObject = 0;
		std::vector<ICommandList*> m_CommandLists;

		size_t m_CurrentFrame = 0;

		std::vector<ISemaphore*> m_WaitSemaphores;
		std::vector<ISemaphore*> m_SignalSemaphores;
		std::vector<IFence*> m_Fences;
	};
}