#pragma once
#include "../Common/InnoType.h"
#include "MeshComponent.h"

namespace Inno
{
	class MTMeshComponent : public MeshComponent
	{
	public:
		void* m_VBO = 0;
		void* m_IBO = 0;
	};
}
