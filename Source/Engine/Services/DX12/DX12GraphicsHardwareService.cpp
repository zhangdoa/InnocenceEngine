#include "DX12GraphicsHardwareService.h"
#include "DX12TextureResourceService.h"
#include "DX12GPUBufferResourceService.h"
#include "DX12RenderPassResourceService.h"
#include "../TextureResourceService.h"
#include "../GPUBufferResourceService.h"
#include "../RenderPassResourceService.h"
#include "../FrameManagementService.h"
#include "../../Engine.h"
#include "../../Services/RenderingConfigurationService.h"
#include "../../Common/LogService.h"
#include "../../Common/LogServiceSpecialization.h"
#include "DX12Helper_Common.h"
#include "DX12Helper_Pipeline.h"
#include "DX12Helper_Texture.h"

#include "../../../External/GitSubmodules/renderdoc/renderdoc/api/app/renderdoc_app.h"

#ifdef _WIN32
#include <Windows.h>
#include <DbgHelp.h>
#pragma comment(lib, "dbghelp.lib")
#endif

using namespace Inno;
using namespace DX12Helper;

static std::atomic<bool> g_GPUErrorDetected{false};

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

static void CALLBACK D3D12DebugMessageCallback(
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

bool DX12GraphicsHardwareService::WaitOnFenceWithDiagnostics(const char* fenceName, ID3D12Fence* fence, HANDLE fenceEvent, uint64_t semaphoreValue)
{
	if (fence->GetCompletedValue() >= semaphoreValue)
		return true;

	fence->SetEventOnCompletion(semaphoreValue, fenceEvent);
	DWORD l_waitResult = WaitForSingleObject(fenceEvent, 30000);
	if (l_waitResult == WAIT_OBJECT_0)
		return true;

	auto l_drr = m_DX12Context.m_device ? m_DX12Context.m_device->GetDeviceRemovedReason() : S_OK;
	if (l_waitResult == WAIT_TIMEOUT)
	{
		Log(Error, fenceName, " wait timeout (30s). Semaphore=", semaphoreValue,
			" Completed=", fence->GetCompletedValue(),
			" DeviceRemovedReason=", static_cast<int32_t>(l_drr));
	}
	else
	{
		Log(Error, fenceName, " wait failed. WaitResult=", static_cast<uint32_t>(l_waitResult),
			" LastError=", static_cast<uint32_t>(GetLastError()),
			" DeviceRemovedReason=", static_cast<int32_t>(l_drr));
	}

	if (l_drr != S_OK)
	{
		m_DX12Context.m_GPUErrorDetected.store(true);
		DumpDRED(m_DX12Context.m_device.Get());
	}
	return false;
}

// --- Sync primitives ---

bool DX12GraphicsHardwareService::SignalOnGPU(ISemaphore* semaphore, GPUEngineType queueType)
{
	auto l_globalSemaphore = reinterpret_cast<DX12Semaphore*>(g_Engine->Get<FrameManagementService>()->GetGlobalSemaphore());
	if (!l_globalSemaphore)
	{
		Log(Error, "Global semaphore is null in SignalOnGPU");
		return false;
	}

	auto l_semaphore = reinterpret_cast<DX12Semaphore*>(semaphore);
	auto l_isOnGlobalSemaphore = l_semaphore == l_globalSemaphore;

	if (queueType == GPUEngineType::Graphics)
	{
		if (!m_DX12Context.m_directCommandQueue || !m_DX12Context.m_directCommandQueueFence)
		{
			Log(Error, "DirectCommandQueue or DirectCommandQueueFence is null in SignalOnGPU");
			return false;
		}

		uint64_t l_directCommandFinishedSemaphore = l_globalSemaphore->m_DirectCommandQueueSemaphore.fetch_add(1) + 1;

		if (l_semaphore && !l_isOnGlobalSemaphore)
			l_semaphore->m_DirectCommandQueueSemaphore = l_directCommandFinishedSemaphore;

		m_DX12Context.m_directCommandQueue->Signal(m_DX12Context.m_directCommandQueueFence.Get(), l_directCommandFinishedSemaphore);
	}
	else if (queueType == GPUEngineType::Compute)
	{
		if (!m_DX12Context.m_computeCommandQueue || !m_DX12Context.m_computeCommandQueueFence)
		{
			Log(Error, "ComputeCommandQueue or ComputeCommandQueueFence is null in SignalOnGPU");
			return false;
		}

		UINT64 l_computeCommandFinishedSemaphore = l_globalSemaphore->m_ComputeCommandQueueSemaphore.fetch_add(1) + 1;

		if (l_semaphore && !l_isOnGlobalSemaphore)
			l_semaphore->m_ComputeCommandQueueSemaphore = l_computeCommandFinishedSemaphore;

		m_DX12Context.m_computeCommandQueue->Signal(m_DX12Context.m_computeCommandQueueFence.Get(), l_computeCommandFinishedSemaphore);
	}
	else if (queueType == GPUEngineType::Copy)
	{
		if (!m_DX12Context.m_copyCommandQueue || !m_DX12Context.m_copyCommandQueueFence)
		{
			Log(Error, "CopyCommandQueue or CopyCommandQueueFence is null in SignalOnGPU");
			return false;
		}

		UINT64 l_copyCommandFinishedSemaphore = l_globalSemaphore->m_CopyCommandQueueSemaphore.fetch_add(1) + 1;

		if (l_semaphore && !l_isOnGlobalSemaphore)
			l_semaphore->m_CopyCommandQueueSemaphore = l_copyCommandFinishedSemaphore;

		m_DX12Context.m_copyCommandQueue->Signal(m_DX12Context.m_copyCommandQueueFence.Get(), l_copyCommandFinishedSemaphore);
	}

	return true;
}

bool DX12GraphicsHardwareService::WaitOnGPU(ISemaphore* semaphore, GPUEngineType queueType, GPUEngineType semaphoreType)
{
	auto l_globalSemaphore = reinterpret_cast<DX12Semaphore*>(g_Engine->Get<FrameManagementService>()->GetGlobalSemaphore());
	ID3D12CommandQueue* commandQueue = nullptr;
	ID3D12Fence* fence = nullptr;
	uint64_t semaphoreValue = 0;
	auto l_semaphore = semaphore ? reinterpret_cast<DX12Semaphore*>(semaphore) : l_globalSemaphore;

	if (queueType == GPUEngineType::Graphics)
		commandQueue = m_DX12Context.GetGlobalCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT).Get();
	else if (queueType == GPUEngineType::Compute)
		commandQueue = m_DX12Context.GetGlobalCommandQueue(D3D12_COMMAND_LIST_TYPE_COMPUTE).Get();
	else if (queueType == GPUEngineType::Copy)
		commandQueue = m_DX12Context.GetGlobalCommandQueue(D3D12_COMMAND_LIST_TYPE_COPY).Get();

	if (semaphoreType == GPUEngineType::Graphics)
	{
		fence = m_DX12Context.m_directCommandQueueFence.Get();
		semaphoreValue = l_semaphore->m_DirectCommandQueueSemaphore;
	}
	else if (semaphoreType == GPUEngineType::Compute)
	{
		fence = m_DX12Context.m_computeCommandQueueFence.Get();
		semaphoreValue = l_semaphore->m_ComputeCommandQueueSemaphore;
	}
	else if (semaphoreType == GPUEngineType::Copy)
	{
		fence = m_DX12Context.m_copyCommandQueueFence.Get();
		semaphoreValue = l_semaphore->m_CopyCommandQueueSemaphore;
	}

	if (semaphoreValue == 0)
		return true;

	if (commandQueue && fence)
		commandQueue->Wait(fence, semaphoreValue);

	return true;
}

bool DX12GraphicsHardwareService::Execute(CommandListComponent* commandList, GPUEngineType queueType)
{
	auto l_commandList = DX12Helper::AsDX12CommandList(commandList);
	if (l_commandList == nullptr)
		return false;

	ID3D12CommandList* l_commandListToExecute[] = { l_commandList };

	if (queueType == GPUEngineType::Graphics)
		m_DX12Context.m_directCommandQueue->ExecuteCommandLists(1, l_commandListToExecute);
	else if (queueType == GPUEngineType::Compute)
		m_DX12Context.m_computeCommandQueue->ExecuteCommandLists(1, l_commandListToExecute);
	else if (queueType == GPUEngineType::Copy)
		m_DX12Context.m_copyCommandQueue->ExecuteCommandLists(1, l_commandListToExecute);

	return true;
}

uint64_t DX12GraphicsHardwareService::GetSemaphoreValue(GPUEngineType queueType)
{
	auto l_semaphore = reinterpret_cast<DX12Semaphore*>(g_Engine->Get<FrameManagementService>()->GetGlobalSemaphore());

	if (queueType == GPUEngineType::Graphics)
		return l_semaphore->m_DirectCommandQueueSemaphore;
	else if (queueType == GPUEngineType::Compute)
		return l_semaphore->m_ComputeCommandQueueSemaphore;
	else if (queueType == GPUEngineType::Copy)
		return l_semaphore->m_CopyCommandQueueSemaphore;

	return 0;
}

bool DX12GraphicsHardwareService::WaitOnCPU(uint64_t semaphoreValue, GPUEngineType queueType)
{
	if (semaphoreValue == 0)
		return true;

	auto l_semaphore = reinterpret_cast<DX12Semaphore*>(g_Engine->Get<FrameManagementService>()->GetGlobalSemaphore());
	HANDLE* fenceEvent = nullptr;

	if (queueType == GPUEngineType::Graphics)
		fenceEvent = &l_semaphore->m_DirectCommandQueueFenceEvent;
	else if (queueType == GPUEngineType::Compute)
		fenceEvent = &l_semaphore->m_ComputeCommandQueueFenceEvent;
	else if (queueType == GPUEngineType::Copy)
		fenceEvent = &l_semaphore->m_CopyCommandQueueFenceEvent;

	if (!fenceEvent || *fenceEvent == nullptr)
	{
		Log(Error, "Invalid fence event handle in WaitOnCPU for queue type: ", (uint32_t)queueType);
		return false;
	}

	if (queueType == GPUEngineType::Graphics)
	{
		if (!m_DX12Context.m_directCommandQueueFence)
		{
			Log(Error, "DirectCommandQueueFence is null in WaitOnCPU");
			return false;
		}

		WaitOnFenceWithDiagnostics("DirectCommandQueueFence", m_DX12Context.m_directCommandQueueFence.Get(), *fenceEvent, semaphoreValue);
	}
	else if (queueType == GPUEngineType::Compute)
	{
		if (!m_DX12Context.m_computeCommandQueueFence)
		{
			Log(Error, "ComputeCommandQueueFence is null in WaitOnCPU");
			return false;
		}

		WaitOnFenceWithDiagnostics("ComputeCommandQueueFence", m_DX12Context.m_computeCommandQueueFence.Get(), *fenceEvent, semaphoreValue);
	}
	else if (queueType == GPUEngineType::Copy)
	{
		if (!m_DX12Context.m_copyCommandQueueFence)
		{
			Log(Error, "CopyCommandQueueFence is null in WaitOnCPU");
			return false;
		}

		WaitOnFenceWithDiagnostics("CopyCommandQueueFence", m_DX12Context.m_copyCommandQueueFence.Get(), *fenceEvent, semaphoreValue);
	}

	return true;
}

// --- Debug/capture ---

bool DX12GraphicsHardwareService::TryLoadRenderDocAPI()
{
#ifdef _WIN32
	auto l_initConfig = g_Engine->getInitConfig();
	if (l_initConfig.captureFrame < 0)
		return false;

	HMODULE l_RenderDocModule = GetModuleHandleA("renderdoc.dll");
	if (l_RenderDocModule == nullptr)
	{
		l_RenderDocModule = LoadLibraryA("C:/Program Files/RenderDoc/renderdoc.dll");
		if (l_RenderDocModule == nullptr)
		{
			Log(Warning, "RenderDoc: failed to load renderdoc.dll from 'C:/Program Files/RenderDoc/'.");
			return false;
		}
		Log(Success, "RenderDoc: loaded renderdoc.dll from 'C:/Program Files/RenderDoc/'.");
	}
	else
	{
		Log(Success, "RenderDoc: detected pre-injected renderdoc.dll.");
	}

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

	std::string l_captureDir = "C:/GitRepo/InnocenceEngine/Build/captures/frame";
	l_API->SetCaptureFilePathTemplate(l_captureDir.c_str());
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

// --- Public accessors ---

ComPtr<ID3D12Device8> DX12GraphicsHardwareService::GetDevice()
{
	return m_DX12Context.m_device.Get();
}

ComPtr<ID3D12CommandAllocator> DX12GraphicsHardwareService::GetGlobalCommandAllocator(D3D12_COMMAND_LIST_TYPE commandListType)
{
	auto l_currentFrame = g_Engine->Get<FrameManagementService>()->GetCurrentFrame();
	return m_DX12Context.GetGlobalCommandAllocator(commandListType, l_currentFrame);
}

ComPtr<ID3D12CommandQueue> DX12GraphicsHardwareService::GetGlobalCommandQueue(D3D12_COMMAND_LIST_TYPE commandListType)
{
	return m_DX12Context.GetGlobalCommandQueue(commandListType);
}

DX12DescriptorHeapAccessor& DX12GraphicsHardwareService::GetDescriptorHeapAccessor(GPUResourceType type, Accessibility bindingAccessibility,
	Accessibility resourceAccessibility, TextureUsage textureUsage, bool isShaderVisible)
{
	return m_DX12Context.GetDescriptorHeapAccessor(type, bindingAccessibility, resourceAccessibility, textureUsage, isShaderVisible);
}

// --- Hardware creation ---

bool DX12GraphicsHardwareService::CreateDebugCallback()
{
    try
    {
        ID3D12Debug* l_debugInterface;

        auto l_HResult = D3D12GetDebugInterface(IID_PPV_ARGS(&l_debugInterface));
        if (FAILED(l_HResult))
        {
            Log(Warning, "DirectX 12 debug interface not available.");
            return false;
        }

        l_HResult = l_debugInterface->QueryInterface(IID_PPV_ARGS(&m_DX12Context.m_debugInterface));
        if (FAILED(l_HResult))
        {
            Log(Warning, "Can't query DirectX 12 debug interface.");
            return false;
        }

        m_DX12Context.m_debugInterface->EnableDebugLayer();

        if (g_Engine->getInitConfig().enableGPUValidation)
        {
            m_DX12Context.m_debugInterface->SetEnableGPUBasedValidation(true);
            m_DX12Context.m_debugInterface->SetEnableSynchronizedCommandQueueValidation(true);
            Log(Success, "Debug layer + GPU-based validation + synchronized command queue validation enabled.");
        }
        else
        {
            Log(Success, "Debug layer enabled (GPU-based validation off).");
        }

        l_HResult = DXGIGetDebugInterface1(0, IID_PPV_ARGS(&m_DX12Context.m_graphicsAnalysis));
        if (SUCCEEDED(l_HResult))
            Log(Success, "PIX attached.");
    }
    catch (...)
    {
        Log(Warning, "Debug layer initialization failed (COM exception), continuing without debug layer.");
        return false;
    }

    return true;
}

bool DX12GraphicsHardwareService::CreatePhysicalDevices()
{
    UINT l_DXGIFlag = 0;
#if defined(INNO_DEBUG) || defined(INNO_RELWITHDEBINFO)
    l_DXGIFlag |= DXGI_CREATE_FACTORY_DEBUG;
#endif

    auto l_HResult = CreateDXGIFactory2(l_DXGIFlag, IID_PPV_ARGS(&m_DX12Context.m_factory));
    if (FAILED(l_HResult))
    {
        Log(Error, "Can't create DXGI factory!");
        return false;
    }

    Log(Success, "DXGI factory has been created.");

    IDXGIAdapter1* l_adapter;
    UINT adapterIndex = 0;
    l_HResult = m_DX12Context.m_factory->EnumAdapterByGpuPreference(adapterIndex, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&l_adapter));
    if (FAILED(l_HResult))
    {
        Log(Warning, "Can't find a high-performance GPU.");
        l_HResult = m_DX12Context.m_factory->EnumAdapterByGpuPreference(adapterIndex, DXGI_GPU_PREFERENCE_UNSPECIFIED, IID_PPV_ARGS(&l_adapter));
        if (FAILED(l_HResult))
        {
            Log(Error, "Can't find any capable GPU!");
            return false;
        }
    }

    if (FAILED(D3D12CreateDevice(l_adapter, D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr)))
    {
        Log(Error, "Adapter doesn't support DirectX 12!");
        return false;
    }

    if (l_adapter == nullptr)
    {
        Log(Error, "Can't create a suitable video card adapter!");
        return false;
    }

    m_DX12Context.m_adapter = reinterpret_cast<IDXGIAdapter4*>(l_adapter);

    DXGI_ADAPTER_DESC3 l_adapter_desc;
    m_DX12Context.m_adapter->GetDesc3(&l_adapter_desc);
    std::wstring l_descL = std::wstring(l_adapter_desc.Description);

    int length = WideCharToMultiByte(CP_UTF8, 0, l_descL.c_str(), -1, nullptr, 0, nullptr, nullptr);
    std::string l_desc(length, 0);
    WideCharToMultiByte(CP_UTF8, 0, l_descL.c_str(), -1, l_desc.data(), length, nullptr, nullptr);

    Log(Success, "Adapter for: ", l_desc.c_str(), " has been created.");

    IDXGIOutput* l_adapterOutput;
    l_HResult = m_DX12Context.m_adapter->EnumOutputs(0, &l_adapterOutput);
    if (FAILED(l_HResult))
    {
        Log(Warning, "the primary output of the adapter is not connected.");
    }
    else
    {
        l_HResult = l_adapterOutput->QueryInterface(IID_PPV_ARGS(&m_DX12Context.m_adapterOutput));
    }

    uint64_t l_stringLength;

    l_HResult = m_DX12Context.m_adapter->GetDesc(&m_DX12Context.m_adapterDesc);
    if (FAILED(l_HResult))
    {
        Log(Error, "can't get the video card adapter description!");
        return false;
    }

    m_DX12Context.m_videoCardMemory = (int32_t)(m_DX12Context.m_adapterDesc.DedicatedVideoMemory / 1024 / 1024);

    if (wcstombs_s(&l_stringLength, m_DX12Context.m_videoCardDescription, 128, m_DX12Context.m_adapterDesc.Description, 128) != 0)
    {
        Log(Error, "can't convert the name of the video card to a character array!");
        return false;
    }

    // Enable DRED (Device Removed Extended Data) BEFORE device creation
    try
    {
        ComPtr<ID3D12DeviceRemovedExtendedDataSettings1> l_pDredSettings;
        if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&l_pDredSettings))))
        {
            l_pDredSettings->SetAutoBreadcrumbsEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
            l_pDredSettings->SetPageFaultEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
            l_pDredSettings->SetBreadcrumbContextEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
            Log(Success, "DRED (Device Removed Extended Data) enabled.");
        }
        else
        {
            Log(Warning, "DRED not available on this system.");
        }
    }
    catch (...)
    {
        Log(Warning, "DRED initialization failed (COM exception), continuing without DRED.");
    }

    auto featureLevel = D3D_FEATURE_LEVEL_12_2;

    l_HResult = D3D12CreateDevice(m_DX12Context.m_adapter.Get(), featureLevel, IID_PPV_ARGS(&m_DX12Context.m_device));
    if (FAILED(l_HResult))
    {
        Log(Error, "Can't create a DirectX 12.2 device. The default video card does not support DirectX 12.2!");
        return false;
    }

    D3D12_FEATURE_DATA_D3D12_OPTIONS options = {};
    if (SUCCEEDED(m_DX12Context.m_device->CheckFeatureSupport(
        D3D12_FEATURE_D3D12_OPTIONS,
        &options,
        sizeof(options))))
    {
        if (!options.TypedUAVLoadAdditionalFormats)
            Log(Warning, "TypedUAVLoadAdditionalFormats is not supported, can't generate mipmap for sRGB textures.");
    }

    Log(Success, "D3D device has been created.");

    try
    {
        ComPtr<ID3D12InfoQueue> l_pInfoQueue;
        l_HResult = m_DX12Context.m_device->QueryInterface(IID_PPV_ARGS(&l_pInfoQueue));

        if (SUCCEEDED(l_HResult) && l_pInfoQueue)
        {
            l_pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
            l_pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
            Log(Success, "Debug report severity has been set.");

            ComPtr<ID3D12InfoQueue1> l_pInfoQueue1;
            if (SUCCEEDED(l_pInfoQueue.As(&l_pInfoQueue1)) && l_pInfoQueue1)
            {
                l_HResult = l_pInfoQueue1->RegisterMessageCallback(
                    D3D12DebugMessageCallback,
                    D3D12_MESSAGE_CALLBACK_FLAG_NONE,
                    &m_DX12Context,
                    &m_DX12Context.m_debugCallbackCookie);

                if (SUCCEEDED(l_HResult))
                    Log(Success, "D3D12 debug message callback registered.");
                else
                    Log(Warning, "Failed to register D3D12 debug message callback.");
            }
        }
    }
    catch (...)
    {
        Log(Warning, "Debug info queue setup failed (COM exception), continuing without debug callbacks.");
    }

    return true;
}

bool DX12GraphicsHardwareService::CreateGlobalCommandQueues()
{
    D3D12_COMMAND_QUEUE_DESC l_graphicCommandQueueDesc = {};
    l_graphicCommandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    l_graphicCommandQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
    l_graphicCommandQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    l_graphicCommandQueueDesc.NodeMask = 0;

    D3D12_COMMAND_QUEUE_DESC l_computeCommandQueueDesc = {};
    l_computeCommandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_COMPUTE;
    l_computeCommandQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
    l_computeCommandQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    l_computeCommandQueueDesc.NodeMask = 0;

    D3D12_COMMAND_QUEUE_DESC l_copyCommandQueueDesc = {};
    l_copyCommandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_COPY;
    l_copyCommandQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
    l_copyCommandQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    l_copyCommandQueueDesc.NodeMask = 0;

    m_DX12Context.m_directCommandQueue = m_DX12Context.CreateCommandQueue(&l_graphicCommandQueueDesc, L"DirectCommandQueue");
    m_DX12Context.m_computeCommandQueue = m_DX12Context.CreateCommandQueue(&l_computeCommandQueueDesc, L"ComputeCommandQueue");
    m_DX12Context.m_copyCommandQueue = m_DX12Context.CreateCommandQueue(&l_copyCommandQueueDesc, L"CopyCommandQueue");

    Log(Success, "Global CommandQueues have been created.");

    return true;
}

bool DX12GraphicsHardwareService::CreateGlobalCommandAllocators()
{
    auto l_swapChainImageCount = g_Engine->Get<FrameManagementService>()->GetSwapChainImageCount();
    m_DX12Context.m_directCommandAllocators.resize(l_swapChainImageCount);
    m_DX12Context.m_computeCommandAllocators.resize(l_swapChainImageCount);
    m_DX12Context.m_copyCommandAllocators.resize(l_swapChainImageCount);
    for (size_t i = 0; i < l_swapChainImageCount; i++)
    {
        m_DX12Context.m_directCommandAllocators[i] = m_DX12Context.CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, (L"DirectCommandAllocator_" + std::to_wstring(i)).c_str());
        m_DX12Context.m_computeCommandAllocators[i] = m_DX12Context.CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COMPUTE, (L"ComputeCommandAllocator_" + std::to_wstring(i)).c_str());
        m_DX12Context.m_copyCommandAllocators[i] = m_DX12Context.CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COPY, (L"CopyCommandAllocator_" + std::to_wstring(i)).c_str());
    }

    Log(Success, "Global CommandAllocators have been created.");

    return true;
}

bool DX12GraphicsHardwareService::CreateSyncPrimitives()
{
    if (FAILED(m_DX12Context.m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_DX12Context.m_directCommandQueueFence))))
    {
        Log(Error, "Can't create Fence for direct CommandQueue!");
        return false;
    }
    if (FAILED(m_DX12Context.m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_DX12Context.m_computeCommandQueueFence))))
    {
        Log(Error, "Can't create Fence for compute CommandQueue!");
        return false;
    }
    if (FAILED(m_DX12Context.m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_DX12Context.m_copyCommandQueueFence))))
    {
        Log(Error, "Can't create Fence for copy CommandQueue!");
        return false;
    }
#if defined(INNO_DEBUG) || defined(INNO_RELWITHDEBINFO)
    m_DX12Context.m_directCommandQueueFence->SetName(L"DirectCommandQueueFence");
    m_DX12Context.m_computeCommandQueueFence->SetName(L"ComputeCommandQueueFence");
    m_DX12Context.m_copyCommandQueueFence->SetName(L"CopyCommandQueueFence");
#endif

    Log(Verbose, "Fences for global CommandQueues have been created.");

    auto l_GlobalSemaphore = static_cast<DX12Semaphore*>(g_Engine->Get<RenderPassResourceService>()->AddSemaphore());
    l_GlobalSemaphore->m_DirectCommandQueueFenceEvent = CreateEventEx(nullptr, FALSE, FALSE, EVENT_ALL_ACCESS);
    l_GlobalSemaphore->m_ComputeCommandQueueFenceEvent = CreateEventEx(nullptr, FALSE, FALSE, EVENT_ALL_ACCESS);
    l_GlobalSemaphore->m_CopyCommandQueueFenceEvent = CreateEventEx(nullptr, FALSE, FALSE, EVENT_ALL_ACCESS);
    m_FrameManagementService->SetGlobalSemaphore(l_GlobalSemaphore);

    Log(Verbose, "Fence events for global CommandQueues have been created.");

    return true;
}

bool DX12GraphicsHardwareService::CreateGlobalDescriptorHeaps()
{
    auto l_renderingCapacity = g_Engine->Get<RenderingConfigurationService>()->GetRenderingCapability();

    uint32_t l_maxGPUBuffers = l_renderingCapacity.maxBuffers;
    uint32_t l_maxMaterialTextures = l_renderingCapacity.maxTextures;
    uint32_t l_maxRenderTargetTextures = 2048;

    {
        auto l_MaxDescriptorCount = l_maxGPUBuffers * 3 + l_maxMaterialTextures * 2 + l_maxRenderTargetTextures * 2;
        auto l_Desc = GetDescriptorHeapDesc(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, l_MaxDescriptorCount, true);
        m_DX12Context.m_CSUDescHeap = m_DX12Context.CreateDescriptorHeap(l_Desc, L"GlobalCSUDescHeap_ShaderVisible");
        auto l_incrementSize = m_DX12Context.m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

        auto l_GPUBufferDescriptorSectionSize = l_maxGPUBuffers * l_incrementSize;
        auto l_MaterialTextureDescriptorSectionSize = l_maxMaterialTextures * l_incrementSize;
        auto l_RenderTargetTextureDescriptorSectionSize = l_maxRenderTargetTextures * l_incrementSize;

        DescriptorHandle l_firstGPUBufferCBVHandle = {};
        DescriptorHandle l_firstGPUBufferSRVHandle = {};
        DescriptorHandle l_firstGPUBufferUAVHandle = {};

        {
            l_firstGPUBufferCBVHandle.m_CPUHandle = m_DX12Context.m_CSUDescHeap->GetCPUDescriptorHandleForHeapStart().ptr;
            l_firstGPUBufferCBVHandle.m_GPUHandle = m_DX12Context.m_CSUDescHeap->GetGPUDescriptorHandleForHeapStart().ptr;

            m_DX12Context.m_GPUBuffer_CBV_DescHeapAccessor = m_DX12Context.CreateDescriptorHeapAccessor(m_DX12Context.m_CSUDescHeap, l_Desc, l_maxGPUBuffers
                , l_incrementSize, l_firstGPUBufferCBVHandle, true, L"GPUBuffer_CBV_DescHeapAccessor");

            auto l_currentHandle = l_firstGPUBufferCBVHandle;
            D3D12_CONSTANT_BUFFER_VIEW_DESC l_CBVDesc = {};
            for (uint32_t i = 0; i < l_maxGPUBuffers; i++)
            {
                m_DX12Context.m_device->CreateConstantBufferView(nullptr, D3D12_CPU_DESCRIPTOR_HANDLE{ l_currentHandle.m_CPUHandle });
                l_currentHandle.m_CPUHandle += l_incrementSize;
                l_currentHandle.m_GPUHandle += l_incrementSize;
            }
        }

        {
            l_firstGPUBufferSRVHandle.m_CPUHandle = l_firstGPUBufferCBVHandle.m_CPUHandle + l_GPUBufferDescriptorSectionSize;
            l_firstGPUBufferSRVHandle.m_GPUHandle = l_firstGPUBufferCBVHandle.m_GPUHandle + l_GPUBufferDescriptorSectionSize;

            m_DX12Context.m_GPUBuffer_SRV_DescHeapAccessor = m_DX12Context.CreateDescriptorHeapAccessor(m_DX12Context.m_CSUDescHeap, l_Desc, l_maxGPUBuffers
                , l_incrementSize, l_firstGPUBufferSRVHandle, true, L"GPUBuffer_SRV_DescHeapAccessor");

            auto l_currentHandle = l_firstGPUBufferSRVHandle;
            D3D12_SHADER_RESOURCE_VIEW_DESC l_SRVDesc = {};
            l_SRVDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
            l_SRVDesc.Format = DXGI_FORMAT_R32_SINT;
            l_SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
            for (uint32_t i = 0; i < l_maxGPUBuffers; i++)
            {
                m_DX12Context.m_device->CreateShaderResourceView(nullptr, &l_SRVDesc, D3D12_CPU_DESCRIPTOR_HANDLE{ l_currentHandle.m_CPUHandle });
                l_currentHandle.m_CPUHandle += l_incrementSize;
                l_currentHandle.m_GPUHandle += l_incrementSize;
            }
        }

        {
            l_firstGPUBufferUAVHandle.m_CPUHandle = l_firstGPUBufferSRVHandle.m_CPUHandle + l_GPUBufferDescriptorSectionSize;
            l_firstGPUBufferUAVHandle.m_GPUHandle = l_firstGPUBufferSRVHandle.m_GPUHandle + l_GPUBufferDescriptorSectionSize;

            m_DX12Context.m_GPUBuffer_UAV_DescHeapAccessor = m_DX12Context.CreateDescriptorHeapAccessor(m_DX12Context.m_CSUDescHeap, l_Desc, l_maxGPUBuffers
                , l_incrementSize, l_firstGPUBufferUAVHandle, true, L"GPUBuffer_UAV_DescHeapAccessor");

            auto l_currentHandle = l_firstGPUBufferUAVHandle;
            D3D12_UNORDERED_ACCESS_VIEW_DESC l_UAVDesc = {};
            l_UAVDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
            l_UAVDesc.Format = DXGI_FORMAT_R32_SINT;
            for (uint32_t i = 0; i < l_maxGPUBuffers; i++)
            {
                m_DX12Context.m_device->CreateUnorderedAccessView(nullptr, nullptr, &l_UAVDesc, D3D12_CPU_DESCRIPTOR_HANDLE{ l_currentHandle.m_CPUHandle });
                l_currentHandle.m_CPUHandle += l_incrementSize;
                l_currentHandle.m_GPUHandle += l_incrementSize;
            }
        }

        DescriptorHandle l_firstMaterialTextureSRVHandle = {};
        DescriptorHandle l_firstMaterialTextureUAVHandle = {};
        {
            l_firstMaterialTextureSRVHandle.m_CPUHandle = l_firstGPUBufferUAVHandle.m_CPUHandle + l_GPUBufferDescriptorSectionSize;
            l_firstMaterialTextureSRVHandle.m_GPUHandle = l_firstGPUBufferUAVHandle.m_GPUHandle + l_GPUBufferDescriptorSectionSize;

            m_DX12Context.m_MaterialTexture_SRV_DescHeapAccessor = m_DX12Context.CreateDescriptorHeapAccessor(m_DX12Context.m_CSUDescHeap, l_Desc, l_maxMaterialTextures, l_incrementSize
                , l_firstMaterialTextureSRVHandle, true, L"MaterialTexture_SRV_DescHeapAccessor");

            auto l_currentHandle = l_firstMaterialTextureSRVHandle;
            D3D12_SHADER_RESOURCE_VIEW_DESC l_SRVDesc = {};
            l_SRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
            l_SRVDesc.Format = DXGI_FORMAT_R32G32B32A32_SINT;
            l_SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
            for (uint32_t i = 0; i < l_maxMaterialTextures; i++)
            {
                m_DX12Context.m_device->CreateShaderResourceView(nullptr, &l_SRVDesc, D3D12_CPU_DESCRIPTOR_HANDLE{ l_currentHandle.m_CPUHandle });
                l_currentHandle.m_CPUHandle += l_incrementSize;
                l_currentHandle.m_GPUHandle += l_incrementSize;
            }
        }

        {
            l_firstMaterialTextureUAVHandle.m_CPUHandle = l_firstMaterialTextureSRVHandle.m_CPUHandle + l_MaterialTextureDescriptorSectionSize;
            l_firstMaterialTextureUAVHandle.m_GPUHandle = l_firstMaterialTextureSRVHandle.m_GPUHandle + l_MaterialTextureDescriptorSectionSize;

            m_DX12Context.m_MaterialTexture_UAV_DescHeapAccessor = m_DX12Context.CreateDescriptorHeapAccessor(m_DX12Context.m_CSUDescHeap, l_Desc, l_maxMaterialTextures, l_incrementSize
                , l_firstMaterialTextureUAVHandle, true, L"MaterialTexture_UAV_DescHeapAccessor");

            auto l_currentHandle = l_firstMaterialTextureUAVHandle;
            D3D12_UNORDERED_ACCESS_VIEW_DESC l_UAVDesc = {};
            l_UAVDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
            l_UAVDesc.Format = DXGI_FORMAT_R32G32B32A32_SINT;
            for (uint32_t i = 0; i < l_maxMaterialTextures; i++)
            {
                m_DX12Context.m_device->CreateUnorderedAccessView(nullptr, nullptr, &l_UAVDesc, D3D12_CPU_DESCRIPTOR_HANDLE{ l_currentHandle.m_CPUHandle });
                l_currentHandle.m_CPUHandle += l_incrementSize;
                l_currentHandle.m_GPUHandle += l_incrementSize;
            }
        }

        DescriptorHandle l_firstRenderTargetTextureSRVHandle = {};
        DescriptorHandle l_firstRenderTargetTextureUAVHandle = {};
        {
            l_firstRenderTargetTextureSRVHandle.m_CPUHandle = l_firstMaterialTextureUAVHandle.m_CPUHandle + l_MaterialTextureDescriptorSectionSize;
            l_firstRenderTargetTextureSRVHandle.m_GPUHandle = l_firstMaterialTextureUAVHandle.m_GPUHandle + l_MaterialTextureDescriptorSectionSize;

            m_DX12Context.m_RenderTarget_SRV_DescHeapAccessor = m_DX12Context.CreateDescriptorHeapAccessor(m_DX12Context.m_CSUDescHeap, l_Desc, l_maxRenderTargetTextures, l_incrementSize
                , l_firstRenderTargetTextureSRVHandle, true, L"RenderTarget_SRV_DescHeapAccessor");

            auto l_currentHandle = l_firstRenderTargetTextureSRVHandle;
            D3D12_SHADER_RESOURCE_VIEW_DESC l_SRVDesc = {};
            l_SRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
            l_SRVDesc.Format = DXGI_FORMAT_R32G32B32A32_SINT;
            l_SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
            for (uint32_t i = 0; i < l_maxRenderTargetTextures; i++)
            {
                m_DX12Context.m_device->CreateShaderResourceView(nullptr, &l_SRVDesc, D3D12_CPU_DESCRIPTOR_HANDLE{ l_currentHandle.m_CPUHandle });
                l_currentHandle.m_CPUHandle += l_incrementSize;
                l_currentHandle.m_GPUHandle += l_incrementSize;
            }
        }

        {
            l_firstRenderTargetTextureUAVHandle.m_CPUHandle = l_firstRenderTargetTextureSRVHandle.m_CPUHandle + l_RenderTargetTextureDescriptorSectionSize;
            l_firstRenderTargetTextureUAVHandle.m_GPUHandle = l_firstRenderTargetTextureSRVHandle.m_GPUHandle + l_RenderTargetTextureDescriptorSectionSize;

            m_DX12Context.m_RenderTarget_UAV_DescHeapAccessor = m_DX12Context.CreateDescriptorHeapAccessor(m_DX12Context.m_CSUDescHeap, l_Desc, l_maxRenderTargetTextures, l_incrementSize
                , l_firstRenderTargetTextureUAVHandle, true, L"RenderTarget_UAV_DescHeapAccessor");

            auto l_currentHandle = l_firstRenderTargetTextureUAVHandle;
            D3D12_UNORDERED_ACCESS_VIEW_DESC l_UAVDesc = {};
            l_UAVDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
            l_UAVDesc.Format = DXGI_FORMAT_R32G32B32A32_SINT;
            for (uint32_t i = 0; i < l_maxRenderTargetTextures; i++)
            {
                m_DX12Context.m_device->CreateUnorderedAccessView(nullptr, nullptr, &l_UAVDesc, D3D12_CPU_DESCRIPTOR_HANDLE{ l_currentHandle.m_CPUHandle });
                l_currentHandle.m_CPUHandle += l_incrementSize;
                l_currentHandle.m_GPUHandle += l_incrementSize;
            }
        }
    }

    {
        auto l_MaxDescriptorCount = l_maxGPUBuffers + l_maxMaterialTextures + l_maxRenderTargetTextures;
        auto l_Desc = GetDescriptorHeapDesc(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, l_MaxDescriptorCount, false);
        m_DX12Context.m_CSUDescHeap_ShaderNonVisible = m_DX12Context.CreateDescriptorHeap(l_Desc, L"GlobalCSUDescHeap_ShaderNonVisible");

        auto l_incrementSize = m_DX12Context.m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

        auto l_GPUBufferDescriptorSectionSize = l_maxGPUBuffers * l_incrementSize;
        auto l_MaterialTextureDescriptorSectionSize = l_maxMaterialTextures * l_incrementSize;

        DescriptorHandle l_firstGPUBufferUAVHandle = {};
        DescriptorHandle l_firstMaterialTextureBufferUAVHandle = {};
        DescriptorHandle l_firstRenderTargetTextureBufferUAVHandle = {};

        l_firstGPUBufferUAVHandle.m_CPUHandle = m_DX12Context.m_CSUDescHeap_ShaderNonVisible->GetCPUDescriptorHandleForHeapStart().ptr;
        l_firstMaterialTextureBufferUAVHandle.m_CPUHandle = l_firstGPUBufferUAVHandle.m_CPUHandle + l_GPUBufferDescriptorSectionSize;
        l_firstRenderTargetTextureBufferUAVHandle.m_CPUHandle = l_firstMaterialTextureBufferUAVHandle.m_CPUHandle + l_MaterialTextureDescriptorSectionSize;

        {
            m_DX12Context.m_GPUBuffer_UAV_DescHeapAccessor_ShaderNonVisible = m_DX12Context.CreateDescriptorHeapAccessor(m_DX12Context.m_CSUDescHeap_ShaderNonVisible, l_Desc, l_maxGPUBuffers
                , l_incrementSize, l_firstGPUBufferUAVHandle, false, L"GPUBuffer_UAV_DescHeapAccessor_ShaderNonVisible");

            auto l_currentHandle = l_firstGPUBufferUAVHandle;
            D3D12_UNORDERED_ACCESS_VIEW_DESC l_UAVDesc = {};
            l_UAVDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
            l_UAVDesc.Format = DXGI_FORMAT_R32_UINT;
            for (uint32_t i = 0; i < l_maxGPUBuffers; i++)
            {
                m_DX12Context.m_device->CreateUnorderedAccessView(nullptr, nullptr, &l_UAVDesc, D3D12_CPU_DESCRIPTOR_HANDLE{ l_currentHandle.m_CPUHandle });
                l_currentHandle.m_CPUHandle += l_incrementSize;
            }
        }

        {
            m_DX12Context.m_MaterialTexture_UAV_DescHeapAccessor_ShaderNonVisible = m_DX12Context.CreateDescriptorHeapAccessor(m_DX12Context.m_CSUDescHeap_ShaderNonVisible, l_Desc, l_maxMaterialTextures
                , l_incrementSize, l_firstMaterialTextureBufferUAVHandle, false, L"MaterialTexture_UAV_DescHeapAccessor_ShaderNonVisible");

            auto l_currentHandle = l_firstMaterialTextureBufferUAVHandle;
            D3D12_UNORDERED_ACCESS_VIEW_DESC l_UAVDesc = {};
            l_UAVDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
            l_UAVDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            for (uint32_t i = 0; i < l_maxMaterialTextures; i++)
            {
                m_DX12Context.m_device->CreateUnorderedAccessView(nullptr, nullptr, &l_UAVDesc, D3D12_CPU_DESCRIPTOR_HANDLE{ l_currentHandle.m_CPUHandle });
                l_currentHandle.m_CPUHandle += l_incrementSize;
            }
        }

        {
            m_DX12Context.m_RenderTarget_UAV_DescHeapAccessor_ShaderNonVisible = m_DX12Context.CreateDescriptorHeapAccessor(m_DX12Context.m_CSUDescHeap_ShaderNonVisible, l_Desc, l_maxRenderTargetTextures
                , l_incrementSize, l_firstRenderTargetTextureBufferUAVHandle, false, L"RenderTarget_UAV_DescHeapAccessor_ShaderNonVisible");

            auto l_currentHandle = l_firstRenderTargetTextureBufferUAVHandle;
            D3D12_UNORDERED_ACCESS_VIEW_DESC l_UAVDesc = {};
            l_UAVDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE3D;
            l_UAVDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            for (uint32_t i = 0; i < l_maxRenderTargetTextures; i++)
            {
                m_DX12Context.m_device->CreateUnorderedAccessView(nullptr, nullptr, &l_UAVDesc, D3D12_CPU_DESCRIPTOR_HANDLE{ l_currentHandle.m_CPUHandle });
                l_currentHandle.m_CPUHandle += l_incrementSize;
            }
        }
    }

    {
        uint32_t l_maxSamplerCount = 128;
        auto l_Desc = GetDescriptorHeapDesc(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, l_maxSamplerCount, true);
        m_DX12Context.m_SamplerDescHeap = m_DX12Context.CreateDescriptorHeap(l_Desc, L"GlobalSamplerDescHeap");

        DescriptorHandle l_firstSamplerHandle = {};
        l_firstSamplerHandle.m_CPUHandle = m_DX12Context.m_SamplerDescHeap->GetCPUDescriptorHandleForHeapStart().ptr;
        l_firstSamplerHandle.m_GPUHandle = m_DX12Context.m_SamplerDescHeap->GetGPUDescriptorHandleForHeapStart().ptr;

        m_DX12Context.m_SamplerDescHeapAccessor = m_DX12Context.CreateDescriptorHeapAccessor(m_DX12Context.m_SamplerDescHeap, l_Desc, l_maxSamplerCount, m_DX12Context.m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER), l_firstSamplerHandle, true, L"SamplerDescHeapAccessor");
    }

    {
        uint32_t l_maxRTVCount = 256;
        auto l_Desc = GetDescriptorHeapDesc(D3D12_DESCRIPTOR_HEAP_TYPE_RTV, l_maxRTVCount, false);
        m_DX12Context.m_RTVDescHeap = m_DX12Context.CreateDescriptorHeap(l_Desc, L"GlobalRTVDescHeap");

        DescriptorHandle l_firstRTVHandle = {};
        l_firstRTVHandle.m_CPUHandle = m_DX12Context.m_RTVDescHeap->GetCPUDescriptorHandleForHeapStart().ptr;

        m_DX12Context.m_RTVDescHeapAccessor = m_DX12Context.CreateDescriptorHeapAccessor(m_DX12Context.m_RTVDescHeap, l_Desc, l_maxRTVCount, m_DX12Context.m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV), l_firstRTVHandle, false, L"RTVDescHeapAccessor");
    }

    {
        uint32_t l_maxDSVCount = 256;
        auto l_Desc = GetDescriptorHeapDesc(D3D12_DESCRIPTOR_HEAP_TYPE_DSV, l_maxDSVCount, false);
        m_DX12Context.m_DSVDescHeap = m_DX12Context.CreateDescriptorHeap(l_Desc, L"GlobalDSVDescHeap");

        DescriptorHandle l_firstDSVHandle = {};
        l_firstDSVHandle.m_CPUHandle = m_DX12Context.m_DSVDescHeap->GetCPUDescriptorHandleForHeapStart().ptr;

        m_DX12Context.m_DSVDescHeapAccessor = m_DX12Context.CreateDescriptorHeapAccessor(m_DX12Context.m_DSVDescHeap, l_Desc, l_maxDSVCount, m_DX12Context.m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV), l_firstDSVHandle, false, L"DSVDescHeapAccessor");
    }

    return true;
}

bool DX12GraphicsHardwareService::CreateHardwareResources()
{
    bool l_result = true;

    TryLoadRenderDocAPI();

#if defined(INNO_DEBUG) || defined(INNO_RELWITHDEBINFO)
    if (g_Engine->getInitConfig().enableGPUValidation)
        l_result &= CreateDebugCallback();
    else
        Log(Warning, "D3D12 debug layer disabled by default to avoid TDR from validation overhead. Pass -gpu_validation to enable.");
#endif
    l_result &= CreatePhysicalDevices();
    l_result &= CreateGlobalCommandQueues();
    l_result &= CreateGlobalCommandAllocators();
    l_result &= CreateSyncPrimitives();
    l_result &= CreateGlobalDescriptorHeaps();

    auto l_textureService = static_cast<DX12TextureResourceService*>(g_Engine->Get<TextureResourceService>());
    auto l_gpuBufferService = static_cast<DX12GPUBufferResourceService*>(g_Engine->Get<GPUBufferResourceService>());
    l_result &= l_textureService->CreateMipmapGenerator();
    l_result &= l_gpuBufferService->CreateRaytracingResources();

    return l_result;
}

bool DX12GraphicsHardwareService::ReleaseHardwareResources()
{
    auto l_gpuBufferService = static_cast<DX12GPUBufferResourceService*>(g_Engine->Get<GPUBufferResourceService>());
    auto l_textureService = static_cast<DX12TextureResourceService*>(g_Engine->Get<TextureResourceService>());
    l_gpuBufferService->ReleaseRaytracingResources();
    l_textureService->ReleaseMipmapGenerator();

    m_DX12Context.m_SamplerDescHeapAccessor.Reset();
    m_DX12Context.m_SamplerDescHeap = nullptr;

    m_DX12Context.m_DSVDescHeapAccessor.Reset();
    m_DX12Context.m_DSVDescHeap = nullptr;

    m_DX12Context.m_RTVDescHeapAccessor.Reset();
    m_DX12Context.m_RTVDescHeap = nullptr;

    m_DX12Context.m_RenderTarget_UAV_DescHeapAccessor_ShaderNonVisible.Reset();
    m_DX12Context.m_MaterialTexture_UAV_DescHeapAccessor_ShaderNonVisible.Reset();
    m_DX12Context.m_GPUBuffer_UAV_DescHeapAccessor_ShaderNonVisible.Reset();
    m_DX12Context.m_CSUDescHeap_ShaderNonVisible = nullptr;

    m_DX12Context.m_RenderTarget_UAV_DescHeapAccessor.Reset();
    m_DX12Context.m_MaterialTexture_UAV_DescHeapAccessor.Reset();
    m_DX12Context.m_GPUBuffer_UAV_DescHeapAccessor.Reset();
    m_DX12Context.m_RenderTarget_SRV_DescHeapAccessor.Reset();
    m_DX12Context.m_MaterialTexture_SRV_DescHeapAccessor.Reset();
    m_DX12Context.m_GPUBuffer_SRV_DescHeapAccessor.Reset();
    m_DX12Context.m_GPUBuffer_CBV_DescHeapAccessor.Reset();
    m_DX12Context.m_CSUDescHeap = nullptr;

    m_DX12Context.m_directCommandQueueFence = nullptr;
    m_DX12Context.m_computeCommandQueueFence = nullptr;
    m_DX12Context.m_copyCommandQueueFence = nullptr;

    m_DX12Context.m_directCommandAllocators.clear();
    m_DX12Context.m_computeCommandAllocators.clear();
    m_DX12Context.m_copyCommandAllocators.clear();

    m_DX12Context.m_directCommandQueue = nullptr;
    m_DX12Context.m_computeCommandQueue = nullptr;
    m_DX12Context.m_copyCommandQueue = nullptr;

    try
    {
        if (m_DX12Context.m_debugInterface && m_DX12Context.m_debugCallbackCookie != 0)
        {
            ComPtr<ID3D12InfoQueue> l_pInfoQueue;
            if (SUCCEEDED(m_DX12Context.m_device->QueryInterface(IID_PPV_ARGS(&l_pInfoQueue))) && l_pInfoQueue)
            {
                ComPtr<ID3D12InfoQueue1> l_pInfoQueue1;
                if (SUCCEEDED(l_pInfoQueue.As(&l_pInfoQueue1)) && l_pInfoQueue1)
                {
                    l_pInfoQueue1->UnregisterMessageCallback(m_DX12Context.m_debugCallbackCookie);
                    m_DX12Context.m_debugCallbackCookie = 0;
                }
            }
        }
    }
    catch (...)
    {
        Log(Warning, "Exception during debug callback cleanup, device may already be removed.");
    }

    m_DX12Context.m_device = nullptr;
    m_DX12Context.m_adapterOutput = nullptr;
    m_DX12Context.m_adapter = nullptr;
    m_DX12Context.m_factory = nullptr;
    m_DX12Context.m_graphicsAnalysis = nullptr;
    m_DX12Context.m_debugInterface = nullptr;

#if defined(INNO_DEBUG) || defined(INNO_RELWITHDEBINFO)
    try
    {
        IDXGIDebug1* pDebug = nullptr;
        if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&pDebug))))
        {
            pDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL);
            pDebug->Release();
        }
    }
    catch (...)
    {
        Log(Warning, "Exception during DXGI debug report, skipping.");
    }
#endif

    return true;
}
