#pragma once
#include "../Common/GraphicsPrimitive.h"
#include "../Component/TextureComponent.h"
#include "../Component/ShaderProgramComponent.h"

namespace Inno
{
	struct RenderTarget
	{
		TextureComponent* m_Texture = nullptr;
		bool m_IsOwned = false;
	};
	
	class RenderPassComponent : public Component
	{
	public:
		static uint32_t GetTypeID() { return 11; };
		static const char* GetTypeName() { return "RenderPassComponent"; };

		ShaderProgramComponent* m_ShaderProgram = 0;

		RenderPassDesc m_RenderPassDesc = {};

		std::vector<RenderTarget> m_RenderTargets;
		RenderTarget m_DepthStencilRenderTarget = {};
		std::vector<ResourceBindingLayoutDesc> m_ResourceBindingLayoutDescs;

		IPipelineStateObject* m_PipelineStateObject = 0;
		size_t m_CurrentFrame = 0;

		std::function<void()> m_OnResize;

		std::vector<ICommandList*> m_CommandLists;
		std::vector<ISemaphore*> m_Semaphores;
	};
}