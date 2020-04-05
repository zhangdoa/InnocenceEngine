#pragma once
#include "../Common/InnoComponent.h"
#include "../Common/InnoMathHelper.h"

struct ChannelInfo
{
	uint32_t keyOffsets;
	uint32_t numKeys;
};

struct KeyData
{
	Vec4 pos;
	Vec4 rot;
};

class AnimationDataComponent : public InnoComponent
{
public:
	float m_Duration = 0.0f;
	uint32_t m_NumChannels = 0;
	// The offset and range in m_KeyData
	std::vector<ChannelInfo> m_ChannelInfo;
	// for each channel first, then for each key
	std::vector<KeyData> m_KeyData;
};
