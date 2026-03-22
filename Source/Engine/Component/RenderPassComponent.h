#pragma once
#include "../Common/GraphicsPrimitive.h"
#include "../Common/Object.h"
#include "../Component/TextureComponent.h"
#include "../Component/ShaderProgramComponent.h"
#include "../Component/CommandListComponent.h"

namespace Inno
{
	struct RenderPassComponent
	{
		ObjectStatus m_ObjectStatus = ObjectStatus::Invalid;
		ObjectName   m_InstanceName = "";

		ShaderProgramComponent* m_ShaderProgram = nullptr;

		RenderPassDesc m_RenderPassDesc = {};
		std::vector<ResourceBindingLayoutDesc> m_ResourceBindingLayoutDescs;

		size_t m_CurrentFrame = 0;

		std::function<void()> m_OnResize;
		std::function<void(CommandListComponent*)> m_CustomCommandsFunc;

		IOutputMergerTarget*  m_OutputMergerTarget  = nullptr;
		IPipelineStateObject* m_PipelineStateObject = nullptr;
		std::vector<ISemaphore*> m_Semaphores;
	};
}
