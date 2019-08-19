#pragma once
#include "../Common/InnoComponent.h"
#include "../Common/InnoMath.h"
#include "TextureDataComponent.h"

class AnimationDataComponent : public InnoComponent
{
public:
	TextureDataComponent* m_animationTexture = 0;
};
