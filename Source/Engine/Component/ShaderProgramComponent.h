#pragma once
#include "../Common/Array.h"
#include "../Common/Object.h"
#include "../Common/STL14.h"

namespace Inno
{
	using ShaderFilePath = FixedSizeString<128>;

	struct ShaderFilePaths
	{
		ShaderFilePath m_VSPath         = "";
		ShaderFilePath m_HSPath         = "";
		ShaderFilePath m_DSPath         = "";
		ShaderFilePath m_GSPath         = "";
		ShaderFilePath m_PSPath         = "";
		ShaderFilePath m_CSPath         = "";
		ShaderFilePath m_RayGenPath     = "";
		ShaderFilePath m_AnyHitPath     = "";
		ShaderFilePath m_ClosestHitPath = "";
		ShaderFilePath m_MissPath       = "";
		ShaderFilePath m_ShadowMissPath = "";
	};

	struct ShaderProgramComponent
	{
		ObjectStatus    m_ObjectStatus    = ObjectStatus::Invalid;
		ObjectName      m_InstanceName    = "";

		ShaderFilePaths m_ShaderFilePaths = {};

		Inno::Array<uint8_t> m_VSBuffer;
		Inno::Array<uint8_t> m_HSBuffer;
		Inno::Array<uint8_t> m_DSBuffer;
		Inno::Array<uint8_t> m_GSBuffer;
		Inno::Array<uint8_t> m_PSBuffer;
		Inno::Array<uint8_t> m_CSBuffer;
		Inno::Array<uint8_t> m_RayGenBuffer;
		Inno::Array<uint8_t> m_AnyHitBuffer;
		Inno::Array<uint8_t> m_ClosestHitBuffer;
		Inno::Array<uint8_t> m_MissBuffer;
		Inno::Array<uint8_t> m_ShadowMissBuffer;
	};
}
