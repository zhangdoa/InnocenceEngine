#include <windows.h>
#include <windowsx.h>
#include <iostream>

#include "../../Common/STL14.h"
#include "../../Engine.h"

using namespace Inno;

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR pScmdline, int nCmdshow)
{
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