#pragma once
#include "../Common/InnoComponent.h"
#include "../Common/InnoMath.h"

struct Key
{
	vec4 m_Pos;
	vec4 m_Rot;
	double m_Time;
};

struct Channel
{
	unsigned int m_ChannelID;
	unsigned int m_NumKeys;
	Key* m_Keys;
};

class AnimationDataComponent : public InnoComponent
{
public:
	unsigned int m_NumChannels;
	Channel* m_Channels;
};
