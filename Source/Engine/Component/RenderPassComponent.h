#pragma once
#include "../Common/InnoType.h"
#include "../Common/InnoGraphicsPrimitive.h"
#include "../Component/TextureComponent.h"
#include "../Component/ShaderProgramComponent.h"

namespace Inno
{
	struct RenderTarget
	{

	};
	
	class RenderPassComponent : public InnoComponent
	{
	public:
		static uint32_t GetTypeID() { return 11; };
		static const char* GetTypeName() { return "RenderPassComponent"; };

		ShaderProgramComponent* m_ShaderProgram = 0;

		RenderPassDesc m_RenderPassDesc = {};

		std::vector<TextureComponent*> m_RenderTargets;
		TextureComponent* m_DepthStencilRenderTarget = 0;
		std::vector<ResourceBindingLayoutDesc> m_ResourceBindingLayoutDescs;

		IPipelineStateObject* m_PipelineStateObject = 0;
		size_t m_CurrentFrame = 0;

		std::vector<ICommandList*> m_CommandLists;
		std::vector<ISemaphore*> m_Semaphores;
	};
}