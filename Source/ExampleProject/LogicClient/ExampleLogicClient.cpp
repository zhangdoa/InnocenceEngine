#include "ExampleLogicClient.h"

#include "ExampleLogicClientImpl.inl"

bool ExampleLogicClient::Setup(IServiceConfig* systemConfig)
{
	return GetImpl()->Setup(systemConfig);
}

bool ExampleLogicClient::Initialize()
{
	return GetImpl()->Initialize();
}

bool ExampleLogicClient::Update()
{
	return GetImpl()->Update();
}

bool ExampleLogicClient::Terminate()
{
	if (GetImpl()->Terminate())
	{
		delete m_Impl;
		return true;
	}

	return false;
}

ObjectStatus ExampleLogicClient::GetStatus()
{
	return GetImpl()->GetStatus();
}

const char* Inno::ExampleLogicClient::GetApplicationName()
{
	return GetImpl()->GetApplicationName();
}

ExampleLogicClientImpl* Inno::ExampleLogicClient::GetImpl()
{
	if (!m_Impl)
		m_Impl = new ExampleLogicClientImpl();

	return m_Impl;
}
