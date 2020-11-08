#pragma once
#include "../Common/InnoType.h"
#include "../Common/InnoObject.h"

namespace Inno
{
	class ShaderProgramComponent : public InnoComponent
	{
	public:
		static uint32_t GetTypeID() { return 12; };
		static const char* GetTypeName() { return "ShaderProgramComponent"; };

		ShaderFilePaths m_ShaderFilePaths = {};
	};
}