#include "LightComponent.h"


LightComponent::LightComponent()
{
}


LightComponent::~LightComponent()
{
}

mat4 LightComponent::getProjectionMatrix(unsigned int cascadedLevel) const
{
	auto l_boundMax = m_AABBs[cascadedLevel].m_boundMax;
	auto l_boundMin = m_AABBs[cascadedLevel].m_boundMin;

	mat4 p;
	p.initializeToOrthographicMatrix(l_boundMin.x, l_boundMax.x, l_boundMin.y, l_boundMax.y, l_boundMin.z, l_boundMax.z);

	return p;
}

void LightComponent::setup()
{
}

void LightComponent::initialize()
{
}

void LightComponent::shutdown()
{
}
