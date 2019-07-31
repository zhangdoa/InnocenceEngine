#pragma once
#include "../Common/InnoType.h"
#include "../Common/InnoGraphicsPrimitive.h"
#include "../Component/TextureDataComponent.h"
#include "../Component/ShaderProgramComponent.h"

class RenderPassDataComponent : public InnoComponent
{
public:
	ShaderProgramComponent* m_ShaderProgram = 0;

	RenderPassDesc m_RenderPassDesc = {};

	std::vector<TextureDataComponent*> m_RenderTargets;
	TextureDataComponent* m_DepthStencilRenderTarget = 0;
	IResourceBinder* m_RenderTargetsResourceBinder = 0;
	std::vector<ResourceBinderLayoutDesc> m_ResourceBinderLayoutDescs;

	IPipelineStateObject* m_PipelineStateObject = 0;

	ICommandQueue* m_CommandQueue = 0;
	std::vector<ICommandList*> m_CommandLists;

	size_t m_CurrentFrame = 0;

	std::vector<ISemaphore*> m_WaitSemaphores;
	std::vector<ISemaphore*> m_SignalSemaphores;
	std::vector<IFence*> m_Fences;
};