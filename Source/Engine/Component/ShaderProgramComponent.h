#pragma once
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
	};

	struct ShaderProgramComponent
	{
		ObjectStatus    m_ObjectStatus    = ObjectStatus::Invalid;
		ObjectName      m_InstanceName    = "";

		ShaderFilePaths m_ShaderFilePaths = {};

		std::vector<uint8_t> m_VSBuffer;
		std::vector<uint8_t> m_HSBuffer;
		std::vector<uint8_t> m_DSBuffer;
		std::vector<uint8_t> m_GSBuffer;
		std::vector<uint8_t> m_PSBuffer;
		std::vector<uint8_t> m_CSBuffer;
		std::vector<uint8_t> m_RayGenBuffer;
		std::vector<uint8_t> m_AnyHitBuffer;
		std::vector<uint8_t> m_ClosestHitBuffer;
		std::vector<uint8_t> m_MissBuffer;
	};
}
