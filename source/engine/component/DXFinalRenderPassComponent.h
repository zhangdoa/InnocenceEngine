#pragma once
#include "../common/InnoType.h"
#include "../system/DXHeaders.h"

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

	DXShaderProgramComponent* m_DXSPC;

private:
	DXFinalRenderPassComponent() {};
};
