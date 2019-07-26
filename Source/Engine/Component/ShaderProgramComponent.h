#pragma once
#include "../Common/InnoType.h"
#include "../Common/InnoComponent.h"

class ShaderProgramComponent : public InnoComponent
{
public:
	ShaderFilePaths m_ShaderFilePaths = {};
};
