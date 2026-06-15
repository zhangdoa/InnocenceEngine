#include "DX12GraphicsHardwareService.h"
#include "../../Engine.h"
#include "../../Common/LogService.h"
#include "../ConfigurationService.h"

#ifdef _WIN32
#include <Windows.h>
#endif

using namespace Inno;

extern void CALLBACK D3D12DebugMessageCallback(
    D3D12_MESSAGE_CATEGORY Category,
    D3D12_MESSAGE_SEVERITY Severity,
    D3D12_MESSAGE_ID ID,
    LPCSTR pDescription,
    void* pContext);

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

        if (g_Engine->Get<ConfigurationService>()->IsEnableGPUValidation())
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

    // DRED settings must be enabled before device creation; the device snapshots
    // them at construction time.
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
            // CORRUPTION is unrecoverable; break helps a debugger catch it at the site.
            l_pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
            // SetBreakOnSeverity(ERROR, TRUE) would RaiseException before
            // D3D12DebugMessageCallback runs, turning Release-shader GBV
            // false-positives into unrecoverable crashes — keep it FALSE so the
            // callback's classifier sees them first.
            l_pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, FALSE);
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
