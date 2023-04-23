#include "Helper.h"
#include "../../Core/Logger.h"
#include "../../Core/IOService.h"
#include "../IRenderingServer.h"

#include "../../Interface/IEngine.h"

using namespace Inno;
extern IEngine* g_Engine;

bool RenderingServerHelper::ReserveRenderTargets(RenderPassComponent* RenderPassComp, IRenderingServer* renderingServer)
{
	if (RenderPassComp->m_RenderPassDesc.m_RenderTargetsReservationFunc)
	{
		RenderPassComp->m_RenderPassDesc.m_RenderTargetsReservationFunc();
	}
	else
	{
		RenderPassComp->m_RenderTargets.reserve(RenderPassComp->m_RenderPassDesc.m_RenderTargetCount);
		for (size_t i = 0; i < RenderPassComp->m_RenderPassDesc.m_RenderTargetCount; i++)
		{
			RenderPassComp->m_RenderTargets.emplace_back();
			RenderPassComp->m_RenderTargets[i] = renderingServer->AddTextureComponent((std::string(RenderPassComp->m_InstanceName.c_str()) + "_RT_" + std::to_string(i) + "/").c_str());
		}
		Logger::Log(LogLevel::Verbose, "RenderingServer: ", RenderPassComp->m_InstanceName.c_str(), " render targets have been allocated.");
	}

	if (RenderPassComp->m_RenderPassDesc.m_DepthStencilRenderTargetsReservationFunc)
	{
		RenderPassComp->m_RenderPassDesc.m_DepthStencilRenderTargetsReservationFunc();
	}
	else
	{
		if (RenderPassComp->m_RenderPassDesc.m_UseDepthBuffer)
		{
			RenderPassComp->m_DepthStencilRenderTarget = renderingServer->AddTextureComponent((std::string(RenderPassComp->m_InstanceName.c_str()) + "_DS/").c_str());
			Logger::Log(LogLevel::Verbose, "RenderingServer: ", RenderPassComp->m_InstanceName.c_str(), " depth stencil target has been allocated.");
		}
	}

	return true;
}

bool RenderingServerHelper::CreateRenderTargets(RenderPassComponent* RenderPassComp, IRenderingServer* renderingServer)
{
	if (RenderPassComp->m_RenderPassDesc.m_RenderTargetsCreationFunc)
	{
		RenderPassComp->m_RenderPassDesc.m_RenderTargetsCreationFunc();
	}
	else
	{
		for (size_t i = 0; i < RenderPassComp->m_RenderPassDesc.m_RenderTargetCount; i++)
		{
			auto l_TextureComp = RenderPassComp->m_RenderTargets[i];

			l_TextureComp->m_TextureDesc = RenderPassComp->m_RenderPassDesc.m_RenderTargetDesc;

			l_TextureComp->m_TextureData = nullptr;

			renderingServer->InitializeTextureComponent(l_TextureComp);
		}

		Logger::Log(LogLevel::Verbose, "RenderingServer: ", RenderPassComp->m_InstanceName.c_str(), " render targets have been initialized.");
	}


	if (RenderPassComp->m_RenderPassDesc.m_DepthStencilRenderTargetsCreationFunc)
	{
		RenderPassComp->m_RenderPassDesc.m_DepthStencilRenderTargetsCreationFunc();
	}
	else
	{
		if (RenderPassComp->m_RenderPassDesc.m_UseDepthBuffer)
		{
			RenderPassComp->m_DepthStencilRenderTarget->m_TextureDesc = RenderPassComp->m_RenderPassDesc.m_RenderTargetDesc;

			if (RenderPassComp->m_RenderPassDesc.m_UseStencilBuffer)
			{
				RenderPassComp->m_DepthStencilRenderTarget->m_TextureDesc.Usage = TextureUsage::DepthStencilAttachment;
				RenderPassComp->m_DepthStencilRenderTarget->m_TextureDesc.PixelDataType = TexturePixelDataType::Float32;
				RenderPassComp->m_DepthStencilRenderTarget->m_TextureDesc.PixelDataFormat = TexturePixelDataFormat::DepthStencil;
			}
			else
			{
				RenderPassComp->m_DepthStencilRenderTarget->m_TextureDesc.Usage = TextureUsage::DepthAttachment;
				RenderPassComp->m_DepthStencilRenderTarget->m_TextureDesc.PixelDataType = TexturePixelDataType::Float32;
				RenderPassComp->m_DepthStencilRenderTarget->m_TextureDesc.PixelDataFormat = TexturePixelDataFormat::Depth;
			}

			RenderPassComp->m_DepthStencilRenderTarget->m_TextureData = nullptr;

			renderingServer->InitializeTextureComponent(RenderPassComp->m_DepthStencilRenderTarget);
			Logger::Log(LogLevel::Verbose, "RenderingServer: ", RenderPassComp->m_InstanceName.c_str(), " depth stencil target has been initialized.");
		}
	}

	return true;
}
