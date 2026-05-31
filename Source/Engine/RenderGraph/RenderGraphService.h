#pragma once
#include <memory>
#include <unordered_map>
#include "RenderGraphDesc.h"
#include "IRenderGraphKernel.h"
#include "../Component/TextureComponent.h"
#include "../Component/ShaderProgramComponent.h"

namespace Inno
{
	// A compiled pass node: the engine components the graph built from a
	// PassNodeDesc, plus the kernel that records its command list.
	struct RenderGraphPassNode
	{
		PassNodeDesc m_Desc;
		RenderPassComponent* m_RenderPass = nullptr;
		ShaderProgramComponent* m_ShaderProgram = nullptr;
		CommandListComponent* m_CommandList_Compute = nullptr;
		CommandListComponent* m_CommandList_Graphics = nullptr;
		IRenderGraphKernel* m_Kernel = nullptr;
		// First Writes resource — the node's primary output (parity with GetResult()).
		GPUResourceComponent* m_PrimaryOutput = nullptr;
	};

	// Startup-load + in-memory compile (RFC D3): loads a render-graph JSON,
	// creates resources + pass nodes via the existing resource services, and
	// records per node through the attached kernel. Phase-0 scope is one
	// no-dependency node; ScheduledNodes() is where a topo-sort slots in later.
	class RenderGraphService
	{
	public:
		RenderGraphService();
		~RenderGraphService();

		bool LoadGraph(const char* fileName);

		RenderGraphPassNode* FindNode(const char* name);
		const Inno::Array<RenderGraphPassNode*>& ScheduledNodes() const { return m_Schedule; }

		// Record a node's command list via its kernel (per-frame / one-shot).
		bool RecordNode(RenderGraphPassNode* node);

	private:
		GPUResourceComponent* FindResource(const std::string& name);
		bool CreateResource(const ResourceDesc& desc);
		bool CreatePassNode(const PassNodeDesc& desc);
		IRenderGraphKernel* ResolveKernel(const std::string& name);

		RenderGraphDesc m_Desc;
		std::unordered_map<std::string, GPUResourceComponent*> m_Resources;
		std::unordered_map<std::string, std::unique_ptr<RenderGraphPassNode>> m_Nodes;
		std::unordered_map<std::string, std::unique_ptr<IRenderGraphKernel>> m_Kernels;
		Inno::Array<RenderGraphPassNode*> m_Schedule;
		bool m_Loaded = false;
	};
}
