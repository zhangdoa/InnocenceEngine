#include "DX12Context.h"
#include "../../Engine.h"
#include "../../Common/LogService.h"

#include <atomic>
#include <cstdlib>
#include <cstring>

#ifdef _WIN32
#include <Windows.h>
#include <DbgHelp.h>
#pragma comment(lib, "dbghelp.lib")
#endif

using namespace Inno;

// TU-local error sentinel: written by the callback, read indirectly via the
// per-context flag (m_GPUErrorDetected) — the callback also propagates onto
// the context pointer it receives, so cross-TU code never reads this directly.
static std::atomic<bool> g_GPUErrorDetected{false};

// GPU-based validation on Release shaders runs in "Shader Patch Mode NONE":
// DXC has stripped the metadata GBV relies on to correlate resource state
// and root-binding info with shader accesses. The result is a family of
// false-positive GBV errors that only surface under -gpu_validation against
// Release shaders. Without this classifier the callback would log them at
// [Error] level, which LogService::SetFatalOnError (active whenever
// InitConfig::totalFrames > 0) upgrades to a fatal exit-1, blocking every
// -gpu_validation integration run. Real GBV errors — corruption-class, or
// any diagnostic from non-Release builds — still fall through to [Error].
//
// Observed categories (extend as new ones surface):
//  1. "Incompatible texture barrier layout" with "Layout: UNKNOWN (N)"
//     where N > D3D12_BARRIER_LAYOUT_VIDEO_QUEUE_COMMON (30, the last real
//     enum value) — GBV can't recover the real layout from a patched shader.
//  2. "Uninitialized root argument accessed" — GBV can't resolve root
//     parameter bindings without the debug-shader metadata.
//
// See CLAUDE.md "Known imprecision", TASK-37, TASK-120.
static bool IsReleaseShaderGBVFalsePositive(LPCSTR pDescription)
{
    if (pDescription == nullptr)
        return false;
    if (std::strstr(pDescription, "GPU-BASED VALIDATION") == nullptr)
        return false;

    if (std::strstr(pDescription, "Incompatible texture barrier layout") != nullptr)
    {
        const char* p = std::strstr(pDescription, "Layout: UNKNOWN (");
        if (p == nullptr)
            return false;
        p += sizeof("Layout: UNKNOWN (") - 1;
        char* end = nullptr;
        const long layoutValue = std::strtol(p, &end, 10);
        if (end == p)
            return false;
        return layoutValue > 30;
    }

    if (std::strstr(pDescription, "Uninitialized root argument accessed") != nullptr)
        return true;

    return false;
}

#ifdef _WIN32
static std::string CaptureCallstack(UINT framesToSkip = 1, UINT maxFrames = 10)
{
    static bool symbolsInitialized = false;
    if (!symbolsInitialized)
    {
        SymInitialize(GetCurrentProcess(), NULL, TRUE);
        symbolsInitialized = true;
    }

    std::string callstack = "\nCallstack:\n";

    void* stack[32];
    WORD numberOfFrames = CaptureStackBackTrace(framesToSkip, maxFrames, stack, NULL);

    SYMBOL_INFO* symbol = (SYMBOL_INFO*)malloc(sizeof(SYMBOL_INFO) + 256 * sizeof(char));
    symbol->MaxNameLen = 255;
    symbol->SizeOfStruct = sizeof(SYMBOL_INFO);

    for (int i = 0; i < numberOfFrames; i++)
    {
        DWORD64 address = (DWORD64)(stack[i]);

        if (SymFromAddr(GetCurrentProcess(), address, 0, symbol))
        {
            callstack += "  " + std::to_string(i) + ": " + symbol->Name + " (0x" +
                        std::to_string(address) + ")\n";
        }
        else
        {
            callstack += "  " + std::to_string(i) + ": <unknown> (0x" +
                        std::to_string(address) + ")\n";
        }
    }

    free(symbol);
    return callstack;
}
#endif

// Defined here, forward-declared in DX12GraphicsHardwareService_Hardware_Devices.cpp.
// Linkage is external (no `static`) so the device-creation TU can pass its address
// to ID3D12InfoQueue1::RegisterMessageCallback.
void CALLBACK D3D12DebugMessageCallback(
    D3D12_MESSAGE_CATEGORY Category,
    D3D12_MESSAGE_SEVERITY Severity,
    D3D12_MESSAGE_ID ID,
    LPCSTR pDescription,
    void* pContext)
{
    switch (Severity)
    {
    case D3D12_MESSAGE_SEVERITY_CORRUPTION:
    case D3D12_MESSAGE_SEVERITY_ERROR:
        {
            if (IsReleaseShaderGBVFalsePositive(pDescription))
            {
                Log(Warning, "D3D12 GBV Release-shader false positive (non-fatal): ", pDescription);
                break;
            }
            g_GPUErrorDetected.store(true);
            if (pContext)
                static_cast<DX12Context*>(pContext)->m_GPUErrorDetected.store(true);
#ifdef _WIN32
            std::string callstackInfo = CaptureCallstack(2, 15);
            Log(Error, "D3D12 ERROR: ", pDescription, callstackInfo.c_str());
#else
            Log(Error, "D3D12 ERROR: ", pDescription);
#endif
        }
        break;
    case D3D12_MESSAGE_SEVERITY_WARNING:
        Log(Warning, "D3D12 WARNING: ", pDescription);
        break;
    case D3D12_MESSAGE_SEVERITY_INFO:
    case D3D12_MESSAGE_SEVERITY_MESSAGE:
        Log(Verbose, "D3D12 INFO: ", pDescription);
        break;
    }
}
