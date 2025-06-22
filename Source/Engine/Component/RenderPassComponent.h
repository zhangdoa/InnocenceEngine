#pragma once
#include "../Common/GraphicsPrimitive.h"
#include "../Component/TextureComponent.h"
#include "../Component/ShaderProgramComponent.h"
#include "../Component/CommandListComponent.h"

namespace Inno
{
	class RenderPassComponent : public Component
	{
	public:
		static uint32_t GetTypeID() { return 11; };
		static const char* GetTypeName() { return "RenderPassComponent"; };

		ShaderProgramComponent* m_ShaderProgram = 0;

		RenderPassDesc m_RenderPassDesc = {};
		std::vector<ResourceBindingLayoutDesc> m_ResourceBindingLayoutDescs;

		size_t m_CurrentFrame = 0;

		std::function<void()> m_OnResize;
		std::function<void(CommandListComponent*)> m_CustomCommandsFunc;

		IOutputMergerTarget* m_OutputMergerTarget = 0;
		IPipelineStateObject* m_PipelineStateObject = 0;
		std::vector<ISemaphore*> m_Semaphores;
	};
}