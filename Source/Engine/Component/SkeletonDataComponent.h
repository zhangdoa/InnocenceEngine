#pragma once
#include "../Common/InnoObject.h"
#include "../Common/InnoMathHelper.h"

namespace Inno
{
	struct BoneData
	{
		Mat4 m_L2B;
	};

	class SkeletonDataComponent : public InnoComponent
	{
	public:
		static uint32_t GetTypeID() { return 9; };
		static char* GetTypeName() { return "SkeletonDataComponent"; };

		Array<BoneData> m_BoneData;
	};
}