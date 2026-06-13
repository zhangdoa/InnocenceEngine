#include "RenderGraphService.h"

#include "../Engine.h"
#include "../Services/RenderingConfigurationService.h"
#include "../Services/TextureResourceService.h"
#include "../Services/GPUBufferResourceService.h"
#include "../Services/SamplerResourceService.h"
#include "../Services/PerFrameDataService.h"
#include "../Services/DrawCallService.h"
#include "../Services/FrameManagementService.h"

using namespace Inno;

namespace
{
	// Built-in dynamic resource: PerFrameCBuffer is double-buffered
	// (GetCurrentFrameBuffer() alternates by frame-parity), so the graph re-resolves
	// this name through that accessor every RecordNode rather than pinning one handle
	// via static import-by-name (which would go stale every other frame).
	const char* const g_PerFrameCBufferName = "PerFrameCBuffer";
	// Both double-buffered like PerFrameCBuffer: the prev-frame per-frame CBuffer
	// and the current-frame transform CBuffer alternate by frame-parity, so they
	// must re-resolve through their accessor each frame, never pin by name.
	const char* const g_PerFrameCBufferPrevName = "PerFrameCBufferPrev";
	const char* const g_TransformBufferName = "TransformBuffer";
	// The engine-owned scene TLAS (built per frame by FrameManagementService /
	// the mesh service). Bound by name; DispatchRays self-guards on IsTLASReady.
	const char* const g_TLASName = "TLAS";
}

GPUResourceComponent* RenderGraphService::FindResource(const std::string& name)
{
	// Re-resolve per frame via the frame-parity accessor: RecordNode calls
	// FindResource each frame, so this hands back the correct double-buffered
	// handle without per-pass C++.
	if (name == g_PerFrameCBufferName)
		return g_Engine->Get<PerFrameDataService>()->GetCurrentFrameBuffer();
	if (name == g_PerFrameCBufferPrevName)
		return g_Engine->Get<PerFrameDataService>()->GetPreviousFrameBuffer();
	if (name == g_TransformBufferName)
		return g_Engine->Get<DrawCallService>()->GetCurrentFrameTransformBuffer();
	if (name == g_TLASName)
		return g_Engine->Get<GPUBufferResourceService>()->GetTLASBuffer();

	// A plain read of a ping-pong name resolves to the current-frame output
	// (parity with the imperative pass's GetResult()).
	if (auto l_pingPong = PingPongTexture(name, false))
		return l_pingPong;

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
		g_Engine->Get<GPUBufferResourceService>()->Initialize(l_buffer);

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
	g_Engine->Get<TextureResourceService>()->Initialize(l_texture);

	m_Resources[desc.m_Name] = l_texture;
	return true;
}

bool RenderGraphService::CreateScreenSizedTexture(const ResourceDesc& desc)
{
	auto l_textureService = g_Engine->Get<TextureResourceService>();

	// Ping-pong: (re)create BOTH parity textures. The logical name stays out of
	// m_Resources (resolved per-frame via m_PingPong); the physical names go in so
	// resize can delete them and a physical-name lookup still works.
	if (desc.m_PingPong)
	{
		auto l_res = g_Engine->Get<RenderingConfigurationService>()->GetScreenResolution();
		auto l_make = [&](const std::string& physName) -> TextureComponent*
		{
			auto l_prior = m_Resources.find(physName);
			if (l_prior != m_Resources.end() && l_prior->second)
				l_textureService->Delete(static_cast<TextureComponent*>(l_prior->second));
			auto l_tex = l_textureService->Add(physName.c_str());
			if (!l_tex)
			{
				Log(Error, "RenderGraphService: failed to Add ping-pong texture [", physName.c_str(), "].");
				return nullptr;
			}
			l_tex->m_TextureDesc = desc.m_TextureDesc;
			l_tex->m_TextureDesc.Width = l_res.x;
			l_tex->m_TextureDesc.Height = l_res.y;
			l_textureService->Initialize(l_tex);
			m_Resources[physName] = l_tex;
			return l_tex;
		};
		auto l_even = l_make(desc.m_Name + " (Even)");
		auto l_odd = l_make(desc.m_Name + " (Odd)");
		if (!l_even || !l_odd)
			return false;
		m_PingPong[desc.m_Name] = { l_even, l_odd };
		return true;
	}

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

TextureComponent* RenderGraphService::PingPongTexture(const std::string& name, bool history)
{
	auto it = m_PingPong.find(name);
	if (it == m_PingPong.end())
		return nullptr;

	// Odd frame -> the current-frame output is the Odd texture; history is the
	// other parity (last frame's output). { first = Even, second = Odd }.
	bool l_odd = (g_Engine->Get<FrameManagementService>()->GetFrameCountSinceLaunch() % 2) == 1;
	bool l_wantOdd = history ? !l_odd : l_odd;
	return l_wantOdd ? it->second.second : it->second.first;
}

void RenderGraphService::CreateOrphanResources()
{
	// Screen-sized resources with no producing node have no RT-init-func to create
	// them; the factory does (zeroed input until a producer exists). Non-screen
	// resources are already created+initialized in CreateResource; writer-owned
	// screen RTs are created by the writer node's RT-init-func.
	for (const auto& l_desc : m_Desc.m_Resources)
	{
		if (l_desc.m_Imported || l_desc.m_SizeExpr != "screen")
			continue;

		bool l_hasWriter = false;
		for (const auto& l_pass : m_Desc.m_Passes)
		{
			for (const auto& l_write : l_pass.m_Writes)
				if (l_write == l_desc.m_Name) { l_hasWriter = true; break; }
			if (l_hasWriter)
				break;
		}
		if (!l_hasWriter)
			CreateScreenSizedTexture(l_desc);
	}
}
