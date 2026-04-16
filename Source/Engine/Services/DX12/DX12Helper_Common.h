#pragma once
#include "../../Engine.h"
#include "../../Common/LogService.h"
#include "../../Common/Object.h"
#include "../../Component/GPUResourceComponent.h"

#include "DX12Headers.h"

namespace Inno
{
	namespace DX12Helper
	{
		template <typename T>
		bool SetObjectName(const wchar_t* name, ComPtr<T> rhs, const char* objectType)
		{
			auto l_HResult = rhs->SetName(name);
			if (FAILED(l_HResult))
			{
				Log(Warning, "Can't name ", objectType, " with ", name);
				return false;
			}
			return true;
		}

		template <typename TOwner, typename T>
		bool SetObjectName(TOwner* owner, ComPtr<T> rhs, const char* objectType)
		{
			auto l_Name = std::string(owner->m_InstanceName.c_str());
			l_Name += "_";
			l_Name += objectType;
			auto l_NameW = std::wstring(l_Name.begin(), l_Name.end());
			return SetObjectName(l_NameW.c_str(), rhs, objectType);
		}

		template <typename T>
		bool SetObjectName(const char* ownerName, ComPtr<T> rhs, const char* objectType)
		{
			auto l_Name = std::string(ownerName);
			l_Name += "_";
			l_Name += objectType;
			auto l_NameW = std::wstring(l_Name.begin(), l_Name.end());
			return SetObjectName(l_NameW.c_str(), rhs, objectType);
		}

		// Consistent failure logging for DX12 resource/PSO/root-signature creation paths (TASK-34).
		// Always logs HRESULT; includes DeviceRemovedReason when the device is present.
		// Calls LogService::Print directly because the Log(level, ...) macro token-pastes
		// `level` into `LogLevel::level` and only accepts a literal enumerator name.
		inline void LogD3D12CreateFailure(ID3D12Device* device, const char* what, const char* contextName, HRESULT hr)
		{
			HRESULT l_drr = device ? device->GetDeviceRemovedReason() : S_OK;
			g_Engine->Get<LogService>()->Print(LogLevel::Error, __FUNCTION__,
				"DX12 create failed: ", what, " context=", contextName ? contextName : "(null)",
				" HRESULT=", static_cast<int32_t>(hr),
				" DeviceRemovedReason=", static_cast<int32_t>(l_drr));
		}

		inline void LogD3D12CreateFailure(ID3D12Device* device, const char* what, const wchar_t* contextName, HRESULT hr)
		{
			HRESULT l_drr = device ? device->GetDeviceRemovedReason() : S_OK;
			g_Engine->Get<LogService>()->Print(LogLevel::Error, __FUNCTION__,
				"DX12 create failed: ", what, " context=", contextName ? contextName : L"(null)",
				" HRESULT=", static_cast<int32_t>(hr),
				" DeviceRemovedReason=", static_cast<int32_t>(l_drr));
		}
	}
}