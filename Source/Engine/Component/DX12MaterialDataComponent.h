#pragma once
#include "MaterialDataComponent.h"
#include "../RenderingBackend/DX12RenderingBackend/DX12Headers.h"

class DX12MaterialDataComponent : public MaterialDataComponent
{
public:
	DX12MaterialDataComponent() {};
	~DX12MaterialDataComponent() {};

	std::vector<DX12SRV> m_SRVs;
};
