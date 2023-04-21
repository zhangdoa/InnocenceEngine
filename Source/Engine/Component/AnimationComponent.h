#pragma once
#include "../Common/Object.h"
#include "../Common/MathHelper.h"

namespace Inno
{
	struct KeyData
	{
		Mat4 m;
	};

	class AnimationComponent : public Component
	{
	public:
		static uint32_t GetTypeID() { return 10; };
		static const char* GetTypeName() { return "AnimationComponent"; };

		float m_Duration = 0.0f;
		uint32_t m_NumChannels = 0;
		uint32_t m_NumTicks = 0;
		// for each tick first, then for each channel
		std::vector<KeyData> m_KeyData;
	};
}