#pragma once
#include "../common/InnoType.h"
#include "DXShaderProgramComponent.h"
#include "DXRenderPassComponent.h"

class DXLightRenderPassComponent
{
public:
	~DXLightRenderPassComponent() {};
	
	static DXLightRenderPassComponent& get()
	{
		static DXLightRenderPassComponent instance;
		return instance;
	}

	ObjectStatus m_objectStatus = ObjectStatus::SHUTDOWN;
	EntityID m_parentEntity;

	DXShaderProgramComponent* m_DXSPC;

	DXRenderPassComponent* m_DXRPC;

private:
	DXLightRenderPassComponent() {};
};
