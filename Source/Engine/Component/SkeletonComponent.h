#pragma once
#include "../Common/Array.h"
#include "../Common/MathHelper.h"

#include "MeshComponent.h"

namespace Inno
{
	struct Bone
	{
		Mat4 m_LocalToBoneSpace;
	};

	struct SkeletonComponent
	{
		Array<Bone> m_BoneList;
		MeshComponent* m_Mesh;
	};
}