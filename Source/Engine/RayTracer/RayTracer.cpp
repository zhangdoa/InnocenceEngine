#include "RayTracer.h"
#include "../Core/InnoLogger.h"
#include "../Common/CommonMacro.inl"
#include "../ComponentManager/ITransformComponentManager.h"
#include "../ComponentManager/IVisibleComponentManager.h"
#include "../ComponentManager/ICameraComponentManager.h"

#include "../Interface/IModuleManager.h"
extern IModuleManager* g_pModuleManager;

namespace InnoRayTracerNS
{
	ObjectStatus m_ObjectStatus = ObjectStatus::Terminated;
	std::atomic<bool> m_isWorking;
	const int m_maxDepth = 4;
	TextureDataComponent* m_TDC;
}

using namespace InnoRayTracerNS;

struct HitResult
{
	Vec4 HitPoint;
	Vec4 HitNormal;
	Vec4 Albedo;
	float t;
};

struct Hitable
{
	VisibleComponent* m_VisibleComponent;
	virtual bool Hit(const Ray& r, float tMin, float tMax, HitResult& hitResult) = 0;
};

struct HitableCube : public Hitable
{
	AABB m_AABB;
	virtual bool Hit(const Ray& r, float tMin, float tMax, HitResult& hitResult);
};

bool HitableCube::Hit(const Ray & r, float tMin, float tMax, HitResult & hitResult)
{
	float t1 = (m_AABB.m_boundMin.x - r.m_origin.x) / r.m_direction.x;
	float t2 = (m_AABB.m_boundMax.x - r.m_origin.x) / r.m_direction.x;
	float t3 = (m_AABB.m_boundMin.y - r.m_origin.y) / r.m_direction.y;
	float t4 = (m_AABB.m_boundMax.y - r.m_origin.y) / r.m_direction.y;
	float t5 = (m_AABB.m_boundMin.z - r.m_origin.z) / r.m_direction.z;
	float t6 = (m_AABB.m_boundMax.z - r.m_origin.z) / r.m_direction.z;

	float tmin = std::max(std::max(std::min(t1, t2), std::min(t3, t4)), std::min(t5, t6));
	float tmax = std::min(std::min(std::max(t1, t2), std::max(t3, t4)), std::max(t5, t6));

	if (tmax < 0.0f)
	{
		return false;
	}

	if (tmin > tmax)
	{
		return false;
	}

	for (uint64_t j = 0; j < m_VisibleComponent->m_model->meshMaterialPairs.m_count; j++)
	{
		auto l_pair = g_pModuleManager->getAssetSystem()->getMeshMaterialPair(m_VisibleComponent->m_model->meshMaterialPairs.m_startOffset + j);
		hitResult.Albedo.x = l_pair->material->m_materialAttributes.AlbedoR;
		hitResult.Albedo.y = l_pair->material->m_materialAttributes.AlbedoG;
		hitResult.Albedo.z = l_pair->material->m_materialAttributes.AlbedoB;
		break;
	}

	if (tmin < 0.0f)
	{
		hitResult.HitPoint = r.m_origin + r.m_direction * tmax;
		hitResult.HitNormal = hitResult.HitPoint - m_AABB.m_center;
		return true;
	}

	hitResult.HitPoint = r.m_origin + r.m_direction * tmin;
	hitResult.HitNormal = hitResult.HitPoint - m_AABB.m_center;
	return true;
}

struct HitableSphere : public Hitable
{
	Sphere m_Sphere;
	virtual bool Hit(const Ray& r, float tMin, float tMax, HitResult& hitResult);
};

bool HitableSphere::Hit(const Ray & r, float tMin, float tMax, HitResult & hitResult)
{
	auto oc = r.m_origin - m_Sphere.m_center;
	auto a = r.m_direction * r.m_direction;
	auto b = oc * r.m_direction * 2.0f;
	auto c = oc * oc - m_Sphere.m_radius * m_Sphere.m_radius;
	auto dis = b * b - 4 * a * c;

	if (dis > 0)
	{
		for (uint64_t j = 0; j < m_VisibleComponent->m_model->meshMaterialPairs.m_count; j++)
		{
			auto l_pair = g_pModuleManager->getAssetSystem()->getMeshMaterialPair(m_VisibleComponent->m_model->meshMaterialPairs.m_startOffset + j);
			hitResult.Albedo.x = l_pair->material->m_materialAttributes.AlbedoR;
			hitResult.Albedo.y = l_pair->material->m_materialAttributes.AlbedoG;
			hitResult.Albedo.z = l_pair->material->m_materialAttributes.AlbedoB;
			break;
		}

		float temp = (-b - sqrt(dis)) / a;
		if (temp < tMax && temp > tMin)
		{
			hitResult.t = temp;
			hitResult.HitPoint = r.m_origin + r.m_direction * temp;
			hitResult.HitNormal = (hitResult.HitPoint - m_Sphere.m_center) / m_Sphere.m_radius;
			hitResult.HitNormal = hitResult.HitNormal.normalize();
			return true;
		}
		temp = (-b + sqrt(dis)) / a;
		if (temp < tMax && temp > tMin)
		{
			hitResult.t = temp;
			hitResult.HitPoint = r.m_origin + r.m_direction * temp;
			hitResult.HitNormal = (hitResult.HitPoint - m_Sphere.m_center) / m_Sphere.m_radius;
			hitResult.HitNormal = hitResult.HitNormal.normalize();
			return true;
		}
	}

	return false;
}

struct HitableList : public Hitable
{
	uint32_t m_Size;
	Hitable** m_List;
	virtual bool Hit(const Ray& r, float tMin, float tMax, HitResult& hitResult);
};

bool HitableList::Hit(const Ray & r, float tMin, float tMax, HitResult & hitResult)
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

Vec4 RandomDirectionInUnitDisk()
{
	std::default_random_engine l_generator;
	std::uniform_real_distribution<float> l_randomDirDelta(0.0f, 1.0f);

	Vec4 p;
	do {
		p = Vec4(l_randomDirDelta(l_generator), l_randomDirDelta(l_generator), 0.0f, 0.0f) * 2.0f - Vec4(1.0f, 1.0f, 0.0f, 0.0f);
	} while (p * p >= 1.0f);
	return p;
}

Vec4 RandomDirectionInUnitSphere()
{
	std::default_random_engine l_generator;
	std::uniform_real_distribution<float> l_randomDirDelta(0.0f, 1.0f);

	Vec4 p;
	do {
		p = Vec4(l_randomDirDelta(l_generator), l_randomDirDelta(l_generator), l_randomDirDelta(l_generator), 0.0f) * 2.0f - Vec4(1.0f, 1.0f, 1.0f, 0.0f);
	} while (p * p >= 1.0f);
	return p;
}

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

Vec4 CalcRadiance(const Ray& r, Hitable* world, int32_t depth)
{
	HitResult l_result;
	Vec4 color;

	if (depth < m_maxDepth)
	{
		if (world->Hit(r, 0.0f, std::numeric_limits<float>::max(), l_result))
		{
			Ray l_ray;
			l_ray.m_origin = l_result.HitPoint;
			l_ray.m_direction = l_result.HitNormal + RandomDirectionInUnitSphere();
			color = l_result.Albedo;

			auto l_incoming_color = CalcRadiance(l_ray, world, depth + 1) * 0.5f;
			color = color + l_incoming_color;
		}
		else
		{
			Vec4 unitDir = r.m_direction.normalize();
			float t = unitDir.y * 0.5f + 0.5f;
			color = InnoMath::lerp(Vec4(0.5f, 0.7f, 1.0f, 1.0f), Vec4(1.0f, 1.0f, 1.0f, 1.0f), t);
		}
	}

	color.w = 1.0f;

	return color;
}

bool ExecuteRayTracing()
{
	InnoLogger::Log(LogLevel::Verbose, "InnoRayTracer: Start ray tracing...");

	auto l_camera = GetComponentManager(CameraComponent)->GetAllComponents()[0];
	auto l_cameraTransformComponent = GetComponent(TransformComponent, l_camera->m_ParentEntity);
	auto l_lookfrom = l_cameraTransformComponent->m_globalTransformVector.m_pos;
	auto l_lookat = l_lookfrom + InnoMath::getDirection(Direction::Backward, l_cameraTransformComponent->m_globalTransformVector.m_rot);
	auto l_up = InnoMath::getDirection(Direction::Up, l_cameraTransformComponent->m_globalTransformVector.m_rot);
	auto l_vfov = l_camera->m_FOVX / l_camera->m_WHRatio;

	RayTracingCamera l_rayTracingCamera(l_lookfrom, l_lookat, l_up, l_vfov, l_camera->m_WHRatio, 0.1f, 1000.0f);

	auto l_visibleComponents = GetComponentManager(VisibleComponent)->GetAllComponents();

	std::vector<Hitable*> l_hitableListVector;
	l_hitableListVector.reserve(l_visibleComponents.size());

	for (auto l_visibleComponent : l_visibleComponents)
	{
		auto l_transformComponent = GetComponent(TransformComponent, l_visibleComponent->m_ParentEntity);
		if (l_visibleComponent->m_meshShapeType == MeshShapeType::Cube)
		{
			auto l_hitable = new HitableCube();
			l_hitable->m_VisibleComponent = l_visibleComponent;
			l_hitable->m_AABB.m_center = l_transformComponent->m_globalTransformVector.m_pos;
			l_hitable->m_AABB.m_extend = l_transformComponent->m_globalTransformVector.m_scale;
			l_hitable->m_AABB.m_extend.w = 0.0f;
			l_hitable->m_AABB.m_boundMax = l_hitable->m_AABB.m_center + l_hitable->m_AABB.m_extend * 0.5f;
			l_hitable->m_AABB.m_boundMin = l_hitable->m_AABB.m_center - l_hitable->m_AABB.m_extend * 0.5f;

			l_hitableListVector.emplace_back(l_hitable);
		}
		else if (l_visibleComponent->m_meshShapeType == MeshShapeType::Sphere)
		{
			auto l_hitable = new HitableSphere();
			l_hitable->m_VisibleComponent = l_visibleComponent;
			l_hitable->m_Sphere.m_center = l_transformComponent->m_globalTransformVector.m_pos;
			l_hitable->m_Sphere.m_radius = 1.0f;

			l_hitableListVector.emplace_back(l_hitable);
		}
	}

	HitableList* l_hitableList = new HitableList();
	l_hitableList->m_List = l_hitableListVector.data();
	l_hitableList->m_Size = (uint32_t)l_hitableListVector.size();

	int32_t nx = m_TDC->m_TextureDesc.Width;
	int32_t ny = m_TDC->m_TextureDesc.Height;
	int32_t totalWorkload = nx * ny;

	std::vector<TVec4<uint8_t>> l_result;
	l_result.reserve(totalWorkload);

	for (int32_t j = ny - 1; j >= 0; j--)
	{
		for (int32_t i = 0; i < nx; i++)
		{
			float u = float(i) / float(nx);
			float v = float(j) / float(ny);
			Vec4 color = CalcRadiance(l_rayTracingCamera.GetRay(u, v), l_hitableList, 0);
			color.x = sqrtf(color.x);
			color.y = sqrtf(color.y);
			color.z = sqrtf(color.z);

			TVec4<uint8_t> l_colorUint8;
			l_colorUint8.x = uint8_t(255.99*color.x);
			l_colorUint8.y = uint8_t(255.99*color.y);
			l_colorUint8.z = uint8_t(255.99*color.z);
			l_colorUint8.w = uint8_t(255);

			l_result.emplace_back(l_colorUint8);
		}
	}

	m_TDC->m_TextureData = &l_result[0];

	auto l_textureFileName = "..//Res//Intermediate//RayTracingResult_" + std::to_string(g_pModuleManager->getTimeSystem()->getCurrentTimeFromEpoch());
	g_pModuleManager->getFileSystem()->saveTexture(l_textureFileName.c_str(), m_TDC);

	InnoLogger::Log(LogLevel::Success, "InnoRayTracer: Ray tracing finished.");

	return true;
}

bool InnoRayTracer::Setup()
{
	InnoRayTracerNS::m_ObjectStatus = ObjectStatus::Created;
	return true;
}

bool InnoRayTracer::Initialize()
{
	const int l_denom = 2;

	auto l_screenResolution = g_pModuleManager->getRenderingFrontend()->getScreenResolution();

	m_TDC = g_pModuleManager->getRenderingServer()->AddTextureDataComponent("RayTracingResult/");

	m_TDC->m_TextureDesc.Sampler = TextureSampler::Sampler2D;
	m_TDC->m_TextureDesc.Usage = TextureUsage::Sample;
	m_TDC->m_TextureDesc.PixelDataFormat = TexturePixelDataFormat::RGBA;
	m_TDC->m_TextureDesc.Width = l_screenResolution.x / l_denom;
	m_TDC->m_TextureDesc.Height = l_screenResolution.y / l_denom;
	m_TDC->m_TextureDesc.PixelDataType = TexturePixelDataType::UByte;

	InnoRayTracerNS::m_ObjectStatus = ObjectStatus::Activated;
	return true;
}

bool InnoRayTracer::Execute()
{
	if (!InnoRayTracerNS::m_isWorking)
	{
		InnoRayTracerNS::m_isWorking = true;

		auto l_rayTracingTask = g_pModuleManager->getTaskSystem()->submit("RayTracingTask", 4, nullptr, [&]() { ExecuteRayTracing(); InnoRayTracerNS::m_isWorking = false; });
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
	return InnoRayTracerNS::m_ObjectStatus;
}