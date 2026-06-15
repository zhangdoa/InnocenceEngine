#include <windows.h>
#include <windowsx.h>
#include <iostream>

#include "../../Common/STL14.h"
#include "../../Engine.h"
#include "../../Services/ConfigurationService.h"
#include "../../Services/GraphicsHardwareService.h"
#include "../../Interface/IClientFactory.h"


#if defined(INNO_DEBUG) || defined(INNO_RELWITHDEBINFO)
#include <dbghelp.h>
#pragma comment(lib, "dbghelp.lib")
#endif

using namespace Inno;

#if defined(INNO_DEBUG) || defined(INNO_RELWITHDEBINFO)
LONG WINAPI UnhandledExceptionHandler(EXCEPTION_POINTERS* exceptionInfo)
{
    DWORD exceptionCode = exceptionInfo->ExceptionRecord->ExceptionCode;
    PVOID exceptionAddress = exceptionInfo->ExceptionRecord->ExceptionAddress;

    char errorMsg[2048];
    char stackTrace[1024] = {0};

    HANDLE process = GetCurrentProcess();
    HANDLE thread = GetCurrentThread();

    SymInitialize(process, NULL, TRUE);
    SymSetOptions(SYMOPT_LOAD_LINES | SYMOPT_DEFERRED_LOADS);

    SYMBOL_INFO* symbol = (SYMBOL_INFO*)calloc(sizeof(SYMBOL_INFO) + 256 * sizeof(char), 1);
    symbol->MaxNameLen = 255;
    symbol->SizeOfStruct = sizeof(SYMBOL_INFO);

    DWORD64 displacement = 0;
    char functionName[256] = "Unknown";

    if (SymFromAddr(process, (DWORD64)exceptionAddress, &displacement, symbol))
    {
        strcpy_s(functionName, sizeof(functionName), symbol->Name);
    }

    IMAGEHLP_LINE64 line;
    line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
    DWORD dwDisplacement = 0;
    char fileInfo[512] = "Unknown source";
    
    if (SymGetLineFromAddr64(process, (DWORD64)exceptionAddress, &dwDisplacement, &line))
    {
        sprintf_s(fileInfo, sizeof(fileInfo), "%s:%d", line.FileName, line.LineNumber);
    }
    
    sprintf_s(stackTrace, sizeof(stackTrace), 
        "\nFunction: %s\nSource: %s\nOffset: +0x%llX",
        functionName, fileInfo, displacement);
    
    free(symbol);
    SymCleanup(process);
    
    if (exceptionCode == EXCEPTION_ACCESS_VIOLATION)
    {
        PVOID faultAddress = (PVOID)exceptionInfo->ExceptionRecord->ExceptionInformation[1];
        BOOL isWrite = exceptionInfo->ExceptionRecord->ExceptionInformation[0];
        
        sprintf_s(errorMsg, sizeof(errorMsg),
            "ACCESS VIOLATION DETECTED!\n"
            "\nException thrown at 0x%p in Main.exe: 0xC0000005: Access violation %s location 0x%p\n"
            "\nDetailed Information:\n"
            "Exception Code: 0x%08X (EXCEPTION_ACCESS_VIOLATION)\n"
            "Exception Address: 0x%p (instruction that caused the crash)\n"
            "Fault Address: 0x%p (memory location being accessed)\n"
            "Operation: %s%s",
            exceptionAddress,
            isWrite ? "writing" : "reading",
            faultAddress,
            exceptionCode,
            exceptionAddress,
            faultAddress,
            isWrite ? "Writing to memory" : "Reading from memory",
            stackTrace
        );
    }
    else
    {
        sprintf_s(errorMsg, sizeof(errorMsg),
            "SYSTEM EXCEPTION DETECTED!\n"
            "Exception Code: 0x%08X\n"
            "Exception Address: 0x%p\n"
            "Exception thrown at 0x%p in Main.exe: 0x%08X%s",
            exceptionCode,
            exceptionAddress,
            exceptionAddress,
            exceptionCode,
            stackTrace
        );
    }

    Log(Error, errorMsg);

    // Fallback: Log() may not flush before ExitProcess.
    fprintf(stderr, "%s\n", errorMsg);
    fflush(stderr);

    ExitProcess(2);
    return EXCEPTION_EXECUTE_HANDLER;
}
#endif

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR pScmdline, int nCmdshow)
{
#if defined(INNO_DEBUG) || defined(INNO_RELWITHDEBINFO)
    SetUnhandledExceptionFilter(UnhandledExceptionHandler);
#endif

    try
    {
        std::unique_ptr<Engine> m_pEngine = std::make_unique<Engine>();

        // -bake implies headless; detect here so the clients aren't constructed only to be destroyed.
        bool l_isHeadless = (pScmdline && (strstr(pScmdline, "headless") != nullptr
                                        || strstr(pScmdline, "-bake") != nullptr));

        std::unique_ptr<IRenderingClient> l_renderingClient = l_isHeadless ? nullptr : Inno::CreateRenderingClient();
        IRenderingClient* l_renderingClientPtr = l_renderingClient.get();

        if (!m_pEngine->Setup(
            hInstance, nullptr, pScmdline,
            std::move(l_renderingClient),
            l_isHeadless ? nullptr : Inno::CreateLogicClient()))
            return 2;

        if (!m_pEngine->Initialize())
            return 2;

        // Normal mode: Run() returns false on clean shutdown (WM_CLOSE / stand-by).
        // Bake mode: Run() returns false only when an import actually failed.
        const bool l_runOK = m_pEngine->Run();

        m_pEngine->Terminate();

        // -serialize_test result takes precedence — the test runs without
        // rendering services so the GPU-error checks below don't apply.
        const std::string& l_serializeTest = m_pEngine->Get<ConfigurationService>()->GetSerializeTest();
        if (!l_serializeTest.empty())
            return m_pEngine->Get<ConfigurationService>()->GetSerializeTestResult();

        // GraphicsHardwareService only exists in non-headless mode.
        if (!l_isHeadless && m_pEngine->Get<GraphicsHardwareService>()->HasGPUError())
            return 1;

        if (l_isHeadless && !l_runOK)
            return 1;

        if (l_renderingClientPtr && !l_renderingClientPtr->GetValidationPassed())
            return 2;

        return 0;
    }
    catch (...)
    {
        return 2;
    }
}