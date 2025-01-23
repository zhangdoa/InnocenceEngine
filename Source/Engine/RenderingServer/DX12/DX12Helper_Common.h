#pragma once
#include "../../Common/LogService.h"
#include "../../Common/Object.h"

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

		template <typename T>
		bool SetObjectName(Object* owner, ComPtr<T> rhs, const char* objectType)
		{
			auto l_Name = std::string(owner->m_InstanceName.c_str());
			l_Name += "_";
			l_Name += objectType;
			auto l_NameW = std::wstring(l_Name.begin(), l_Name.end());
			return SetObjectName(l_NameW.c_str(), rhs, objectType);
		}
	}
}