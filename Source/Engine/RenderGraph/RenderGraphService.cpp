#include "RenderGraphService.h"
#include "RenderGraphSerializer.h"
#include "RenderGraphPassRecorder.h"

#include "../Engine.h"
#include "../Services/RenderingConfigurationService.h"
#include "../Services/ShaderProgramResourceService.h"
#include "../Services/RenderPassResourceService.h"
#include "../Services/CommandListResourceService.h"

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
		// Bypass node: declared in the graph for documentation / round-trip /
		// future re-enable, but never instantiated (no components, no shader
		// compile, no RT alloc, no entry in m_Schedule). Render() and
		// CreateOrphanResources / CreateResource also short-circuit on
		// IsReferencedByLivePasses so its Writes never allocate either.
		if (l_pass.m_BypassEnabled)
		{
			Log(Verbose, "RenderGraphService: pass [", l_pass.m_Name.c_str(),
				"] bypassed; skipping instantiation.");
			continue;
		}
		if (!CreatePassNode(l_pass))
			return false;
	}

	// Graph-owned resources whose producing pass isn't a graph node yet have no
	// node to create/initialize them; the factory does it so consumers bind a
	// valid (zeroed) input. Resources WITH a producing node are owned by it.
	CreateOrphanResources();

	// Named init hooks: residual CPU resource creation/fill a pure-data node can't
	// express (SSAO noise/kernel/samplers, tiled-frustum buffers). The client
	// registered these before LoadGraph; run each once now that the nodes exist.
	// Bypassed nodes aren't instantiated, so their registered hooks (if any) are
	// skipped — invoking them would touch un-created resources.
	for (const auto& l_pass : m_Desc.m_Passes)
	{
		if (l_pass.m_BypassEnabled)
			continue;
		auto l_hook = m_InitHooks.find(l_pass.m_Name);
		if (l_hook != m_InitHooks.end())
			l_hook->second();
	}


	// Drain RenderPassResourceService deferred queue, then publish each
	// raster node's auto-created OutputMerger color + depth-stencil textures
	// under their conventional names so downstream passes can bind them by
	// name in pure JSON.
	g_Engine->Get<RenderPassResourceService>()->InitializeComponents();
	for (auto& l_kv : m_Nodes)
	{
		auto& l_nodeDesc = l_kv.second->m_Desc;
		auto* l_renderPass = l_kv.second->m_RenderPass;
		if (!l_nodeDesc.m_Raster.m_Enabled || !l_renderPass)
			continue;
		auto* l_om = l_renderPass->m_OutputMergerTarget;
		if (!l_om)
			continue;
		for (size_t i = 0; i < l_om->m_ColorOutputs.size(); ++i)
		{
			if (l_om->m_ColorOutputs[i])
				m_Resources[l_nodeDesc.m_Name + "_RT_" + std::to_string(i)] = l_om->m_ColorOutputs[i];
		}
		if (l_om->m_DepthStencilOutput)
			m_Resources[l_nodeDesc.m_Name + "_DS"] = l_om->m_DepthStencilOutput;
	}

	m_Loaded = true;
	Log(Success, "RenderGraphService loaded graph [", m_Desc.m_Name.c_str(), "] with ",
		m_Desc.m_Resources.size(), " resources and ", m_Desc.m_Passes.size(), " passes.");
	return true;
}

bool RenderGraphService::CreatePassNode(const PassNodeDesc& desc)
{
	auto l_node = std::make_unique<RenderGraphPassNode>();
	l_node->m_Desc = desc;


	l_node->m_ShaderProgram = g_Engine->Get<ShaderProgramResourceService>()->Add(desc.m_Name.c_str());
	l_node->m_ShaderProgram->m_ShaderFilePaths = desc.m_ShaderFilePaths;

	auto l_renderPass = g_Engine->Get<RenderPassResourceService>()->Add(desc.m_Name.c_str());

	auto l_renderPassDesc = g_Engine->Get<RenderingConfigurationService>()->GetDefaultRenderPassDesc();
	l_renderPassDesc.m_RenderTargetCount = 0;
	l_renderPassDesc.m_GPUEngineType = desc.m_Queue;
	l_renderPassDesc.m_Resizable = false;

	// A raster node draws into an OutputMergerTarget via ExecuteIndirect instead of
	// a compute Dispatch. The engine auto-creates "<Node>_RT_<i>" + "<Node>_DS" from
	// m_RenderTargetCount; downstream nodes read those by name. RT format/size stays
	// at the engine default (screen-sized), matching the imperative pass.
	if (desc.m_Raster.m_Enabled)
	{
		l_renderPassDesc.m_RenderTargetCount = desc.m_Raster.m_RenderTargetCount;
		l_renderPassDesc.m_UseDepthBuffer = desc.m_Raster.m_UseDepthBuffer;
		l_renderPassDesc.m_IndirectDraw = desc.m_Raster.m_IndirectDraw;
		l_renderPassDesc.m_UseOutputMerger = true;
		l_renderPassDesc.m_Resizable = true;
		l_renderPassDesc.m_PostCLState = desc.m_Raster.m_CrossQueueExitToCommon
			? CrossQueueExit::ToCommon : CrossQueueExit::None;
		auto& l_pipeline = l_renderPassDesc.m_GraphicsPipelineDesc;
		l_pipeline.m_DepthStencilDesc.m_DepthEnable = desc.m_Raster.m_DepthEnable;
		l_pipeline.m_DepthStencilDesc.m_AllowDepthWrite = desc.m_Raster.m_DepthWrite;
		l_pipeline.m_DepthStencilDesc.m_DepthComparisionFunction = desc.m_Raster.m_DepthCompare;
		l_pipeline.m_DepthStencilDesc.m_AllowDepthClamp = desc.m_Raster.m_DepthClamp;
		l_pipeline.m_RasterizerDesc.m_UseCulling = desc.m_Raster.m_UseCulling;
	}

	// A raytracing node: the engine builds the RT PSO + shader table from the
	// ShaderProgram's RT stages (copied above). m_UseOutputMerger is forced off
	// by the deferred-RT block below (the visibility UAV is a screen write).
	if (desc.m_UseRaytracing)
		l_renderPassDesc.m_UseRaytracing = true;

	// A screen-sized write makes this node own a deferred RT: hand the engine an
	// RT-init-func that (re)creates that texture at current screen resolution, and
	// mark the pass resizable so PostResize re-invokes it (parity with the
	// imperative pass's m_RenderTargetsInitializationFunc). Multiple screen-sized
	// writes are all (re)created in one func call.
	Inno::Array<ResourceDesc> l_deferredWrites;
	for (const auto& l_write : desc.m_Writes)
	{
		auto itScreen = m_DeferredScreenTextures.find(l_write);
		if (itScreen != m_DeferredScreenTextures.end())
		{
			l_deferredWrites.push_back(itScreen->second);
			continue;
		}
		auto itTiled = m_DeferredTiledTextures.find(l_write);
		if (itTiled != m_DeferredTiledTextures.end())
		{
			l_deferredWrites.push_back(itTiled->second);
			continue;
		}
		auto itTiledArray = m_DeferredTiledArrayTextures.find(l_write);
		if (itTiledArray != m_DeferredTiledArrayTextures.end())
		{
			l_deferredWrites.push_back(itTiledArray->second);
			continue;
		}
	}
	if (!l_deferredWrites.empty())
	{
		l_renderPassDesc.m_Resizable = true;
		l_renderPassDesc.m_UseOutputMerger = false;
		l_renderPassDesc.m_RenderTargetsInitializationFunc = [this, l_deferredWrites]()
		{
			for (const auto& l_resource : l_deferredWrites)
			{
				if (l_resource.m_SizeExpr == "tiled")
				{
					if (!CreateTiledTexture(l_resource)) return false;
					continue;
				}
				if (l_resource.m_SizeExpr == "tiledArray")
				{
					if (!CreateTiledArrayTexture(l_resource)) return false;
					continue;
				}
				if (!CreateScreenSizedTexture(l_resource)) return false;
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
		l_layout.m_IsRootConstant = l_binding.m_IsRootConstant;
		l_layout.m_SubresourceCount = l_binding.m_SubresourceCount;
		l_layout.m_GPUBufferUsage = l_binding.m_GPUBufferUsage;
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

	// The graph owns its components' lifetime: initialize them here so passes
	// don't. Initializing the render pass fires its RT-init-func, creating any
	// screen-sized output it writes.
	g_Engine->Get<ShaderProgramResourceService>()->Initialize(l_node->m_ShaderProgram);
	g_Engine->Get<RenderPassResourceService>()->Initialize(l_node->m_RenderPass);
	g_Engine->Get<CommandListResourceService>()->Initialize(l_node->m_CommandList_Compute);
	g_Engine->Get<CommandListResourceService>()->Initialize(l_node->m_CommandList_Graphics);
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
	if (!node)
	{
		Log(Warning, "RenderGraphService::RecordNode rejected: null node.");
		return false;
	}

	RenderGraphPassContext l_ctx;
	l_ctx.m_Node = &node->m_Desc;
	l_ctx.m_RenderPass = node->m_RenderPass;
	l_ctx.m_CommandList = (node->m_Desc.m_Queue == GPUEngineType::Graphics)
		? node->m_CommandList_Graphics : node->m_CommandList_Compute;
	l_ctx.m_CommandList_Graphics = node->m_CommandList_Graphics;

	l_ctx.m_BoundResources.reserve(node->m_Desc.m_Bindings.size());
	for (const auto& l_binding : node->m_Desc.m_Bindings)
	{
		// Root-constant slots carry no resource (the indirect command signature
		// supplies the value per draw); an empty name is an intentional null bind
		// (e.g. a bindless texture slot). Push null; the recorder skips/null-binds.
		if (l_binding.m_IsRootConstant || l_binding.m_Resource.empty())
			l_ctx.m_BoundResources.push_back(nullptr);
		else
			l_ctx.m_BoundResources.push_back(l_binding.m_PingPongHistory
				? PingPongTexture(l_binding.m_Resource, true)
				: FindResource(l_binding.m_Resource));
	}

	if (node->m_Desc.m_Raster.m_Enabled && !node->m_Desc.m_Raster.m_IndirectArgsBuffer.empty())
		l_ctx.m_IndirectArgs = FindResource(node->m_Desc.m_Raster.m_IndirectArgsBuffer);

	return RecordPass(l_ctx);
}
