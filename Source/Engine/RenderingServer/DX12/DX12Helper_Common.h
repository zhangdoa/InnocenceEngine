#pragma once
#include "../../Common/LogService.h"
#include "../../Component/DX12TextureComponent.h"
#include "../../Component/DX12RenderPassComponent.h"
#include "../../Component/DX12ShaderProgramComponent.h"
#include "../IRenderingServer.h"

namespace Inno
{
	namespace DX12Helper
	{
		template <typename U, typename T>
		bool SetObjectName(U* owner, ComPtr<T> rhs, const char* objectType)
		{
			auto l_Name = std::string(owner->m_InstanceName.c_str());
			l_Name += "_";
			l_Name += objectType;
			auto l_NameW = std::wstring(l_Name.begin(), l_Name.end());
			auto l_HResult = rhs->SetName(l_NameW.c_str());
			if (FAILED(l_HResult))
			{
				Log(Warning, "Can't name ", objectType, " with ", l_Name.c_str());
				return false;
			}
			return true;
		}
	}
}