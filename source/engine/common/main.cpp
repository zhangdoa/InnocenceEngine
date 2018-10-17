#include "stdafx.h"
#include "config.h"
#include "InnoApplication.h"
#include "../component/WindowSystemSingletonComponent.h"

#if defined(INNO_RENDERER_DX)
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR pScmdline, int nCmdshow)
{
	AllocConsole();
	freopen("CONOUT$", "w+", stdout);
	WindowSystemSingletonComponent::getInstance().m_hInstance = hInstance;
	WindowSystemSingletonComponent::getInstance().m_pScmdline = pScmdline;
	WindowSystemSingletonComponent::getInstance().m_nCmdshow = nCmdshow;
#else
int main()
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