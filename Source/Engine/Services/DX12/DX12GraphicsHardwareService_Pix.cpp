#include "DX12GraphicsHardwareService.h"
#include "DX12Helper_Common.h"
#include "../../Engine.h"
#include "../../Common/LogService.h"

#ifdef _WIN32
#include <Windows.h>
#endif

using namespace Inno;
using namespace DX12Helper;

void DX12GraphicsHardwareService::TryLoadPIXEventRuntime()
{
#ifdef _WIN32
	// Load order: in-process → INNO_PIX_RUNTIME_DLL env override → PATH.
	HMODULE l_PIXModule = GetModuleHandleA("WinPixEventRuntime.dll");
	const char* l_LoadedFrom = nullptr;
	if (l_PIXModule)
	{
		l_LoadedFrom = "pre-injected";
	}
	else
	{
		char l_EnvOverride[MAX_PATH] = {};
		DWORD l_EnvLen = GetEnvironmentVariableA("INNO_PIX_RUNTIME_DLL", l_EnvOverride, MAX_PATH);
		if (l_EnvLen > 0 && l_EnvLen < MAX_PATH)
		{
			l_PIXModule = LoadLibraryA(l_EnvOverride);
			if (l_PIXModule) l_LoadedFrom = l_EnvOverride;
		}
		if (!l_PIXModule)
		{
			l_PIXModule = LoadLibraryA("WinPixEventRuntime.dll");
			if (l_PIXModule) l_LoadedFrom = "PATH";
		}
		if (!l_PIXModule)
		{
			Log(Verbose, "PIX: WinPixEventRuntime.dll not found (tried pre-injected, INNO_PIX_RUNTIME_DLL, PATH). PIX event markers disabled (timer queries unaffected).");
			return;
		}
	}

	auto l_PIXBegin = reinterpret_cast<PIXBeginEventOnCommandListFn>(GetProcAddress(l_PIXModule, "PIXBeginEventOnCommandList"));
	auto l_PIXEnd   = reinterpret_cast<PIXEndEventOnCommandListFn>(GetProcAddress(l_PIXModule, "PIXEndEventOnCommandList"));
	if (!l_PIXBegin || !l_PIXEnd)
	{
		Log(Warning, "PIX: WinPixEventRuntime.dll loaded but PIXBeginEventOnCommandList / PIXEndEventOnCommandList exports missing. PIX event markers disabled.");
		return;
	}

	m_PIXModule = l_PIXModule;
	m_PIXBeginEventOnCommandList = l_PIXBegin;
	m_PIXEndEventOnCommandList = l_PIXEnd;
	Log(Success, "PIX: WinPixEventRuntime loaded from ", l_LoadedFrom, ". GPU events will appear on PIX timeline.");
#endif
}

bool DX12GraphicsHardwareService::BeginGpuEvent(CommandListComponent* commandList, const char* name, uint32_t color)
{
	// No log on the disabled path: per-pass per-frame call site, flooding would
	// drown real diagnostics.
	if (m_PIXBeginEventOnCommandList == nullptr)
		return false;

	auto l_commandList = AsDX12CommandList(commandList);
	if (l_commandList == nullptr)
		return false;

	if (name == nullptr)
	{
		Log(Warning, "BeginGpuEvent: null name passed; skipping.");
		return false;
	}

	// PIX_COLOR(r,g,b) is the public macro; the export takes a packed UINT64.
	// 0 selects the PIX default colour.
	m_PIXBeginEventOnCommandList(l_commandList, static_cast<uint64_t>(color), name);
	return true;
}

bool DX12GraphicsHardwareService::EndGpuEvent(CommandListComponent* commandList)
{
	if (m_PIXEndEventOnCommandList == nullptr)
		return false;

	auto l_commandList = AsDX12CommandList(commandList);
	if (l_commandList == nullptr)
		return false;

	m_PIXEndEventOnCommandList(l_commandList);
	return true;
}
