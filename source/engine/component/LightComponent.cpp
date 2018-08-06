#include "LightComponent.h"

mat4 LightComponent::getProjectionMatrix(unsigned int cascadedLevel) const
{
	auto l_center = m_AABBs[cascadedLevel].m_center;
	double l_radius = 0.0;
	for (unsigned int i = 0; i < 8; ++i)
	{
		auto l_vertex = m_AABBs[cascadedLevel].m_vertices[i];

		double l_distance = (l_vertex.m_pos - l_center).length();
		l_radius = std::max(l_radius, l_distance);
	}
	vec4 l_maxExtents = vec4(l_radius, l_radius, l_radius, 1.0);
	vec4 l_minExtents = l_maxExtents * (-1.0);
	vec4 l_cascadeExtents = l_maxExtents - l_minExtents;
	mat4 p;
	p.initializeToOrthographicMatrix(l_minExtents.x, l_maxExtents.x, l_minExtents.y, l_maxExtents.y, l_minExtents.z, l_maxExtents.z);
	return p;
}