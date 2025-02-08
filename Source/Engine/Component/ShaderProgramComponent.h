#pragma once
#include "../Common/Object.h"

namespace Inno
{
	using ShaderFilePath = FixedSizeString<128>;

	struct ShaderFilePaths
	{
		ShaderFilePath m_VSPath = "";
		ShaderFilePath m_HSPath = "";
		ShaderFilePath m_DSPath = "";
		ShaderFilePath m_GSPath = "";
		ShaderFilePath m_PSPath = "";
		ShaderFilePath m_CSPath = "";
		ShaderFilePath m_RayGenPath = "";
		ShaderFilePath m_AnyHitPath = "";
		ShaderFilePath m_ClosestHitPath = "";
		ShaderFilePath m_MissPath = "";
	};

	class ShaderProgramComponent : public Component
	{
	public:
		static uint32_t GetTypeID() { return 12; };
		static const char* GetTypeName() { return "ShaderProgramComponent"; };

		ShaderFilePaths m_ShaderFilePaths = {};
	};
}