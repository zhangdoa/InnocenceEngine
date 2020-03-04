#pragma once
#include "../Common/InnoComponent.h"
#include "../Common/InnoMathHelper.h"

class AnimationDataComponent : public InnoComponent
{
public:
	float m_Duration = 0.0f;
	uint32_t m_NumChannels = 0;
	uint32_t m_NumKeys = 0;
	Array<Vec4> m_KeyData;
};
