#pragma once
#include "../../Engine.h"
#include "../../Common/LogService.h"
#include "../../Component/CommandListComponent.h"

#include "DX12Headers.h"

namespace Inno
{
	namespace DX12Helper
	{
		// Centralised typed access to DX12 command lists. CommandListComponent stores the COM
		// pointer as a uint64_t for API-agnostic abstraction; callers reinterpret_cast it back.
		// This helper does that cast in one place with a non-null guard, so every call site
		// sees either a valid list or nullptr (and a logged error) — no silent UB on a zero
		// handle.
		inline ID3D12GraphicsCommandList7* AsDX12CommandList(CommandListComponent* commandList)
		{
			if (commandList == nullptr || commandList->m_CommandList == 0)
			{
				Log(Error, "DX12: AsDX12CommandList called with null or unbacked CommandListComponent.");
				return nullptr;
			}
			return reinterpret_cast<ID3D12GraphicsCommandList7*>(commandList->m_CommandList);
		}

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

		// Post-mortem dump of DRED (Device Removed Extended Data) state.
		// Safe to call with or without a removed device; emits nothing if the DRED interface is unavailable.
		// Single implementation shared by every DX12 failure path that wants breadcrumb/page-fault context.
		inline void DumpDRED(ID3D12Device* device)
		{
			if (!device)
				return;
			try
			{
				ComPtr<ID3D12DeviceRemovedExtendedData1> l_pDred;
				if (FAILED(device->QueryInterface(IID_PPV_ARGS(&l_pDred))))
				{
					g_Engine->Get<LogService>()->Print(LogLevel::Warning, __FUNCTION__,
						"DRED: interface not available for post-mortem.");
					return;
				}

				D3D12_DRED_AUTO_BREADCRUMBS_OUTPUT1 l_breadcrumbs = {};
				if (SUCCEEDED(l_pDred->GetAutoBreadcrumbsOutput1(&l_breadcrumbs)))
				{
					const D3D12_AUTO_BREADCRUMB_NODE1* l_node = l_breadcrumbs.pHeadAutoBreadcrumbNode;
					int l_nodeIndex = 0;
					while (l_node)
					{
						if (l_node->pLastBreadcrumbValue && l_node->pCommandListDebugNameW)
						{
							uint32_t l_lastCompleted = *l_node->pLastBreadcrumbValue;
							g_Engine->Get<LogService>()->Print(LogLevel::Warning, __FUNCTION__,
								"DRED Breadcrumb[", l_nodeIndex, "]: CL='",
								l_node->pCommandListDebugNameW,
								"' Queue='",
								l_node->pCommandQueueDebugNameW ? l_node->pCommandQueueDebugNameW : L"(null)",
								"' LastCompleted=", l_lastCompleted, "/", l_node->BreadcrumbCount);

							for (uint32_t i = 0; i < l_node->BreadcrumbCount; i++)
							{
								const char* l_status = (i < l_lastCompleted) ? "DONE" : (i == l_lastCompleted) ? ">>LAST>>" : "pending";
								g_Engine->Get<LogService>()->Print(LogLevel::Warning, __FUNCTION__,
									"  [", i, "] op=", static_cast<int>(l_node->pCommandHistory[i]), " ", l_status);
							}
						}
						l_node = l_node->pNext;
						l_nodeIndex++;
					}
				}

				D3D12_DRED_PAGE_FAULT_OUTPUT l_pageFault = {};
				if (SUCCEEDED(l_pDred->GetPageFaultAllocationOutput(&l_pageFault)))
				{
					if (l_pageFault.PageFaultVA != 0)
					{
						g_Engine->Get<LogService>()->Print(LogLevel::Warning, __FUNCTION__,
							"DRED Page Fault at VA=0x", l_pageFault.PageFaultVA);

						// Walk the existing-allocations list — resources still live at fault time that
						// cover the faulting VA. Helps attribute the fault to a named D3D12 resource.
						int l_existingIdx = 0;
						for (const D3D12_DRED_ALLOCATION_NODE* l_n = l_pageFault.pHeadExistingAllocationNode;
							l_n != nullptr && l_existingIdx < 32; l_n = l_n->pNext, ++l_existingIdx)
						{
							g_Engine->Get<LogService>()->Print(LogLevel::Warning, __FUNCTION__,
								"DRED Existing[", l_existingIdx, "]: name='",
								l_n->ObjectNameW ? l_n->ObjectNameW : L"(null)",
								"' type=", static_cast<int>(l_n->AllocationType));
						}

						// Walk the recently-freed-allocations list — resources whose VA range
						// overlaps the faulting VA. This typically identifies a use-after-free.
						int l_freedIdx = 0;
						for (const D3D12_DRED_ALLOCATION_NODE* l_n = l_pageFault.pHeadRecentFreedAllocationNode;
							l_n != nullptr && l_freedIdx < 32; l_n = l_n->pNext, ++l_freedIdx)
						{
							g_Engine->Get<LogService>()->Print(LogLevel::Warning, __FUNCTION__,
								"DRED RecentlyFreed[", l_freedIdx, "]: name='",
								l_n->ObjectNameW ? l_n->ObjectNameW : L"(null)",
								"' type=", static_cast<int>(l_n->AllocationType));
						}
					}
				}
			}
			catch (...)
			{
				g_Engine->Get<LogService>()->Print(LogLevel::Warning, __FUNCTION__,
					"DRED: exception during post-mortem query, skipping.");
			}
		}
	}
}