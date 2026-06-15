#include "../RenderPassResourceService.h"
#include "../../Engine.h"
#include "../../Common/LogService.h"
#include "../FrameManagementService.h"
#include "../TextureResourceService.h"

using namespace Inno;

bool RenderPassResourceService::InitializeRenderPass(RenderPassComponent* renderPass)
{
	if (!CreateOutputMergerTargets(renderPass))
	{
		Log(Error, renderPass->m_InstanceName.c_str(), " CreateOutputMergerTargets failed; aborting.");
		return false;
	}

	if (!InitializeOutputMergerTargets(renderPass))
	{
		Log(Error, renderPass->m_InstanceName.c_str(), " InitializeOutputMergerTargets failed; aborting.");
		return false;
	}

	if (!OnOutputMergerTargetsCreated(renderPass))
	{
		Log(Error, renderPass->m_InstanceName.c_str(), " OnOutputMergerTargetsCreated failed; aborting.");
		return false;
	}

	renderPass->m_PipelineStateObject = AddPipelineStateObject();
	if (!CreatePipelineStateObject(renderPass))
	{
		Log(Error, renderPass->m_InstanceName.c_str(), " CreatePipelineStateObject failed; aborting.");
		return false;
	}

	Log(Verbose, renderPass->m_InstanceName.c_str(), " PipelineStateObject created.");

	renderPass->m_Semaphores.resize(g_Engine->Get<FrameManagementService>()->GetSwapChainImageCount());
	for (size_t i = 0; i < renderPass->m_Semaphores.size(); i++)
		renderPass->m_Semaphores[i] = AddSemaphore();

	Log(Verbose, renderPass->m_InstanceName.c_str(), " Semaphores created.");

	if (!CreateFenceEvents(renderPass))
	{
		Log(Error, renderPass->m_InstanceName.c_str(), " CreateFenceEvents failed; aborting.");
		return false;
	}

	Log(Verbose, renderPass->m_InstanceName.c_str(), " completed successfully.");
	return true;
}

bool RenderPassResourceService::CreateOutputMergerTargets(RenderPassComponent* renderPass)
{
	Log(Verbose, " ", renderPass->m_InstanceName.c_str(), " starting.");

	if (renderPass->m_RenderPassDesc.m_RenderTargetsCreationFunc)
	{
		Log(Verbose, "Calling customized render targets reservation function for: ", renderPass->m_InstanceName.c_str());
		if (!renderPass->m_RenderPassDesc.m_RenderTargetsCreationFunc())
		{
			Log(Error, " ", renderPass->m_InstanceName.c_str(), " custom render targets reservation returned false; aborting.");
			return false;
		}
	}
	else
	{
		Log(Verbose, " ", renderPass->m_InstanceName.c_str(), " using default render targets path (no custom func).");
		if (!renderPass->m_OutputMergerTarget)
		{
			Log(Verbose, " ", renderPass->m_InstanceName.c_str(), " allocating new OutputMergerTarget (none attached).");
			if (!Add(renderPass->m_OutputMergerTarget))
			{
				Log(Error, " ", renderPass->m_InstanceName.c_str(), " Add(OutputMergerTarget) failed; aborting.");
				return false;
			}
		}
		else
		{
			Log(Warning, " Trying to add output merger target(s) for ", renderPass->m_InstanceName.c_str(), " again");
		}

		auto l_textureService = g_Engine->Get<TextureResourceService>();
		auto l_outputMergerTarget = renderPass->m_OutputMergerTarget;
		l_outputMergerTarget->m_ColorOutputs.resize(renderPass->m_RenderPassDesc.m_RenderTargetCount);
		Log(Verbose, " ", renderPass->m_InstanceName.c_str(), " resized ColorOutputs to ", l_outputMergerTarget->m_ColorOutputs.size(), ".");
		for (size_t i = 0; i < l_outputMergerTarget->m_ColorOutputs.size(); i++)
		{
			auto& l_renderTarget = l_outputMergerTarget->m_ColorOutputs[i];
			l_renderTarget = l_textureService->Add((std::string(renderPass->m_InstanceName.c_str()) + "_RT_" + std::to_string(i)).c_str());
			if (!l_renderTarget)
			{
				Log(Error, " ", renderPass->m_InstanceName.c_str(), " TextureService->Add returned null for color RT ", i, "; aborting.");
				return false;
			}
			Log(Verbose, "Render target: ", l_renderTarget->m_InstanceName.c_str(), " has been allocated at: ", l_renderTarget);
		}
	}

	if (renderPass->m_RenderPassDesc.m_DepthStencilRenderTargetsCreationFunc)
	{
		Log(Verbose, "Calling customized depth-stencil render target reservation function for: ", renderPass->m_InstanceName.c_str());
		renderPass->m_RenderPassDesc.m_DepthStencilRenderTargetsCreationFunc();
	}
	else if (renderPass->m_RenderPassDesc.m_UseDepthBuffer)
	{
		Log(Verbose, " ", renderPass->m_InstanceName.c_str(), " using default depth-stencil path (m_UseDepthBuffer=true).");
		auto l_textureService = g_Engine->Get<TextureResourceService>();
		auto l_outputMergerTarget = renderPass->m_OutputMergerTarget;
		auto& l_depthStencilRenderTarget = l_outputMergerTarget->m_DepthStencilOutput;
		l_depthStencilRenderTarget = l_textureService->Add((std::string(renderPass->m_InstanceName.c_str()) + "_DS").c_str());
		if (!l_depthStencilRenderTarget)
		{
			Log(Error, " ", renderPass->m_InstanceName.c_str(), " TextureService->Add returned null for depth-stencil RT; aborting.");
			return false;
		}
		Log(Verbose, renderPass->m_InstanceName.c_str(), " depth stencil target has been allocated.");
	}
	else
	{
		Log(Verbose, " ", renderPass->m_InstanceName.c_str(), " no depth-stencil target (m_UseDepthBuffer=false and no custom func).");
	}

	Log(Verbose, " ", renderPass->m_InstanceName.c_str(), " finished successfully.");
	return true;
}

bool RenderPassResourceService::InitializeOutputMergerTargets(RenderPassComponent* renderPass)
{
	Log(Verbose, " ", renderPass->m_InstanceName.c_str(), " starting.");
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

			if (!l_textureService->InitializeSynchronous(l_renderTarget, nullptr))
			{
				Log(Error, " ", renderPass->m_InstanceName.c_str(), " InitializeSynchronous failed for color RT ", i, "; aborting.");
				return false;
			}
		}

		Log(Verbose, "Render target: ", renderPass->m_InstanceName.c_str(), " have been created.");
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

		if (!l_textureService->InitializeSynchronous(l_depthStencilRenderTarget, nullptr))
		{
			Log(Error, " ", renderPass->m_InstanceName.c_str(), " InitializeSynchronous failed for depth-stencil RT; aborting.");
			return false;
		}

		Log(Verbose, renderPass->m_InstanceName.c_str(), " depth stencil target has been created.");
	}
	else
	{
		Log(Verbose, " ", renderPass->m_InstanceName.c_str(), " no depth-stencil target (m_UseDepthBuffer=false and no custom func).");
	}

	Log(Verbose, " ", renderPass->m_InstanceName.c_str(), " finished successfully.");
	return true;
}

bool RenderPassResourceService::DeleteRenderTargets(RenderPassComponent* renderPass)
{
	Log(Verbose, " ", renderPass->m_InstanceName.c_str(), " starting.");

	if (renderPass->m_OutputMergerTarget)
	{
		Log(Verbose, " ", renderPass->m_InstanceName.c_str(), " deleting OutputMergerTarget.");
		if (!Delete(renderPass->m_OutputMergerTarget))
		{
			Log(Error, " ", renderPass->m_InstanceName.c_str(), " Delete(OutputMergerTarget) failed; aborting.");
			return false;
		}
		renderPass->m_OutputMergerTarget = nullptr;
	}
	else
	{
		Log(Verbose, " ", renderPass->m_InstanceName.c_str(), " no OutputMergerTarget to delete.");
	}

	if (renderPass->m_PipelineStateObject)
	{
		Log(Verbose, " ", renderPass->m_InstanceName.c_str(), " deleting PipelineStateObject.");
		if (!Delete(renderPass->m_PipelineStateObject))
		{
			Log(Error, " ", renderPass->m_InstanceName.c_str(), " Delete(PipelineStateObject) failed; aborting.");
			return false;
		}
		renderPass->m_PipelineStateObject = nullptr;
	}
	else
	{
		Log(Verbose, " ", renderPass->m_InstanceName.c_str(), " no PipelineStateObject to delete.");
	}

	Log(Verbose, " ", renderPass->m_InstanceName.c_str(), " finished successfully.");
	return true;
}
