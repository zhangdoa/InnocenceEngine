#pragma once
#include "DefaultRenderingClient.h"
#include "DefaultRenderingClientImpl.inl"

bool DefaultRenderingClient::Setup(ISystemConfig *systemConfig)
{
	m_Impl = new DefaultRenderingClientImpl();
	return m_Impl->Setup(systemConfig);
}

bool DefaultRenderingClient::Initialize()
{
	return m_Impl->Initialize();
}

bool DefaultRenderingClient::Render(IRenderingConfig *renderingConfig)
{
	return m_Impl->Render(renderingConfig);
}

bool DefaultRenderingClient::Terminate()
{
	if(m_Impl->Terminate())
	{
		delete m_Impl;
		return true;
	}

	return false;
}

ObjectStatus DefaultRenderingClient::GetStatus()
{
	return m_Impl->GetStatus();
}