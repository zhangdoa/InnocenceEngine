#include <windows.h>
#include <windowsx.h>

#include "../../Common/InnoApplication.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR pScmdline, int nCmdshow)
{
	AllocConsole();

	errno_t err;
	FILE *stream;
	err = freopen_s(&stream, "CONOUT$", "w", stdout);
	SetConsoleTitle("Innocence Engine Debug Console");

	if (!InnoApplication::setup(hInstance, nullptr, pScmdline))
	{
		return 0;
	}
	if (!InnoApplication::initialize())
	{
		return 0;
	}
	while (InnoApplication::getStatus() == ObjectStatus::Activated)
	{
		if (!InnoApplication::update())
		{
			InnoApplication::terminate();
			return 0;
		}
	}

	return 0;
}