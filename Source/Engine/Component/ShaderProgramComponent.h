#pragma once
#include "../Common/Type.h"
#include "../Common/Object.h"

namespace Inno
{
	class ShaderProgramComponent : public Component
	{
	public:
		static uint32_t GetTypeID() { return 12; };
		static const char* GetTypeName() { return "ShaderProgramComponent"; };

		ShaderFilePaths m_ShaderFilePaths = {};
	};
}