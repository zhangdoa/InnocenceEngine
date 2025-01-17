#include "Helper.h"
#include "../../Common/LogService.h"
#include "../../Common/IOService.h"
#include "../IRenderingServer.h"

#include "../../Engine.h"

using namespace Inno;
;

bool RenderingServerHelper::ReserveRenderTargets(RenderPassComponent* RenderPassComp, IRenderingServer* renderingServer)
{
	if (RenderPassComp->m_RenderPassDesc.m_RenderTargetsReservationFunc)
	{
		Log(Verbose, "Calling customized render targets reservation function for: ", RenderPassComp->m_InstanceName.c_str());
		RenderPassComp->m_RenderPassDesc.m_RenderTargetsReservationFunc();
	}
	else
	{
		RenderPassComp->m_RenderTargets.resize(RenderPassComp->m_RenderPassDesc.m_RenderTargetCount);
		for (size_t i = 0; i < RenderPassComp->m_RenderPassDesc.m_RenderTargetCount; i++)
		{
			RenderPassComp->m_RenderTargets[i].m_IsOwned = true;
			RenderPassComp->m_RenderTargets[i].m_Texture = renderingServer->AddTextureComponent((std::string(RenderPassComp->m_InstanceName.c_str()) + "_RT_" + std::to_string(i) + "/").c_str());
			Log(Verbose, "Render target: ", RenderPassComp->m_RenderTargets[i].m_Texture->m_InstanceName.c_str(), " has been allocated at: ", RenderPassComp->m_RenderTargets[i].m_Texture);
		}		
	}

	if (RenderPassComp->m_RenderPassDesc.m_DepthStencilRenderTargetsReservationFunc)
	{
		Log(Verbose, "Calling customized depth-stencil render target reservation function for: ", RenderPassComp->m_InstanceName.c_str());
		RenderPassComp->m_RenderPassDesc.m_DepthStencilRenderTargetsReservationFunc();
	}
	else
	{
		if (RenderPassComp->m_RenderPassDesc.m_UseDepthBuffer)
		{
			RenderPassComp->m_DepthStencilRenderTarget.m_IsOwned = true;
			RenderPassComp->m_DepthStencilRenderTarget.m_Texture = renderingServer->AddTextureComponent((std::string(RenderPassComp->m_InstanceName.c_str()) + "_DS/").c_str());
			Log(Verbose, "", RenderPassComp->m_InstanceName.c_str(), " depth stencil target has been allocated.");
		}
	}

	return true;
}

bool RenderingServerHelper::CreateRenderTargets(RenderPassComponent* RenderPassComp, IRenderingServer* renderingServer)
{
	if (RenderPassComp->m_RenderPassDesc.m_RenderTargetsCreationFunc)
	{		
		Log(Verbose, "Calling customized render targets creation function for: ", RenderPassComp->m_InstanceName.c_str());
		RenderPassComp->m_RenderPassDesc.m_RenderTargetsCreationFunc();
	}
	else
	{
		for (size_t i = 0; i < RenderPassComp->m_RenderPassDesc.m_RenderTargetCount; i++)
		{
			auto l_TextureComp = RenderPassComp->m_RenderTargets[i].m_Texture;
			l_TextureComp->m_TextureDesc = RenderPassComp->m_RenderPassDesc.m_RenderTargetDesc;
			l_TextureComp->m_TextureData = nullptr;

			renderingServer->InitializeTextureComponent(l_TextureComp);

			Log(Verbose, "Render target: ", RenderPassComp->m_RenderTargets[i].m_Texture->m_InstanceName.c_str(), " has been initialized.");
		}	
	}


	if (RenderPassComp->m_RenderPassDesc.m_DepthStencilRenderTargetsCreationFunc)
	{	
		Log(Verbose, "Calling customized depth-stencil render target reservation function for: ", RenderPassComp->m_InstanceName.c_str());
		RenderPassComp->m_RenderPassDesc.m_DepthStencilRenderTargetsCreationFunc();
	}
	else
	{
		if (RenderPassComp->m_RenderPassDesc.m_UseDepthBuffer)
		{
			RenderPassComp->m_DepthStencilRenderTarget.m_Texture->m_TextureDesc = RenderPassComp->m_RenderPassDesc.m_RenderTargetDesc;

			if (RenderPassComp->m_RenderPassDesc.m_UseStencilBuffer)
			{
				RenderPassComp->m_DepthStencilRenderTarget.m_Texture->m_TextureDesc.Usage = TextureUsage::DepthStencilAttachment;
				RenderPassComp->m_DepthStencilRenderTarget.m_Texture->m_TextureDesc.PixelDataType = TexturePixelDataType::Float32;
				RenderPassComp->m_DepthStencilRenderTarget.m_Texture->m_TextureDesc.PixelDataFormat = TexturePixelDataFormat::DepthStencil;
			}
			else
			{
				RenderPassComp->m_DepthStencilRenderTarget.m_Texture->m_TextureDesc.Usage = TextureUsage::DepthAttachment;
				RenderPassComp->m_DepthStencilRenderTarget.m_Texture->m_TextureDesc.PixelDataType = TexturePixelDataType::Float32;
				RenderPassComp->m_DepthStencilRenderTarget.m_Texture->m_TextureDesc.PixelDataFormat = TexturePixelDataFormat::Depth;
			}

			RenderPassComp->m_DepthStencilRenderTarget.m_Texture->m_TextureData = nullptr;

			renderingServer->InitializeTextureComponent(RenderPassComp->m_DepthStencilRenderTarget.m_Texture);
			Log(Verbose, "", RenderPassComp->m_InstanceName.c_str(), " depth stencil target has been initialized.");
		}
	}

	return true;
}
