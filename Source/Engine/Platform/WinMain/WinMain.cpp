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

	if (!InnoApplication::Setup(hInstance, nullptr, pScmdline))
	{
		return 0;
	}
	if (!InnoApplication::Initialize())
	{
		return 0;
	}
	if (!InnoApplication::Run())
	{
		InnoApplication::Terminate();
		return 0;
	}

	return 0;
}