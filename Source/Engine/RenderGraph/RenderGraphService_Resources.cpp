#include "RenderGraphService.h"

#include "../Engine.h"
#include "../Services/RenderingConfigurationService.h"
#include "../Services/TextureResourceService.h"
#include "../Services/GPUBufferResourceService.h"
#include "../Services/SamplerResourceService.h"
#include "../Services/PerFrameDataService.h"

using namespace Inno;

namespace
{
	// Built-in dynamic resource: PerFrameCBuffer is double-buffered
	// (GetCurrentFrameBuffer() alternates by frame-parity), so the graph re-resolves
	// this name through that accessor every RecordNode rather than pinning one handle
	// via static import-by-name (which would go stale every other frame).
	const char* const g_PerFrameCBufferName = "PerFrameCBuffer";
}

GPUResourceComponent* RenderGraphService::FindResource(const std::string& name)
{
	// Re-resolve per frame via the frame-parity accessor: RecordNode calls
	// FindResource each frame, so this hands back the correct double-buffered
	// handle without per-pass C++.
	if (name == g_PerFrameCBufferName)
		return g_Engine->Get<PerFrameDataService>()->GetCurrentFrameBuffer();

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
	if (auto l_sampler = g_Engine->Get<SamplerResourceService>()->Find(name.c_str()))
		return l_sampler;

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

	// Screen-sized textures are (re)created by the writer node's RT-init-func at
	// Initialize + resize, never eagerly here — defer until CreatePassNode wires
	// that hook (so resize flows through the engine's PostResize loop).
	if (desc.m_SizeExpr == "screen")
	{
		m_DeferredScreenTextures[desc.m_Name] = desc;
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

bool RenderGraphService::CreateScreenSizedTexture(const ResourceDesc& desc)
{
	auto l_textureService = g_Engine->Get<TextureResourceService>();

	// Resize re-invokes this func: delete the prior texture before recreating.
	auto it = m_Resources.find(desc.m_Name);
	if (it != m_Resources.end() && it->second)
		l_textureService->Delete(static_cast<TextureComponent*>(it->second));

	auto l_screenResolution = g_Engine->Get<RenderingConfigurationService>()->GetScreenResolution();

	auto l_texture = l_textureService->Add(desc.m_Name.c_str());
	if (!l_texture)
	{
		Log(Error, "RenderGraphService: failed to Add screen-sized texture [", desc.m_Name.c_str(), "].");
		return false;
	}

	l_texture->m_TextureDesc = desc.m_TextureDesc;
	l_texture->m_TextureDesc.Width = l_screenResolution.x;
	l_texture->m_TextureDesc.Height = l_screenResolution.y;

	l_textureService->Initialize(l_texture);

	m_Resources[desc.m_Name] = l_texture;
	return true;
}
