#include "DX12GraphicsHardwareService.h"
#include "DX12Helper_Common.h"
#include "../../Engine.h"
#include "../../Common/IOService.h"
#include "../../Common/LogService.h"

#include "../../../External/GitSubmodules/renderdoc/renderdoc/api/app/renderdoc_app.h"

#include <filesystem>

#ifdef _WIN32
#include <Windows.h>
#endif

using namespace Inno;
using namespace DX12Helper;

bool DX12GraphicsHardwareService::TryLoadRenderDocAPI()
{
#ifdef _WIN32
	auto l_initConfig = g_Engine->getInitConfig();
	if (l_initConfig.captureFrame < 0)
		return false;

	// Load order: in-process → INNO_RENDERDOC_DLL env override → PATH → default
	// install path.
	static constexpr const char* k_DefaultInstallPath = "C:/Program Files/RenderDoc/renderdoc.dll";
	HMODULE l_RenderDocModule = GetModuleHandleA("renderdoc.dll");
	const char* l_LoadedFrom = nullptr;
	if (l_RenderDocModule)
	{
		l_LoadedFrom = "pre-injected";
	}
	else
	{
		char l_EnvOverride[MAX_PATH] = {};
		DWORD l_EnvLen = GetEnvironmentVariableA("INNO_RENDERDOC_DLL", l_EnvOverride, MAX_PATH);
		if (l_EnvLen > 0 && l_EnvLen < MAX_PATH)
		{
			l_RenderDocModule = LoadLibraryA(l_EnvOverride);
			if (l_RenderDocModule) l_LoadedFrom = l_EnvOverride;
		}
		if (!l_RenderDocModule)
		{
			l_RenderDocModule = LoadLibraryA("renderdoc.dll");
			if (l_RenderDocModule) l_LoadedFrom = "PATH";
		}
		if (!l_RenderDocModule)
		{
			l_RenderDocModule = LoadLibraryA(k_DefaultInstallPath);
			if (l_RenderDocModule) l_LoadedFrom = k_DefaultInstallPath;
		}
		if (!l_RenderDocModule)
		{
			Log(Warning, "RenderDoc: renderdoc.dll not found (tried pre-injected, INNO_RENDERDOC_DLL env var, PATH, '",
				k_DefaultInstallPath, "'). In-process capture disabled.");
			return false;
		}
	}
	Log(Success, "RenderDoc: loaded renderdoc.dll from ", l_LoadedFrom, ".");

	auto l_GetAPI = (pRENDERDOC_GetAPI)GetProcAddress(l_RenderDocModule, "RENDERDOC_GetAPI");
	if (l_GetAPI == nullptr)
	{
		Log(Warning, "RenderDoc: RENDERDOC_GetAPI symbol not found.");
		return false;
	}

	RENDERDOC_API_1_6_0* l_API = nullptr;
	int l_Result = l_GetAPI(eRENDERDOC_API_Version_1_6_0, (void**)&l_API);
	if (l_Result != 1 || l_API == nullptr)
		return false;

	m_RenderDocAPI = l_API;

	// Template is a "<file-prefix>"; RenderDoc appends frame index and `.rdc`.
	// Capture dir may not exist on first run, hence create_directories.
	auto l_workingDir  = g_Engine->Get<IOService>()->GetWorkingDirectory();
	auto l_captureDir  = l_workingDir + "../Build/captures";
	std::filesystem::create_directories(l_captureDir);
	std::string l_captureTemplate = l_captureDir + "/frame";
	l_API->SetCaptureFilePathTemplate(l_captureTemplate.c_str());
	l_API->SetCaptureOptionU32(eRENDERDOC_Option_RefAllResources, 1);
	l_API->SetCaptureOptionU32(eRENDERDOC_Option_CaptureAllCmdLists, 1);

	Log(Success, "RenderDoc API loaded.");
	return true;
#else
	return false;
#endif
}

bool DX12GraphicsHardwareService::BeginCapture()
{
	if (m_RenderDocAPI != nullptr)
	{
		auto l_API = static_cast<RENDERDOC_API_1_6_0*>(m_RenderDocAPI);
		// StartFrameCapture works without a swapchain present, so it succeeds in offscreen mode.
		l_API->StartFrameCapture(nullptr, nullptr);
		Log(Success, "RenderDoc: frame capture started.");
		return true;
	}

	if (m_DX12Context.m_graphicsAnalysis != nullptr)
	{
		m_DX12Context.m_graphicsAnalysis->BeginCapture();
		return true;
	}

	return false;
}

bool DX12GraphicsHardwareService::EndCapture()
{
	if (m_RenderDocAPI != nullptr)
	{
		auto l_API = static_cast<RENDERDOC_API_1_6_0*>(m_RenderDocAPI);
		uint32_t l_Result = l_API->EndFrameCapture(nullptr, nullptr);
		if (l_Result != 1)
		{
			Log(Warning, "RenderDoc: EndFrameCapture returned failure (no matching StartFrameCapture?).");
			return false;
		}
		uint32_t l_NumCaptures = l_API->GetNumCaptures();
		if (l_NumCaptures > 0)
		{
			char l_Path[512] = {};
			uint32_t l_PathLen = sizeof(l_Path);
			uint64_t l_Timestamp = 0;
			l_API->GetCapture(l_NumCaptures - 1, l_Path, &l_PathLen, &l_Timestamp);
			Log(Success, "RenderDoc: frame capture saved to ", l_Path);
		}
		return true;
	}

	if (m_DX12Context.m_graphicsAnalysis != nullptr)
	{
		m_DX12Context.m_graphicsAnalysis->EndCapture();
		return true;
	}

	return false;
}

bool DX12GraphicsHardwareService::HasGPUError() const
{
	if (m_DX12Context.m_GPUErrorDetected.load())
		return true;

	if (m_DX12Context.m_device)
	{
		try
		{
			auto hr = m_DX12Context.m_device->GetDeviceRemovedReason();
			if (hr != S_OK)
			{
				m_DX12Context.m_GPUErrorDetected.store(true);
				return true;
			}
		}
		catch (...)
		{
			m_DX12Context.m_GPUErrorDetected.store(true);
			return true;
		}
	}

	return false;
}

void DX12GraphicsHardwareService::DumpGPUDiagnostics()
{
	if (!m_DX12Context.m_device)
		return;

	try
	{
		auto hr = m_DX12Context.m_device->GetDeviceRemovedReason();
		if (hr != S_OK)
		{
			Log(Warning, "GPU device removed reason: HRESULT=", static_cast<int32_t>(hr));
			DumpDRED(m_DX12Context.m_device.Get());
		}
	}
	catch (...)
	{
		Log(Warning, "Exception while querying GPU diagnostics, device may be in unrecoverable state.");
	}
}
