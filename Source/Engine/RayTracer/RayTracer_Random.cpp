#include "RayTracer_Internal.h"

using namespace Inno;
using namespace RayTracerNS;

Vec4 RandomDirectionInUnitDisk()
{
	Vec4 p;
	do {
		p = Vec4(m_randomDirDelta(m_generator), m_randomDirDelta(m_generator), 0.0f, 0.0f);
	} while (p * p >= 1.0f);
	return p;
}

Vec4 RandomDirectionInUnitSphere()
{
	Vec4 p;
	do {
		p = Vec4(m_randomDirDelta(m_generator), m_randomDirDelta(m_generator), m_randomDirDelta(m_generator), 0.0f);
	} while (p * p >= 1.0f);
	return p;
}

Vec4 RandomUnitVector()
{
	auto a = (m_randomDirDelta(m_generator) + 1.0f) * PI<float>;
	auto z = m_randomDirDelta(m_generator);
	auto r = sqrt(1 - z * z);
	return Vec4(r * cos(a), r * sin(a), z, 0.0f);
}

Vec4 Reflect(const Vec4& v, const Vec4& n)
{
	float NdotL = v * n;
	return v - n * 2 * NdotL;
}
