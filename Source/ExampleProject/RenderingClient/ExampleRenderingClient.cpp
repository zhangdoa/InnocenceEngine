#include "ExampleRenderingClient.h"

#include "../../Engine/Common/Object.h"

using namespace Inno;

bool ExampleRenderingClient::Initialize()
{
	m_ObjectStatus = ObjectStatus::Activated;
	return true;
}

bool ExampleRenderingClient::Terminate()
{
	m_ObjectStatus = ObjectStatus::Terminated;
	return true;
}

ObjectStatus ExampleRenderingClient::GetStatus()
{
	return m_ObjectStatus;
}
