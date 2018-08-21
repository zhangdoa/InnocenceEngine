#include "stdafx.h"
#include "interface/IApplication.h"
#include "config.h"

extern IApplication* g_pApp;

#if defined(INNO_PLATFORM_WIN32) || defined(INNO_PLATFORM_WIN64)
#include <windows.h>
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR pScmdline, int nCmdshow)
{
	g_pApp->setup(hInstance, pScmdline, nCmdshow);
#else
int main()
{
	g_pApp->setup();
#endif
	g_pApp->initialize();

	while (g_pApp->getStatus() == objectStatus::ALIVE)
	{
		g_pApp->update();
	}

	g_pApp->shutdown();

	return 0;
}