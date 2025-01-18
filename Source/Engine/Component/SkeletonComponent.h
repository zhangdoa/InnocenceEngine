#pragma once
#include "../Common/Object.h"
#include "../Common/Array.h"
#include "../Common/MathHelper.h"

#include "MeshComponent.h"

namespace Inno
{
	struct Bone
	{
		Mat4 m_LocalToBoneSpace;
	};

	class SkeletonComponent : public Component
	{
	public:
		static uint32_t GetTypeID() { return 9; };
		static const char* GetTypeName() { return "SkeletonComponent"; };

		Array<Bone> m_BoneList;
		MeshComponent* m_Mesh;
	};
}