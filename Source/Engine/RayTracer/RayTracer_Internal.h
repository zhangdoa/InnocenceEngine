#pragma once
#include "RayTracer.h"

#include "../Common/STL14.h"
#include "../Common/MathHelper.h"
#include "../Common/TaskScheduler.h"

namespace Inno
{
	class TextureComponent;
}

namespace RayTracerNS
{
	using namespace Inno;

	extern ObjectStatus m_ObjectStatus;
	extern std::atomic<bool> m_isWorking;
	extern std::shared_ptr<ITask> m_LastTask;
	extern const int m_maxDepth;
	extern const int m_maxSamplePerPixel;
	extern std::default_random_engine m_generator;
	extern std::uniform_real_distribution<float> m_randomDirDelta;

	extern TextureComponent* m_TextureComp;
	extern uint32_t m_outputWidth;
	extern uint32_t m_outputHeight;
	extern uint32_t m_explicitWidth;
	extern uint32_t m_explicitHeight;
	extern uint32_t m_downsampleDenominator;

	extern Vec4 m_sunDir;
	extern Vec4 m_sunColor;
}

// --- RNG / vector helpers (defined in RayTracer_Random.cpp) ---
Vec4 RandomDirectionInUnitDisk();
Vec4 RandomDirectionInUnitSphere();
Vec4 RandomUnitVector();
Vec4 Reflect(const Vec4& v, const Vec4& n);

// --- Shading (defined in RayTracer_Shading.cpp) ---
struct Hitable;
Vec4 SkyColor(const Ray& r);
Vec4 CalcRadiance(const Ray& r, Hitable* world, int32_t depth);

// --- Scene driver (defined in RayTracer_Scene.cpp) ---
bool ExecuteRayTracing();

// --- Domain types ---

struct Material;

struct HitResult
{
	Vec4 HitPoint;
	Vec4 HitNormal;
	Material* HitMaterial;
	float t;
	bool FrontFace;

	inline void setFaceNormal(const Ray& r, const Vec4& outward_normal)
	{
		FrontFace = (r.m_direction * outward_normal) < 0;
		HitNormal = FrontFace ? outward_normal : outward_normal * -1.0f;
	}
};

struct Material
{
	Vec4 Albedo;
	Vec4 MRAT;

	virtual bool scatter(const Ray& r, const HitResult& result, Vec4& attenuation, Ray& scattered) const = 0;
};

struct Lambertian : public Material
{
	bool scatter(const Ray& r, const HitResult& result, Vec4& attenuation, Ray& scattered) const override
	{
		Vec4 scatterDir = result.HitNormal + RandomUnitVector();
		scattered.m_origin = result.HitPoint;
		scattered.m_direction = scatterDir;
		attenuation = Albedo;
		return true;
	}
};

struct Metal : public Material
{
	bool scatter(const Ray& r, const HitResult& result, Vec4& attenuation, Ray& scattered) const override
	{
		Vec4 reflected = Reflect(r.m_direction.normalize(), result.HitNormal);
		scattered.m_origin = result.HitPoint;
		scattered.m_direction = reflected;
		attenuation = Albedo;
		return (scattered.m_direction * result.HitNormal > 0);
	}
};

struct Emissive : public Material
{
	bool scatter(const Ray& r, const HitResult& result, Vec4& attenuation, Ray& scattered) const override
	{
		attenuation = Albedo;
		return false;
	}
};

struct Hitable
{
	Material* m_Material;
	virtual bool Hit(const Ray& r, float tMin, float tMax, HitResult& hitResult) = 0;
};

struct HitableCube : public Hitable
{
	AABB m_AABB;
	bool Hit(const Ray& r, float tMin, float tMax, HitResult& hitResult) override;
};

struct HitableSphere : public Hitable
{
	Sphere m_Sphere;
	bool Hit(const Ray& r, float tMin, float tMax, HitResult& hitResult) override;
};

struct HitableList : public Hitable
{
	uint32_t m_Size;
	Hitable** m_List;
	bool Hit(const Ray& r, float tMin, float tMax, HitResult& hitResult) override;
};

class RayTracingCamera
{
public:
	RayTracingCamera(Vec4 lookfrom, Vec4 lookat, Vec4 vup, float vfov, float aspect, float aperture, float focus_dist)
	{
		lens_radius = aperture / 2;
		float theta = vfov * PI<float> / 180.0f;
		float half_height = tan(theta / 2);
		float half_width = aspect * half_height;
		origin = lookfrom;
		w = (lookfrom - lookat).normalize();
		u = (vup.cross(w)).normalize();
		v = (w.cross(u)).normalize();
		lower_left_corner = origin - u * half_width * focus_dist - v * half_height * focus_dist - w * focus_dist;
		horizontal = u * 2 * half_width * focus_dist;
		vertical = v * 2 * half_height * focus_dist;
	}

	Ray GetRay(float s, float t)
	{
		Vec4 rd = RandomDirectionInUnitDisk() * lens_radius;
		Vec4 offset = u * rd.x + v * rd.y;

		Ray l_result;
		l_result.m_origin = origin + offset;
		l_result.m_direction = lower_left_corner + horizontal * s + vertical * t - origin - offset;
		l_result.m_direction = l_result.m_direction.normalize();
		return l_result;
	}

	Vec4 origin;
	Vec4 lower_left_corner;
	Vec4 horizontal;
	Vec4 vertical;
	Vec4 u, v, w;
	float lens_radius;
};
