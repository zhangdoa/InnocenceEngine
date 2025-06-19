#include <windows.h>
#include <windowsx.h>
#include <iostream>

#include "../../Common/STL14.h"
#include "../../Engine.h"

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
    
    // Get stack trace and function name
    HANDLE process = GetCurrentProcess();
    HANDLE thread = GetCurrentThread();
    
    SymInitialize(process, NULL, TRUE);
    SymSetOptions(SYMOPT_LOAD_LINES | SYMOPT_DEFERRED_LOADS);
    
    // Get symbol information
    SYMBOL_INFO* symbol = (SYMBOL_INFO*)calloc(sizeof(SYMBOL_INFO) + 256 * sizeof(char), 1);
    symbol->MaxNameLen = 255;
    symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
    
    DWORD64 displacement = 0;
    char functionName[256] = "Unknown";
    
    if (SymFromAddr(process, (DWORD64)exceptionAddress, &displacement, symbol))
    {
        strcpy_s(functionName, sizeof(functionName), symbol->Name);
    }
    
    // Get line information
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
    
    return EXCEPTION_EXECUTE_HANDLER;
}
#endif

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR pScmdline, int nCmdshow)
{
#if defined(INNO_DEBUG) || defined(INNO_RELWITHDEBINFO)
    // Set up global exception handler to catch access violations in debug builds
    SetUnhandledExceptionFilter(UnhandledExceptionHandler);
#endif
    
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