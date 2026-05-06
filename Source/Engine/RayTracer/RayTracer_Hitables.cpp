#include "RayTracer_Internal.h"

using namespace Inno;

bool HitableCube::Hit(const Ray& r, float tMin, float tMax, HitResult& hitResult)
{
	float t1 = (m_AABB.m_boundMin.x - r.m_origin.x) / r.m_direction.x;
	float t2 = (m_AABB.m_boundMax.x - r.m_origin.x) / r.m_direction.x;
	float t3 = (m_AABB.m_boundMin.y - r.m_origin.y) / r.m_direction.y;
	float t4 = (m_AABB.m_boundMax.y - r.m_origin.y) / r.m_direction.y;
	float t5 = (m_AABB.m_boundMin.z - r.m_origin.z) / r.m_direction.z;
	float t6 = (m_AABB.m_boundMax.z - r.m_origin.z) / r.m_direction.z;

	float tXmin = std::min(t1, t2);
	float tYmin = std::min(t3, t4);
	float tZmin = std::min(t5, t6);

	float tXmax = std::max(t1, t2);
	float tYmax = std::max(t3, t4);
	float tZmax = std::max(t5, t6);

	int axisEntry = 0;
	float tminVal = tXmin;
	if (tYmin > tminVal) { tminVal = tYmin; axisEntry = 1; }
	if (tZmin > tminVal) { tminVal = tZmin; axisEntry = 2; }

	int axisExit = 0;
	float tmaxVal = tXmax;
	if (tYmax < tmaxVal) { tmaxVal = tYmax; axisExit = 1; }
	if (tZmax < tmaxVal) { tmaxVal = tZmax; axisExit = 2; }

	if (tmaxVal < tMin || tminVal > tMax || tminVal > tmaxVal)
		return false;

	hitResult.HitMaterial = m_Material;

	if (tminVal < tMin)
	{
		hitResult.HitPoint = r.m_origin + r.m_direction * tmaxVal;
		hitResult.t = tmaxVal;
		Vec4 n;
		if (axisExit == 0) n = Vec4((r.m_direction.x < 0.0f) ? 1.0f : -1.0f, 0.0f, 0.0f, 0.0f);
		else if (axisExit == 1) n = Vec4(0.0f, (r.m_direction.y < 0.0f) ? 1.0f : -1.0f, 0.0f, 0.0f);
		else n = Vec4(0.0f, 0.0f, (r.m_direction.z < 0.0f) ? 1.0f : -1.0f, 0.0f);
		hitResult.HitNormal = n;
	}
	else
	{
		hitResult.HitPoint = r.m_origin + r.m_direction * tminVal;
		hitResult.t = tminVal;
		Vec4 n;
		if (axisEntry == 0) n = Vec4((r.m_direction.x < 0.0f) ? 1.0f : -1.0f, 0.0f, 0.0f, 0.0f);
		else if (axisEntry == 1) n = Vec4(0.0f, (r.m_direction.y < 0.0f) ? 1.0f : -1.0f, 0.0f, 0.0f);
		else n = Vec4(0.0f, 0.0f, (r.m_direction.z < 0.0f) ? 1.0f : -1.0f, 0.0f);
		hitResult.HitNormal = n;
	}
	return true;
}

bool HitableSphere::Hit(const Ray& r, float tMin, float tMax, HitResult& hitResult)
{
	auto oc = r.m_origin - m_Sphere.m_center;
	auto a = r.m_direction * r.m_direction;
	auto half_b = oc * r.m_direction;
	auto c = oc * oc - m_Sphere.m_radius * m_Sphere.m_radius;
	auto dis = half_b * half_b - a * c;

	if (dis > 0)
	{
		hitResult.HitMaterial = m_Material;

		auto root = sqrt(dis);

		auto temp = (-half_b - root) / a;
		if (temp < tMax && temp > tMin)
		{
			hitResult.t = temp;
			hitResult.HitPoint = r.m_origin + r.m_direction * temp;
			auto outward_normal = (hitResult.HitPoint - m_Sphere.m_center) / m_Sphere.m_radius;
			outward_normal = outward_normal.normalize();
			hitResult.setFaceNormal(r, outward_normal);
			return true;
		}

		temp = (-half_b + root) / a;
		if (temp < tMax && temp > tMin)
		{
			hitResult.t = temp;
			hitResult.HitPoint = r.m_origin + r.m_direction * temp;
			auto outward_normal = (hitResult.HitPoint - m_Sphere.m_center) / m_Sphere.m_radius;
			outward_normal = outward_normal.normalize();
			hitResult.setFaceNormal(r, outward_normal);
			return true;
		}
	}

	return false;
}

bool HitableList::Hit(const Ray& r, float tMin, float tMax, HitResult& hitResult)
{
	HitResult l_hitResult;
	bool hit_anything = false;
	float closest_so_far = tMax;
	for (uint32_t i = 0; i < m_Size; i++)
	{
		if (m_List[i]->Hit(r, tMin, closest_so_far, l_hitResult))
		{
			hit_anything = true;
			closest_so_far = l_hitResult.t;
			hitResult = l_hitResult;
		}
	}
	return hit_anything;
}
