#pragma once
#include "../Common/InnoType.h"
#include "MeshDataComponent.h"

class MTMeshDataComponent : public MeshDataComponent
{
public:
	void* m_VBO = 0;
	void* m_IBO = 0;
};
