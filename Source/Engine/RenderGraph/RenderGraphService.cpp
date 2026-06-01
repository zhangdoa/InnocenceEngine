#include "RenderGraphService.h"
#include "RenderGraphSerializer.h"
#include "DefaultKernel.h"

#include "../Engine.h"
#include "../Services/RenderingConfigurationService.h"
#include "../Services/ShaderProgramResourceService.h"
#include "../Services/RenderPassResourceService.h"
#include "../Services/TextureResourceService.h"
#include "../Services/GPUBufferResourceService.h"
#include "../Services/CommandListResourceService.h"

using namespace Inno;

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

GPUResourceComponent* RenderGraphService::FindResource(const std::string& name)
{
	auto it = m_Resources.find(name);
	if (it != m_Resources.end())
		return it->second;

	return ResolveImportedResource(name);
}

GPUResourceComponent* RenderGraphService::ResolveImportedResource(const std::string& name)
{
	// Imported resources (and any Reads name not declared in Resources) are
	// produced by a still-imperative pass: resolve to the live engine resource
	// by name via the owning *ResourceService. The graph never creates or owns
	// them, so the imperative consumer and the graph share one handle.
	if (auto l_buffer = g_Engine->Get<GPUBufferResourceService>()->Find(name.c_str()))
		return l_buffer;
	if (auto l_texture = g_Engine->Get<TextureResourceService>()->Find(name.c_str()))
		return l_texture;

	Log(Error, "RenderGraphService: imported resource [", name.c_str(),
		"] not found in any resource service.");
	return nullptr;
}

bool RenderGraphService::CreateResource(const ResourceDesc& desc)
{
	// Imported resources are owned by an imperative pass; resolved live by name
	// at bind time, never created here.
	if (desc.m_Imported)
		return true;

	if (desc.m_Type == RenderGraphResourceType::Buffer)
	{
		auto l_buffer = g_Engine->Get<GPUBufferResourceService>()->Add(desc.m_Name.c_str());
		if (!l_buffer)
		{
			Log(Error, "RenderGraphService: failed to Add buffer [", desc.m_Name.c_str(), "].");
			return false;
		}

		l_buffer->m_ElementCount = desc.m_BufferDesc.m_ElementCount;
		l_buffer->m_ElementSize = desc.m_BufferDesc.m_ElementSize;
		l_buffer->m_Usage = desc.m_BufferDesc.m_Usage;
		l_buffer->m_CPUAccessibility = desc.m_BufferDesc.m_CPUAccessibility;
		l_buffer->m_GPUAccessibility = desc.m_BufferDesc.m_GPUAccessibility;

		m_Resources[desc.m_Name] = l_buffer;
		return true;
	}

	auto l_texture = g_Engine->Get<TextureResourceService>()->Add(desc.m_Name.c_str());
	if (!l_texture)
	{
		Log(Error, "RenderGraphService: failed to Add texture [", desc.m_Name.c_str(), "].");
		return false;
	}

	l_texture->m_TextureDesc = desc.m_TextureDesc;

	m_Resources[desc.m_Name] = l_texture;
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

	if (!desc.m_Writes.empty())
		l_node->m_PrimaryOutput = FindResource(desc.m_Writes[0]);

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
