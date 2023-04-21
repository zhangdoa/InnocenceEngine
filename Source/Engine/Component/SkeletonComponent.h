#pragma once
#include "../Common/Object.h"
#include "../Common/MathHelper.h"

namespace Inno
{
	struct BoneData
	{
		Mat4 m_L2B;
	};

	class SkeletonComponent : public Component
	{
	public:
		static uint32_t GetTypeID() { return 9; };
		static const char* GetTypeName() { return "SkeletonComponent"; };

		Array<BoneData> m_BoneData;
	};
}