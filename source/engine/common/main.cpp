#include "stdafx.h"
#include "config.h"
#include "InnoApplication.h"

#if defined(INNO_RENDERER_DX)
#include "../component/DXWindowSystemSingletonComponent.h"
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR pScmdline, int nCmdshow)
{
	AllocConsole();
	freopen("CONOUT$", "w+", stdout);
	DXWindowSystemSingletonComponent::getInstance().m_hInstance = hInstance;
	DXWindowSystemSingletonComponent::getInstance().m_pScmdline = pScmdline;
	DXWindowSystemSingletonComponent::getInstance().m_nCmdshow = nCmdshow;
#else
int main(int argc, char *argv[])
{
#endif
	InnoApplication::setup();
	InnoApplication::initialize();

	while (InnoApplication::getStatus() == objectStatus::ALIVE)
	{
		InnoApplication::update();
	}

	InnoApplication::shutdown();

	return 0;
}