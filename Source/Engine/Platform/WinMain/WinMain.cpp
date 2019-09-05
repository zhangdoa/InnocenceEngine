#include <windows.h>
#include <windowsx.h>

#include "../ApplicationEntry/InnoApplicationEntry.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR pScmdline, int nCmdshow)
{
	AllocConsole();

	errno_t err;
	FILE *stream;
	err = freopen_s(&stream, "CONOUT$", "w", stdout);
	SetConsoleTitle("Innocence Engine Debug Console");

	if (!InnoApplicationEntry::Setup(hInstance, nullptr, pScmdline))
	{
		return 0;
	}
	if (!InnoApplicationEntry::Initialize())
	{
		return 0;
	}
	InnoApplicationEntry::Run();
	InnoApplicationEntry::Terminate();
	return 0;
}