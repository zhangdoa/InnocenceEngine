#include "stdafx.h"
#include "interface/IApplication.h"

extern IApplication* g_pApp;

int main()
{
	g_pApp->setup();
	g_pApp->initialize();

	while (g_pApp->getStatus() == objectStatus::ALIVE)
	{
		g_pApp->update();
	}

	g_pApp->shutdown();

	return 0;
}
