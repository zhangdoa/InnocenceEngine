#include "stdafx.h"
#include "config.h"
#include "InnoApplication.h"

#if defined(INNO_PLATFORM_WIN32) || defined(INNO_PLATFORM_WIN64)
#include <windows.h>
#include <windowsx.h>
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR pScmdline, int nCmdshow)
{
	AllocConsole();
	freopen("CONOUT$", "w+", stdout);
	if (!InnoApplication::setup(hInstance, hPrevInstance, pScmdline, nCmdshow))
#else
int main(int argc, char *argv[])
{
	if (!InnoApplication::setup(nullptr, nullptr, nullptr, 0))
#endif
	{
		return 0;
	}
	if (!InnoApplication::initialize())
	{
		return 0;
	}
	while (InnoApplication::getStatus() == ObjectStatus::ALIVE)
	{
		if (!InnoApplication::update())
		{
			InnoApplication::terminate();
			return 0;
		}
	}

	return 0;
}
