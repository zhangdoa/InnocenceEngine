#include "DefaultLogicClient.h"

#include "DefaultLogicClientImpl.inl"

bool DefaultLogicClient::Setup(ISystemConfig* systemConfig)
{
	return GetImpl()->Setup(systemConfig);
}

bool DefaultLogicClient::Initialize()
{
	return GetImpl()->Initialize();
}

bool DefaultLogicClient::Update()
{
	return GetImpl()->Update();
}

bool DefaultLogicClient::Terminate()
{
	if (GetImpl()->Terminate())
	{
		delete m_Impl;
		return true;
	}

	return false;
}

ObjectStatus DefaultLogicClient::GetStatus()
{
	return GetImpl()->GetStatus();
}

std::string Inno::DefaultLogicClient::getApplicationName()
{
	return GetImpl()->getApplicationName();
}

DefaultLogicClientImpl* Inno::DefaultLogicClient::GetImpl()
{
	if (!m_Impl)
		m_Impl = new DefaultLogicClientImpl();

	return m_Impl;
}
