#include "stdafx.h"
#include "config.h"
#include "InnoApplication.h"

#if defined(INNO_RENDERER_DX)
#include <windows.h>
#include <windowsx.h>
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR pScmdline, int nCmdshow)
{
	AllocConsole();
	freopen("CONOUT$", "w+", stdout);
	if (!InnoApplication::setup(hInstance, hPrevInstance, pScmdline, nCmdshow))
	{
		return 0;
	}
#else
int main(int argc, char *argv[])
{
	if (!InnoApplication::setup())
	{
		return 0;
	}
#endif
	if (!InnoApplication::initialize())
	{
		return 0;
	}
	while (InnoApplication::getStatus() == objectStatus::ALIVE)
	{
		if (!InnoApplication::update())
		{
			return 0;
		}
	}

	if (!InnoApplication::terminate())
	{
		return 0;
	}

	return 0;
}