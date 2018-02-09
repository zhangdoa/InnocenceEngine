// InnocenceEngine.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "../engine/platform/Win32Application.h"

int main(int argc, char** argv)
{
	IApplication* g_pApp = new Win32Application();
	
	g_pApp->setup();
	g_pApp->initialize();

	while (g_pApp->getStatus() == objectStatus::ALIVE)
	{
		g_pApp->update();
	}
	g_pApp->shutdown();

	delete g_pApp;

	return 0;
}

