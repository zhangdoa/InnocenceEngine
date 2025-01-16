#include <windows.h>
#include <windowsx.h>

#include "../../Common/STL14.h"
#include "../../Engine.h"

using namespace Inno;
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR pScmdline, int nCmdshow)
{
	AllocConsole();

	errno_t err;
	FILE* stream;
	err = freopen_s(&stream, "CONOUT$", "w", stdout);
	SetConsoleTitle("Innocence Engine Debug Console");
	
	std::unique_ptr<Engine> m_pEngine = std::make_unique<Engine>();

	if (!m_pEngine->Setup(hInstance, nullptr, pScmdline))
	{
		return 0;
	}

	if (!m_pEngine->Initialize())
	{
		return 0;
	}

	m_pEngine->Run();

	m_pEngine->Terminate();

	return 0;
}