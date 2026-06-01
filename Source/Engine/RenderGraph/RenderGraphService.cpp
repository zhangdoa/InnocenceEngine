#include "RenderGraphService.h"
#include "RenderGraphSerializer.h"
#include "DefaultKernel.h"

#include "../Engine.h"
#include "../Services/RenderingConfigurationService.h"
#include "../Services/ShaderProgramResourceService.h"
#include "../Services/RenderPassResourceService.h"
#include "../Services/CommandListResourceService.h"
#include "ComputeCullingKernel.h"
#include "ScreenTileKernel.h"

using namespace Inno;

// Resource creation + resolution (FindResource / ResolveImportedResource /
// CreateResource / CreateScreenSizedTexture) lives in
// RenderGraphService_Resources.cpp.

RenderGraphService::RenderGraphService() = default;
RenderGraphService::~RenderGraphService() = default;

bool RenderGraphService::LoadGraph(const char* fileName)
{
	if (m_Loaded)
	{
		Log(Warning, "RenderGraphService::LoadGraph rejected: a graph is already loaded.");
		return false;
	}

	if (!RenderGraphSerializer::LoadFromFile(fileName, m_Desc))
	{
		Log(Error, "RenderGraphService failed to load graph file: ", fileName);
		return false;
	}

	for (const auto& l_resource : m_Desc.m_Resources)
	{
		if (!CreateResource(l_resource))
			return false;
	}

	for (const auto& l_pass : m_Desc.m_Passes)
	{
		if (!CreatePassNode(l_pass))
			return false;
	}

	m_Loaded = true;
	Log(Success, "RenderGraphService loaded graph [", m_Desc.m_Name.c_str(), "] with ",
		m_Desc.m_Resources.size(), " resources and ", m_Desc.m_Passes.size(), " passes.");
	return true;
}

IRenderGraphKernel* RenderGraphService::ResolveKernel(const std::string& name)
{
	auto it = m_Kernels.find(name);
	if (it != m_Kernels.end())
		return it->second.get();

	if (name == "Default")
	{
		auto l_kernel = std::make_unique<DefaultKernel>();
		auto l_raw = l_kernel.get();
		m_Kernels[name] = std::move(l_kernel);
		return l_raw;
	}

	if (name == "ComputeCulling")
	{
		auto l_kernel = std::make_unique<ComputeCullingKernel>();
		auto l_raw = l_kernel.get();
		m_Kernels[name] = std::move(l_kernel);
		return l_raw;
	}

	if (name == "ScreenTile")
	{
		auto l_kernel = std::make_unique<ScreenTileKernel>();
		auto l_raw = l_kernel.get();
		m_Kernels[name] = std::move(l_kernel);
		return l_raw;
	}

	Log(Error, "RenderGraphService: unknown kernel [", name.c_str(), "].");
	return nullptr;
}

bool RenderGraphService::CreatePassNode(const PassNodeDesc& desc)
{
	auto l_kernel = ResolveKernel(desc.m_Kernel);
	if (!l_kernel)
		return false;

	auto l_node = std::make_unique<RenderGraphPassNode>();
	l_node->m_Desc = desc;
	l_node->m_Kernel = l_kernel;

	l_node->m_ShaderProgram = g_Engine->Get<ShaderProgramResourceService>()->Add(desc.m_Name.c_str());
	l_node->m_ShaderProgram->m_ShaderFilePaths = desc.m_ShaderFilePaths;

	auto l_renderPass = g_Engine->Get<RenderPassResourceService>()->Add(desc.m_Name.c_str());

	auto l_renderPassDesc = g_Engine->Get<RenderingConfigurationService>()->GetDefaultRenderPassDesc();
	l_renderPassDesc.m_RenderTargetCount = 0;
	l_renderPassDesc.m_GPUEngineType = desc.m_Queue;
	l_renderPassDesc.m_Resizable = false;

	// A screen-sized write makes this node own a deferred RT: hand the engine an
	// RT-init-func that (re)creates that texture at current screen resolution, and
	// mark the pass resizable so PostResize re-invokes it (parity with the
	// imperative pass's m_RenderTargetsInitializationFunc). Multiple screen-sized
	// writes are all (re)created in one func call.
	Inno::Array<ResourceDesc> l_screenWrites;
	for (const auto& l_write : desc.m_Writes)
	{
		auto it = m_DeferredScreenTextures.find(l_write);
		if (it != m_DeferredScreenTextures.end())
			l_screenWrites.push_back(it->second);
	}
	if (!l_screenWrites.empty())
	{
		l_renderPassDesc.m_Resizable = true;
		l_renderPassDesc.m_UseOutputMerger = false;
		l_renderPassDesc.m_RenderTargetsInitializationFunc = [this, l_screenWrites]()
		{
			for (const auto& l_resource : l_screenWrites)
			{
				if (!CreateScreenSizedTexture(l_resource))
					return false;
			}
			return true;
		};
	}

	l_renderPass->m_RenderPassDesc = l_renderPassDesc;

	l_renderPass->m_ResourceBindingLayoutDescs.resize(desc.m_Bindings.size());
	for (size_t i = 0; i < desc.m_Bindings.size(); i++)
	{
		const auto& l_binding = desc.m_Bindings[i];
		auto& l_layout = l_renderPass->m_ResourceBindingLayoutDescs[i];
		l_layout.m_GPUResourceType = l_binding.m_GPUResourceType;
		l_layout.m_DescriptorSetIndex = l_binding.m_DescriptorSetIndex;
		l_layout.m_DescriptorIndex = l_binding.m_DescriptorIndex;
		l_layout.m_BindingAccessibility = l_binding.m_BindingAccessibility;
		l_layout.m_ResourceAccessibility = l_binding.m_ResourceAccessibility;
		l_layout.m_TextureUsage = l_binding.m_TextureUsage;
		l_layout.m_ShaderStage = l_binding.m_ShaderStage;
	}

	l_renderPass->m_ShaderProgram = l_node->m_ShaderProgram;
	l_node->m_RenderPass = l_renderPass;

	l_node->m_CommandList_Compute =
		g_Engine->Get<CommandListResourceService>()->Add((desc.m_Name + "/Compute").c_str());
	l_node->m_CommandList_Compute->m_Type = GPUEngineType::Compute;
	l_node->m_CommandList_Graphics =
		g_Engine->Get<CommandListResourceService>()->Add((desc.m_Name + "/Graphics").c_str());
	l_node->m_CommandList_Graphics->m_Type = GPUEngineType::Graphics;

	// Primary output = first graph-OWNED write (parity with the pass's GetResult()).
	// Imported writes are created by their owning pass after graph load, so they
	// resolve lazily at RecordNode — never eagerly here (would log a false miss).
	if (!desc.m_Writes.empty())
	{
		auto it = m_Resources.find(desc.m_Writes[0]);
		if (it != m_Resources.end())
			l_node->m_PrimaryOutput = it->second;
	}

	auto l_raw = l_node.get();
	m_Nodes[desc.m_Name] = std::move(l_node);
	m_Schedule.push_back(l_raw);
	return true;
}

RenderGraphPassNode* RenderGraphService::FindNode(const char* name)
{
	auto it = m_Nodes.find(name);
	return (it != m_Nodes.end()) ? it->second.get() : nullptr;
}

bool RenderGraphService::RecordNode(RenderGraphPassNode* node)
{
	if (!node || !node->m_Kernel)
	{
		Log(Warning, "RenderGraphService::RecordNode rejected: null node or kernel.");
		return false;
	}

	RenderGraphPassContext l_ctx;
	l_ctx.m_Node = &node->m_Desc;
	l_ctx.m_RenderPass = node->m_RenderPass;
	l_ctx.m_CommandList = (node->m_Desc.m_Queue == GPUEngineType::Graphics)
		? node->m_CommandList_Graphics : node->m_CommandList_Compute;

	l_ctx.m_BoundResources.reserve(node->m_Desc.m_Bindings.size());
	for (const auto& l_binding : node->m_Desc.m_Bindings)
		l_ctx.m_BoundResources.push_back(FindResource(l_binding.m_Resource));

	return node->m_Kernel->Record(l_ctx);
}
