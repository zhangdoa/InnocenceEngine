#pragma once
#include "../Common/InnoComponent.h"
#include "../Common/InnoMathHelper.h"
#include "TextureDataComponent.h"

class AnimationDataComponent : public InnoComponent
{
public:
	TextureDataComponent* m_animationTexture = 0;
};
