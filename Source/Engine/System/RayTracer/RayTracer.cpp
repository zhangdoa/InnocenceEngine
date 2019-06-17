#include "RayTracer.h"

#include "../ICoreSystem.h"
extern ICoreSystem* g_pCoreSystem;

namespace InnoRayTracerNS
{
	std::function<void()> f_test;
	ObjectStatus m_ObjectStatus = ObjectStatus::Terminated;
	std::atomic<bool> m_isWorking;
}

bool InnoRayTracer::Setup()
{
	InnoRayTracerNS::f_test = [&]() { g_pCoreSystem->getTaskSystem()->submit("RayTracingTask", [&]() {Execute(); });  };
	g_pCoreSystem->getInputSystem()->addButtonStatusCallback(ButtonData{ INNO_KEY_H, ButtonStatus::PRESSED }, &InnoRayTracerNS::f_test);

	InnoRayTracerNS::m_ObjectStatus = ObjectStatus::Created;
	return true;
}

bool InnoRayTracer::Initialize()
{
	InnoRayTracerNS::m_ObjectStatus = ObjectStatus::Activated;
	return true;
}

bool InnoRayTracer::Execute()
{
	if (!InnoRayTracerNS::m_isWorking)
	{
		InnoRayTracerNS::m_isWorking = true;

		std::ofstream l_ExportFile(g_pCoreSystem->getFileSystem()->getWorkingDirectory() + "//Res//test.ppm", std::ios::out | std::ios::trunc);

		auto l_ScreenResolution = g_pCoreSystem->getRenderingFrontend()->getScreenResolution();

		int nx = l_ScreenResolution.x;
		int ny = l_ScreenResolution.y;

		l_ExportFile << "P3\n" << nx << " " << ny << "\n255\n";

		for (int j = ny - 1; j >= 0; j--) {
			for (int i = 0; i < nx; i++) {
				float r = float(i) / float(nx);
				float g = float(j) / float(ny);
				float b = 0.2f;
				int ir = int(255.99*r);
				int ig = int(255.99*g);
				int ib = int(255.99*b);
				l_ExportFile << ir << " " << ig << " " << ib << "\n";
			}
		}

		l_ExportFile.close();

		InnoRayTracerNS::m_isWorking = false;
	}

	return true;
}

bool InnoRayTracer::Terminate()
{
	InnoRayTracerNS::m_ObjectStatus = ObjectStatus::Terminated;
	return true;
}

ObjectStatus InnoRayTracer::GetStatus()
{
	return ObjectStatus();
}

struct HitResult
{
	vec4 HitPoint;
	vec4 HitNormal;
	float t;
};

struct Hitable
{
	VisibleComponent* m_VisibleComponent;
	virtual bool Hit(const Ray& r, float tMin, float tMax, HitResult& hitResult) = 0;
};

struct HitableSphere : public Hitable
{
	Sphere m_Sphere;
	virtual bool Hit(const Ray& r, float tMin, float tMax, HitResult& hitResult);
};

bool HitableSphere::Hit(const Ray & r, float tMin, float tMax, HitResult & hitResult)
{
	auto l_transformComponent = g_pCoreSystem->getGameSystem()->get<TransformComponent>(m_VisibleComponent->m_parentEntity);

	m_Sphere.m_center = l_transformComponent->m_globalTransformVector.m_pos;
	m_Sphere.m_radius = 1.0f;

	auto oc = r.m_origin - m_Sphere.m_center;
	auto a = r.m_direction * r.m_direction;
	auto b = oc * r.m_direction;
	auto c = oc * oc - m_Sphere.m_radius * m_Sphere.m_radius;
	auto dis = b * b - 4 * a * c;

	if (dis > 0) {
		float temp = (-b - sqrt(dis)) / a;
		if (temp < tMax && temp > tMin)
		{
			hitResult.t = temp;
			hitResult.HitPoint = r.m_origin + r.m_direction * temp;
			hitResult.HitNormal = (hitResult.HitPoint - m_Sphere.m_center) / m_Sphere.m_radius;
			return true;
		}
		temp = (-b + sqrt(dis)) / a;
		if (temp < tMax && temp > tMin)
		{
			hitResult.t = temp;
			hitResult.HitPoint = r.m_origin + r.m_direction * temp;
			hitResult.HitNormal = (hitResult.HitPoint - m_Sphere.m_center) / m_Sphere.m_radius;
			return true;
		}
	}

	return false;
}

struct HitableList : public Hitable
{
	unsigned int m_Size;
	Hitable** m_List;
	virtual bool Hit(const Ray& r, float tMin, float tMax, HitResult& hitResult) const;
};

bool HitableList::Hit(const Ray & r, float tMin, float tMax, HitResult & hitResult) const
{
	HitResult l_hitResult;
	bool hit_anything = false;
	float closest_so_far = std::numeric_limits<float>::max();
	for (unsigned int i = 0; i < m_Size; i++)
	{
		if (m_List[i]->Hit(r, 0.001f, closest_so_far, l_hitResult))
		{
			hit_anything = true;
			closest_so_far = l_hitResult.t;
			hitResult = l_hitResult;
		}
	}
	return hit_anything;
}

vec4 CalcRadiance(const Ray& r, VisibleComponent* rhs)
{
	return vec4();
}