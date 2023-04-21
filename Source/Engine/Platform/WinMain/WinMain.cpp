#include <windows.h>
#include <windowsx.h>

#include "../ApplicationEntry/ApplicationEntry.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR pScmdline, int nCmdshow)
{
	AllocConsole();

	errno_t err;
	FILE* stream;
	err = freopen_s(&stream, "CONOUT$", "w", stdout);
	SetConsoleTitle("cence Engine Debug Console");

	if (!Inno::ApplicationEntry::Setup(hInstance, nullptr, pScmdline))
	{
		return 0;
	}
	if (!Inno::ApplicationEntry::Initialize())
	{
		return 0;
	}
	Inno::ApplicationEntry::Run();
	Inno::ApplicationEntry::Terminate();
	return 0;
}