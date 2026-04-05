#include "EditorService.h"
#include "../Engine.h"
#include "../Common/LogService.h"

using namespace Inno;

bool EditorService::Setup(IServiceConfig* config)
{
	m_ObjectStatus = ObjectStatus::Created;
	Log(Success, "EditorService: Setup finished.");
	return true;
}

bool EditorService::Initialize()
{
	m_ObjectStatus = ObjectStatus::Activated;
	Log(Success, "EditorService: Initialized.");
	return true;
}

bool EditorService::Update()
{
	return true;
}

bool EditorService::Terminate()
{
	m_ObjectStatus = ObjectStatus::Terminated;
	Log(Success, "EditorService: Terminated.");
	return true;
}

ObjectStatus EditorService::GetStatus()
{
	return m_ObjectStatus;
}
