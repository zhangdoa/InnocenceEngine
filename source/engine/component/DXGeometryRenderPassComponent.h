#pragma once
#include "../common/InnoType.h"
#include "DXShaderProgramComponent.h"
#include "DXRenderPassComponent.h"

class DXGeometryRenderPassComponent
{
public:
	~DXGeometryRenderPassComponent() {};
	
	static DXGeometryRenderPassComponent& get()
	{
		static DXGeometryRenderPassComponent instance;
		return instance;
	}

	ObjectStatus m_objectStatus = ObjectStatus::SHUTDOWN;
	EntityID m_parentEntity;

	DXShaderProgramComponent* m_DXSPC;

	DXRenderPassComponent* m_DXRPC;

private:
	DXGeometryRenderPassComponent() {};
};
