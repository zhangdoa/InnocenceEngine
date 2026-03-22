#include "TestLogicClient.h"

using namespace Inno;

bool TestLogicClient::Setup(IServiceConfig*)
{
    m_ObjectStatus = ObjectStatus::Activated;
    return true;
}

bool TestLogicClient::Initialize() { return true; }
bool TestLogicClient::Update()     { return true; }

bool TestLogicClient::Terminate()
{
    m_ObjectStatus = ObjectStatus::Terminated;
    return true;
}

ObjectStatus TestLogicClient::GetStatus() { return m_ObjectStatus; }
const char* TestLogicClient::GetApplicationName() { return "RenderTest"; }
