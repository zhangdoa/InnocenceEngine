#pragma once
#include "../common/InnoType.h"
#include "DXShaderProgramComponent.h"

class DXFinalRenderPassComponent
{
public:
	~DXFinalRenderPassComponent() {};
	
	static DXFinalRenderPassComponent& get()
	{
		static DXFinalRenderPassComponent instance;
		return instance;
	}

	ObjectStatus m_objectStatus = ObjectStatus::SHUTDOWN;
	EntityID m_parentEntity;

	ShaderFilePaths m_finalPass_shaderFilePaths = { "DX11//finalBlendPassVertex.sf" , "", "DX11//finalBlendPassPixel.sf" };

	DXShaderProgramComponent* m_DXSPC;

private:
	DXFinalRenderPassComponent() {};
};
