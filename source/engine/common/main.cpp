#include "stdafx.h"
#include "config.h"
#include "InnoApplication.h"

#if defined(INNO_PLATFORM_WIN)
#include <windows.h>
#include <windowsx.h>
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR pScmdline, int nCmdshow)
{
	AllocConsole();

	errno_t err;
	FILE *stream;
	err = freopen_s(&stream, "CONOUT$", "w", stdout);
	SetConsoleTitle("Innocence Engine Debug Console");

	if (!InnoApplication::setup(hInstance, hPrevInstance, pScmdline, nCmdshow))
#else
int main(int argc, char *argv[])
{
	if (!InnoApplication::setup(nullptr, nullptr, argv[1], 0))
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