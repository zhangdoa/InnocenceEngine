#include "../RenderPassResourceService.h"
#include "../../Engine.h"
#include "../../Common/LogService.h"
#include "../../Common/LogServiceSpecialization.h"
#include "../TextureResourceService.h"
#include "../FrameManagementService.h"

using namespace Inno;

bool RenderPassResourceService::Setup(IServiceConfig* systemConfig)
{
	m_Pool.Initialize(128);
	m_ObjectStatus = ObjectStatus::Created;
	return true;
}

bool RenderPassResourceService::Terminate()
{
	m_Pool.Terminate();
	m_ObjectStatus = ObjectStatus::Terminated;
	return true;
}

RenderPassComponent* RenderPassResourceService::Add(const char* name)
{
	return m_Pool.Allocate(name);
}

RenderPassComponent* RenderPassResourceService::Find(const char* name)
{
	return m_Pool.Find(name);
}

bool RenderPassResourceService::Delete(RenderPassComponent* ptr)
{
	m_Pool.Release(ptr);
	return true;
}

void RenderPassResourceService::ForEach(std::function<void(RenderPassComponent*)> func)
{
	m_Pool.ForEach(func);
}

void RenderPassResourceService::Initialize(RenderPassComponent* renderPass)
{
	if (renderPass->m_ObjectStatus == ObjectStatus::Activated)
		return;

	m_DeferredQueue.push(renderPass);
}

bool RenderPassResourceService::InitializeComponents()
{
	RenderPassComponent* l_renderPass = nullptr;
	while (m_DeferredQueue.tryPop(l_renderPass))
	{
		if (!l_renderPass)
			continue;

		if (InitializeRenderPass(l_renderPass))
			l_renderPass->m_ObjectStatus = ObjectStatus::Activated;
		else
			m_DeferredQueue.push(l_renderPass);
	}

	return true;
}

bool RenderPassResourceService::InitializeRenderPass(RenderPassComponent* renderPass)
{
	bool l_result = true;

	l_result &= CreateOutputMergerTargets(renderPass);
	l_result &= InitializeOutputMergerTargets(renderPass);
	l_result &= OnOutputMergerTargetsCreated(renderPass);

	renderPass->m_PipelineStateObject = AddPipelineStateObject();
	l_result &= CreatePipelineStateObject(renderPass);

	Log(Verbose, renderPass->m_InstanceName, " PipelineStateObject has been created.");

	renderPass->m_Semaphores.resize(g_Engine->Get<FrameManagementService>()->GetSwapChainImageCount());
	for (size_t i = 0; i < renderPass->m_Semaphores.size(); i++)
	{
		renderPass->m_Semaphores[i] = AddSemaphore();
	}

	Log(Verbose, renderPass->m_InstanceName, " Semaphore has been created.");

	CreateFenceEvents(renderPass);

	return l_result;
}

bool RenderPassResourceService::CreateOutputMergerTargets(RenderPassComponent* renderPass)
{
	if (renderPass->m_RenderPassDesc.m_RenderTargetsCreationFunc)
	{
		Log(Verbose, "Calling customized render targets reservation function for: ", renderPass->m_InstanceName.c_str());
		renderPass->m_RenderPassDesc.m_RenderTargetsCreationFunc();
	}
	else
	{
		if (!renderPass->m_OutputMergerTarget)
			Add(renderPass->m_OutputMergerTarget);

		auto l_textureService = g_Engine->Get<TextureResourceService>();
		auto l_outputMergerTarget = renderPass->m_OutputMergerTarget;
		l_outputMergerTarget->m_ColorOutputs.resize(renderPass->m_RenderPassDesc.m_RenderTargetCount);
		for (size_t i = 0; i < l_outputMergerTarget->m_ColorOutputs.size(); i++)
		{
			auto& l_renderTarget = l_outputMergerTarget->m_ColorOutputs[i];
			l_renderTarget = l_textureService->Add((std::string(renderPass->m_InstanceName.c_str()) + "_RT_" + std::to_string(i)).c_str());
			Log(Verbose, "Render target: ", l_renderTarget->m_InstanceName, " has been allocated at: ", l_renderTarget);
		}
	}

	if (renderPass->m_RenderPassDesc.m_DepthStencilRenderTargetsCreationFunc)
	{
		Log(Verbose, "Calling customized depth-stencil render target reservation function for: ", renderPass->m_InstanceName.c_str());
		renderPass->m_RenderPassDesc.m_DepthStencilRenderTargetsCreationFunc();
	}
	else if (renderPass->m_RenderPassDesc.m_UseDepthBuffer)
	{
		auto l_textureService = g_Engine->Get<TextureResourceService>();
		auto l_outputMergerTarget = renderPass->m_OutputMergerTarget;
		auto& l_depthStencilRenderTarget = l_outputMergerTarget->m_DepthStencilOutput;
		l_depthStencilRenderTarget = l_textureService->Add((std::string(renderPass->m_InstanceName.c_str()) + "_DS").c_str());
		Log(Verbose, renderPass->m_InstanceName.c_str(), " depth stencil target has been allocated.");
	}

	return true;
}

bool RenderPassResourceService::InitializeOutputMergerTargets(RenderPassComponent* renderPass)
{
	auto l_textureService = g_Engine->Get<TextureResourceService>();

	if (renderPass->m_RenderPassDesc.m_RenderTargetsInitializationFunc)
	{
		Log(Verbose, "Calling customized render targets creation function for: ", renderPass->m_InstanceName.c_str());
		renderPass->m_RenderPassDesc.m_RenderTargetsInitializationFunc();
	}
	else
	{
		auto l_outputMergerTarget = renderPass->m_OutputMergerTarget;
		const size_t l_colorOutputCount = l_outputMergerTarget->m_ColorOutputs.size();

		// Fail at declaration time on the pass-type / RT-Usage mismatch — the silent
		// path produces a GBV-only crash at first UAV/RTV bind. Skipped when there
		// are no color outputs or the pass installs custom init funcs (no copy of
		// the default desc onto the targets).
		if (l_colorOutputCount > 0)
		{
			const auto l_passEngine = renderPass->m_RenderPassDesc.m_GPUEngineType;
			const auto l_rtUsage    = renderPass->m_RenderPassDesc.m_RenderTargetDesc.Usage;
			if (l_passEngine == GPUEngineType::Compute && l_rtUsage == TextureUsage::ColorAttachment)
			{
				Log(Error, "RenderPass '", renderPass->m_InstanceName.c_str(),
					"' declares GPUEngineType::Compute but its RenderTargetDesc.Usage is "
					"ColorAttachment. A compute pass's UAV outputs must use "
					"TextureUsage::ComputeOnly — ColorAttachment maps to RENDER_TARGET "
					"layout and will crash on UAV binding.");
				return false;
			}
			if (l_passEngine == GPUEngineType::Graphics && l_rtUsage == TextureUsage::ComputeOnly)
			{
				Log(Error, "RenderPass '", renderPass->m_InstanceName.c_str(),
					"' declares GPUEngineType::Graphics but its RenderTargetDesc.Usage is "
					"ComputeOnly. A graphics pass's RTV outputs must use "
					"TextureUsage::ColorAttachment — ComputeOnly maps to UAV layout and "
					"will reject OMSetRenderTargets.");
				return false;
			}
		}

		for (size_t i = 0; i < l_outputMergerTarget->m_ColorOutputs.size(); i++)
		{
			auto l_renderTarget = l_outputMergerTarget->m_ColorOutputs[i];
			l_renderTarget->m_TextureDesc = renderPass->m_RenderPassDesc.m_RenderTargetDesc;

			l_textureService->InitializeSynchronous(l_renderTarget, nullptr);
		}

		Log(Verbose, "Render target: ", renderPass->m_InstanceName, " have been created.");
	}

	if (renderPass->m_RenderPassDesc.m_DepthStencilRenderTargetsInitializationFunc)
	{
		Log(Verbose, "Calling customized depth-stencil render target reservation function for: ", renderPass->m_InstanceName.c_str());
		renderPass->m_RenderPassDesc.m_DepthStencilRenderTargetsInitializationFunc();
	}
	else if (renderPass->m_RenderPassDesc.m_UseDepthBuffer)
	{
		auto l_outputMergerTarget = renderPass->m_OutputMergerTarget;
		auto l_depthStencilRenderTarget = l_outputMergerTarget->m_DepthStencilOutput;
		l_depthStencilRenderTarget->m_TextureDesc = renderPass->m_RenderPassDesc.m_RenderTargetDesc;

		if (renderPass->m_RenderPassDesc.m_UseStencilBuffer)
		{
			l_depthStencilRenderTarget->m_TextureDesc.Usage = TextureUsage::DepthStencilAttachment;
			l_depthStencilRenderTarget->m_TextureDesc.PixelDataType = TexturePixelDataType::Float32;
			l_depthStencilRenderTarget->m_TextureDesc.PixelDataFormat = TexturePixelDataFormat::DepthStencil;
		}
		else
		{
			l_depthStencilRenderTarget->m_TextureDesc.Usage = TextureUsage::DepthAttachment;
			l_depthStencilRenderTarget->m_TextureDesc.PixelDataType = TexturePixelDataType::Float32;
			l_depthStencilRenderTarget->m_TextureDesc.PixelDataFormat = TexturePixelDataFormat::Depth;
		}

		l_textureService->InitializeSynchronous(l_depthStencilRenderTarget, nullptr);

		Log(Verbose, renderPass->m_InstanceName, " depth stencil target has been created.");
	}

	return true;
}

bool RenderPassResourceService::DeleteRenderTargets(RenderPassComponent* renderPass)
{
	if (renderPass->m_OutputMergerTarget)
	{
		Delete(renderPass->m_OutputMergerTarget);
		renderPass->m_OutputMergerTarget = nullptr;
	}

	if (renderPass->m_PipelineStateObject)
	{
		Delete(renderPass->m_PipelineStateObject);
		renderPass->m_PipelineStateObject = nullptr;
	}

	return true;
}
