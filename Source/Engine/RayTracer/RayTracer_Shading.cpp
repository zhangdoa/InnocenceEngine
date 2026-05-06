#include "RayTracer_Internal.h"

using namespace Inno;
using namespace RayTracerNS;

Vec4 SkyColor(const Ray& r)
{
	Vec4 unitDir = r.m_direction.normalize();
	float t = unitDir.y * 0.5f + 0.5f;
	Vec4 skyAmbient = Math::lerp(Vec4(0.5f, 0.7f, 1.0f, 1.0f), Vec4(1.0f, 1.0f, 1.0f, 1.0f), t);
	float sunDot  = std::max(0.0f, unitDir * m_sunDir);
	float sunDisc = std::pow(sunDot, 128.0f);
	return skyAmbient + m_sunColor * (sunDisc * 4.0f);
}

Vec4 CalcRadiance(const Ray& r, Hitable* world, int32_t depth)
{
	if (depth >= m_maxDepth)
		return SkyColor(r);

	HitResult l_result;
	if (world->Hit(r, 0.001f, std::numeric_limits<float>::infinity(), l_result))
	{
		Ray scattered;
		Vec4 attenuation;
		if (l_result.HitMaterial->scatter(r, l_result, attenuation, scattered))
		{
			Vec4 directSun(0.0f, 0.0f, 0.0f, 1.0f);
			float NdotL = l_result.HitNormal * m_sunDir;
			if (NdotL > 0.0f)
			{
				Ray shadowRay;
				shadowRay.m_origin    = l_result.HitPoint;
				shadowRay.m_direction = m_sunDir;
				HitResult shadowHit;
				if (!world->Hit(shadowRay, 0.001f, std::numeric_limits<float>::infinity(), shadowHit))
					directSun = attenuation.scale(m_sunColor) * NdotL;
			}
			return directSun + attenuation.scale(CalcRadiance(scattered, world, depth + 1));
		}
		else
			return attenuation;
	}

	return SkyColor(r);
}
