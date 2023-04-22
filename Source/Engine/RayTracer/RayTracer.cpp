#include "RayTracer.h"
#include "../Core/Logger.h"

#include "../Interface/IEngine.h"
using namespace Inno;
extern IEngine* g_Engine;

namespace RayTracerNS
{
	ObjectStatus m_ObjectStatus = ObjectStatus::Terminated;
	std::atomic<bool> m_isWorking;
	const int m_maxDepth = 64;
	const int m_maxSamplePerPixel = 8;
	std::default_random_engine m_generator;
	std::uniform_real_distribution<float> m_randomDirDelta(-1.0f, 1.0f);

	TextureComponent* m_TextureComp;
}

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
	virtual bool scatter(const Ray& r, const HitResult& result, Vec4& attenuation, Ray& scattered) const
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
	virtual bool scatter(const Ray& r, const HitResult& result, Vec4& attenuation, Ray& scattered) const
	{
		Vec4 reflected = Reflect(r.m_direction.normalize(), result.HitNormal);
		scattered.m_origin = result.HitPoint;
		scattered.m_direction = reflected;
		attenuation = Albedo;
		return (scattered.m_direction * result.HitNormal > 0);
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
	virtual bool Hit(const Ray& r, float tMin, float tMax, HitResult& hitResult);
};

bool HitableCube::Hit(const Ray& r, float tMin, float tMax, HitResult& hitResult)
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

	hitResult.HitMaterial = m_Material;

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

struct HitableList : public Hitable
{
	uint32_t m_Size;
	Hitable** m_List;
	virtual bool Hit(const Ray& r, float tMin, float tMax, HitResult& hitResult);
};

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
		Vec4 rd = RandomDirectionInUnitDisk() * lens_radius * 0.5 + 0.5;
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
	Vec4 color = Vec4(0.0f, 0.0f, 0.0f, 1.0f);
	static bool l_visualizeNormal = false;

	if (depth < m_maxDepth)
	{
		if (world->Hit(r, 0.001f, std::numeric_limits<float>::infinity(), l_result))
		{
			if (l_visualizeNormal)
			{
				color = l_result.HitNormal;
			}
			else
			{
				Ray scattered;
				Vec4 attenuation;
				if (l_result.HitMaterial->scatter(r, l_result, attenuation, scattered))
				{
					color = attenuation.scale(CalcRadiance(scattered, world, depth + 1));
				}
			}
		}
		else
		{
			Vec4 unitDir = r.m_direction.normalize();
			float t = unitDir.y * 0.5f + 0.5f;
			color = Math::lerp(Vec4(0.5f, 0.7f, 1.0f, 1.0f), Vec4(1.0f, 1.0f, 1.0f, 1.0f), t);
		}
	}

	return color;
}

bool ExecuteRayTracing()
{
	Logger::Log(LogLevel::Verbose, "RayTracer: Start ray tracing...");

	auto l_camera = static_cast<ICameraSystem*>(g_Engine->getComponentManager()->GetComponentSystem<CameraComponent>())->GetMainCamera();
	auto l_cameraTransformComponent = g_Engine->getComponentManager()->Find<TransformComponent>(l_camera->m_Owner);
	auto l_lookfrom = l_cameraTransformComponent->m_globalTransformVector.m_pos;
	auto l_lookat = l_lookfrom + Math::getDirection(Direction::Backward, l_cameraTransformComponent->m_globalTransformVector.m_rot);
	auto l_up = Math::getDirection(Direction::Up, l_cameraTransformComponent->m_globalTransformVector.m_rot);
	auto l_vfov = l_camera->m_FOVX / l_camera->m_WHRatio;

	RayTracingCamera l_rayTracingCamera(l_lookfrom, l_lookat, l_up, l_vfov, l_camera->m_WHRatio, 1.0f / l_camera->m_aperture, 1000.0f);

	auto l_visibleComponents = g_Engine->getComponentManager()->GetAll<VisibleComponent>();

	std::vector<Hitable*> l_hitableListVector;
	l_hitableListVector.reserve(l_visibleComponents.size());

	for (auto l_visibleComponent : l_visibleComponents)
	{
		if (l_visibleComponent->m_proceduralMeshShape == ProceduralMeshShape::Sphere)
		{
			auto l_transformComponent = g_Engine->getComponentManager()->Find<TransformComponent>(l_camera->m_Owner);
			for (uint64_t j = 0; j < l_visibleComponent->m_model->meshMaterialPairs.m_count; j++)
			{
				auto l_pair = g_Engine->getAssetSystem()->getMeshMaterialPair(l_visibleComponent->m_model->meshMaterialPairs.m_startOffset + j);
				if (l_pair->material->m_ShaderModel == ShaderModel::Opaque)
				{
					auto l_hitable = new HitableSphere();
					l_hitable->m_Material = new Lambertian();
					l_hitable->m_Material->Albedo.x = l_pair->material->m_materialAttributes.AlbedoR;
					l_hitable->m_Material->Albedo.y = l_pair->material->m_materialAttributes.AlbedoG;
					l_hitable->m_Material->Albedo.z = l_pair->material->m_materialAttributes.AlbedoB;
					l_hitable->m_Material->MRAT.x = l_pair->material->m_materialAttributes.Metallic;
					l_hitable->m_Material->MRAT.y = l_pair->material->m_materialAttributes.Roughness;

					l_hitable->m_Sphere.m_center = l_transformComponent->m_globalTransformVector.m_pos.xyz();
					l_hitable->m_Sphere.m_radius = 1.0f;

					l_hitableListVector.emplace_back(l_hitable);
					break;
				}
			}
		}
	}

	auto l_hitable = new HitableSphere();
	l_hitable->m_Material = new Lambertian();
	l_hitable->m_Material->Albedo.x = 1.0f;
	l_hitable->m_Material->Albedo.y = 1.0f;
	l_hitable->m_Material->Albedo.z = 1.0f;
	l_hitable->m_Material->MRAT.x = 0.0f;
	l_hitable->m_Material->MRAT.y = 1.0f;

	l_hitable->m_Sphere.m_center = Vec3(0.0f, -200.0f, 0.0f);
	l_hitable->m_Sphere.m_radius = 200.0f;

	l_hitableListVector.emplace_back(l_hitable);

	for (size_t i = 0; i < 32; i++)
	{
		l_hitable = new HitableSphere();

		if (m_randomDirDelta(m_generator) > 0.0f)
		{
			l_hitable->m_Material = new Metal();
		}
		else
		{
			l_hitable->m_Material = new Lambertian();
		}

		l_hitable->m_Material->Albedo.x = m_randomDirDelta(m_generator) * 0.5f + 0.5f;
		l_hitable->m_Material->Albedo.y = m_randomDirDelta(m_generator) * 0.5f + 0.5f;
		l_hitable->m_Material->Albedo.z = m_randomDirDelta(m_generator) * 0.5f + 0.5f;
		l_hitable->m_Material->MRAT.x = 0.0f;
		l_hitable->m_Material->MRAT.y = 1.0f;

		l_hitable->m_Sphere.m_radius = (m_randomDirDelta(m_generator) + 1.5f);
		l_hitable->m_Sphere.m_center = Vec3(m_randomDirDelta(m_generator) * 10.0f, l_hitable->m_Sphere.m_radius, m_randomDirDelta(m_generator) * 10.0f);

		l_hitableListVector.emplace_back(l_hitable);
	}

	HitableList* l_hitableList = new HitableList();
	l_hitableList->m_List = l_hitableListVector.data();
	l_hitableList->m_Size = (uint32_t)l_hitableListVector.size();

	int32_t nx = m_TextureComp->m_TextureDesc.Width;
	int32_t ny = m_TextureComp->m_TextureDesc.Height;
	int32_t totalWorkload = nx * ny;

	std::vector<TVec4<uint8_t>> l_result;
	l_result.reserve(totalWorkload);

	for (int32_t j = ny - 1; j >= 0; j--)
	{
		for (int32_t i = 0; i < nx; i++)
		{
			float u = float(i) / float(nx);
			float v = float(j) / float(ny);

			Vec4 l_totalColor = Vec4();
			for (int32_t k = 0; k < m_maxSamplePerPixel; k++)
			{
				auto l_singleSampleColor = CalcRadiance(l_rayTracingCamera.GetRay(u, v), l_hitableList, 0);
				l_singleSampleColor = l_singleSampleColor / (float)m_maxSamplePerPixel;

				l_totalColor = l_totalColor + l_singleSampleColor;
			}

			l_totalColor.x = sqrtf(l_totalColor.x);
			l_totalColor.y = sqrtf(l_totalColor.y);
			l_totalColor.z = sqrtf(l_totalColor.z);

			TVec4<uint8_t> l_colorUint8;
			l_colorUint8.x = uint8_t(255.99 * l_totalColor.x);
			l_colorUint8.y = uint8_t(255.99 * l_totalColor.y);
			l_colorUint8.z = uint8_t(255.99 * l_totalColor.z);
			l_colorUint8.w = uint8_t(255);
			l_result.emplace_back(l_colorUint8);
		}
	}

	m_TextureComp->m_TextureData = &l_result[0];

	auto l_textureFileName = "..//Res//Intermediate//RayTracingResult_" + std::to_string(g_Engine->getTimeSystem()->getCurrentTimeFromEpoch());
	g_Engine->getAssetSystem()->saveTexture(l_textureFileName.c_str(), m_TextureComp);

	Logger::Log(LogLevel::Success, "RayTracer: Ray tracing finished.");

	return true;
}

bool RayTracer::Setup(ISystemConfig* systemConfig)
{
	RayTracerNS::m_ObjectStatus = ObjectStatus::Created;
	return true;
}

bool RayTracer::Initialize()
{
	const int l_denom = 2;

	auto l_screenResolution = g_Engine->getRenderingFrontend()->GetScreenResolution();

	m_TextureComp = g_Engine->getRenderingServer()->AddTextureComponent("RayTracingResult/");

	m_TextureComp->m_TextureDesc.Sampler = TextureSampler::Sampler2D;
	m_TextureComp->m_TextureDesc.Usage = TextureUsage::Sample;
	m_TextureComp->m_TextureDesc.PixelDataFormat = TexturePixelDataFormat::RGBA;
	m_TextureComp->m_TextureDesc.Width = l_screenResolution.x / l_denom;
	m_TextureComp->m_TextureDesc.Height = l_screenResolution.y / l_denom;
	m_TextureComp->m_TextureDesc.PixelDataType = TexturePixelDataType::UByte;

	RayTracerNS::m_ObjectStatus = ObjectStatus::Activated;
	return true;
}

bool RayTracer::Execute()
{
	if (!RayTracerNS::m_isWorking)
	{
		RayTracerNS::m_isWorking = true;

		auto l_rayTracingTask = g_Engine->getTaskSystem()->Submit("RayTracingTask", 4, nullptr, [&]() { ExecuteRayTracing(); RayTracerNS::m_isWorking = false; });
	}

	return true;
}

bool RayTracer::Terminate()
{
	RayTracerNS::m_ObjectStatus = ObjectStatus::Terminated;
	return true;
}

ObjectStatus RayTracer::GetStatus()
{
	return RayTracerNS::m_ObjectStatus;
}