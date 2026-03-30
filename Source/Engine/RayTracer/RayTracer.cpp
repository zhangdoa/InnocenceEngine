#include "RayTracer.h"
#include "../Common/Timer.h"
#include "../Common/TaskScheduler.h"
#include "../Common/LogService.h"

#include "../Services/CameraService.h"
#include "../Services/AssetService.h"
#include "../Services/RenderingConfigurationService.h"
#include "../Services/EntityRegistry.h"

#include "../Component/MeshComponent.h"
#include "../Component/TransformComponent.h"
#include "../Component/MaterialComponent.h"
#include "../Component/CameraComponent.h"

#include "../Engine.h"
#include "../Services/IGraphicsService.h"
using namespace Inno;

namespace RayTracerNS
{
	ObjectStatus m_ObjectStatus = ObjectStatus::Terminated;
	std::atomic<bool> m_isWorking;
	Handle<ITask> m_LastTask;
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

	if (tmaxVal < 0.0f || tminVal > tmaxVal)
		return false;

	hitResult.HitMaterial = m_Material;

	if (tminVal < 0.0f)
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

struct HitableSphere : public Hitable
{
	Sphere m_Sphere;
	bool Hit(const Ray& r, float tMin, float tMax, HitResult& hitResult) override;
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
	bool Hit(const Ray& r, float tMin, float tMax, HitResult& hitResult) override;
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
	Vec4 color = Vec4(0.0f, 0.0f, 0.0f, 1.0f);

	if (depth < m_maxDepth)
	{
		if (world->Hit(r, 0.001f, std::numeric_limits<float>::infinity(), l_result))
		{
			Ray scattered;
			Vec4 attenuation;
			if (l_result.HitMaterial->scatter(r, l_result, attenuation, scattered))
			{
				color = attenuation.scale(CalcRadiance(scattered, world, depth + 1));
			}
			else
			{
				color = attenuation;
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

static AABB BuildWorldAABB(const AABB& localAABB, const TransformComponent& xf)
{
	Mat4 l_s = Math::toScaleMatrix(Vec4(xf.m_LocalScale.x, xf.m_LocalScale.y, xf.m_LocalScale.z, 1.0f));
	Mat4 l_r = Math::toRotationMatrix(xf.m_LocalRot);
	Mat4 l_t = Math::toTranslationMatrix(Vec4(xf.m_LocalPos.x, xf.m_LocalPos.y, xf.m_LocalPos.z, 1.0f));
	Mat4 l_trs = l_t * l_r * l_s;
	return Math::TransformAABB(localAABB, l_trs);
}

bool ExecuteRayTracing()
{
	Log(Verbose, "Start ray tracing...");

	auto l_registry = g_Engine->Get<EntityRegistry>();
	auto l_entityIDs = l_registry->GetAllEntityIDs(ObjectLifespan::Scene);

	Vec4 l_lookfrom = Vec4(0.0f, 0.0f, 0.0f, 1.0f);
	Vec4 l_lookat   = Vec4(0.0f, 0.0f, -1.0f, 1.0f);
	CameraComponent* l_camComp = nullptr;

	for (auto l_entityID : l_entityIDs)
	{
		auto* l_cam = l_registry->Get<CameraComponent>(l_entityID);
		auto* l_xf  = l_registry->Get<TransformComponent>(l_entityID);
		if (l_cam && l_xf)
		{
			l_camComp = l_cam;
			l_lookfrom = Vec4(l_xf->m_LocalPos.x, l_xf->m_LocalPos.y, l_xf->m_LocalPos.z, 1.0f);
			auto l_forward = Vec4(0.0f, 0.0f, -1.0f, 0.0f).rotateDirectionByQuat(l_xf->m_LocalRot);
			l_lookat = l_lookfrom + l_forward;
			break;
		}
	}

	auto l_camera = l_camComp ? l_camComp : g_Engine->Get<CameraService>()->GetMainCamera();
	auto l_vfov = l_camera->m_FOVX / l_camera->m_WHRatio;

	auto l_up = Vec4(0.0f, 1.0f, 0.0f, 0.0f);
	RayTracingCamera l_rayTracingCamera(l_lookfrom, l_lookat, l_up, l_vfov, l_camera->m_WHRatio, 1.0f / l_camera->m_Aperture, 1000.0f);

	std::vector<Hitable*> l_hitableListVector;

	for (auto l_entityID : l_entityIDs)
	{
		auto* l_mesh = l_registry->Get<MeshComponent>(l_entityID);
		auto* l_xf   = l_registry->Get<TransformComponent>(l_entityID);
		if (!l_mesh || !l_xf)
			continue;
		if (l_mesh->m_VertexBufferView.m_StrideInBytes == 0)
			continue;

		auto* l_mat = l_registry->Get<MaterialComponent>(l_entityID);

		auto* l_hitable = new HitableCube();
		l_hitable->m_AABB = BuildWorldAABB(l_mesh->m_AABB, *l_xf);

		float roughness = l_mat ? l_mat->m_materialAttributes.Roughness : 0.8f;
		float emissive  = l_mat ? (l_mat->m_materialAttributes.AlbedoR +
		                           l_mat->m_materialAttributes.AlbedoG +
		                           l_mat->m_materialAttributes.AlbedoB) / 3.0f : 0.0f;

		if (l_mat && emissive > 0.5f && l_mat->m_ShaderModel == ShaderModel::Emissive)
		{
			auto* m = new Emissive();
			m->Albedo = Vec4(l_mat->m_materialAttributes.AlbedoR,
			                 l_mat->m_materialAttributes.AlbedoG,
			                 l_mat->m_materialAttributes.AlbedoB, 1.0f);
			l_hitable->m_Material = m;
		}
		else if (roughness > 0.5f)
		{
			auto* m = new Lambertian();
			m->Albedo = Vec4(l_mat ? l_mat->m_materialAttributes.AlbedoR : 0.8f,
			                 l_mat ? l_mat->m_materialAttributes.AlbedoG : 0.8f,
			                 l_mat ? l_mat->m_materialAttributes.AlbedoB : 0.8f, 1.0f);
			m->MRAT.y = roughness;
			l_hitable->m_Material = m;
		}
		else
		{
			auto* m = new Metal();
			m->Albedo = Vec4(l_mat ? l_mat->m_materialAttributes.AlbedoR : 0.8f,
			                 l_mat ? l_mat->m_materialAttributes.AlbedoG : 0.8f,
			                 l_mat ? l_mat->m_materialAttributes.AlbedoB : 0.8f, 1.0f);
			m->MRAT.y = roughness;
			l_hitable->m_Material = m;
		}

		l_hitableListVector.emplace_back(l_hitable);
	}

	if (l_hitableListVector.empty())
	{
		Log(Error, "RayTracer: no renderable mesh entities — writing 1x1 black PNG.");
		uint8_t l_black[4] = {0, 0, 0, 255};
		TextureDesc l_err = {};
		l_err.Width           = 1;
		l_err.Height          = 1;
		l_err.PixelDataType   = TexturePixelDataType::UByte;
		l_err.PixelDataFormat = TexturePixelDataFormat::RGBA;
		l_err.Sampler         = TextureSampler::Sampler2D;
		AssetService::Save("cpu_reference.png", l_err, l_black);
		return false;
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

	TextureDesc l_outDesc = {};
	l_outDesc.Width           = (uint32_t)nx;
	l_outDesc.Height          = (uint32_t)ny;
	l_outDesc.PixelDataType   = TexturePixelDataType::UByte;
	l_outDesc.PixelDataFormat = TexturePixelDataFormat::RGBA;
	l_outDesc.Sampler         = TextureSampler::Sampler2D;

	if (AssetService::Save("cpu_reference.png", l_outDesc, l_result.data()))
		Log(Success, "RayTracer: cpu_reference.png written.");
	else
		Log(Error, "RayTracer: failed to write cpu_reference.png.");

	Log(Success, "Ray tracing finished.");

	return true;
}

bool RayTracer::Setup(IServiceConfig* systemConfig)
{
	RayTracerNS::m_ObjectStatus = ObjectStatus::Created;
	return true;
}

bool RayTracer::Initialize()
{
	const int l_denom = 2;

	auto l_screenResolution = g_Engine->Get<RenderingConfigurationService>()->GetScreenResolution();

	m_TextureComp = g_Engine->getGraphicsService()->AddTextureComponent("RayTracingResult/");

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

		RayTracerNS::m_LastTask = g_Engine->Get<TaskScheduler>()->Submit(ITask::Desc("RayTracingTask", ITask::Type::Once, 4), [&]() { ExecuteRayTracing(); RayTracerNS::m_isWorking = false; });
		RayTracerNS::m_LastTask->Activate();
	}

	return true;
}

bool RayTracer::Terminate()
{
	if (RayTracerNS::m_LastTask)
	{
		// Block until path tracer finishes writing cpu_reference.png.
		// FRAGILITY NOTE: TaskScheduler::Freeze/Reset must not be called before this returns.
		// Engine::Terminate() order: LogicClient::Terminate (reaches here) → TaskScheduler::Reset.
		// If that ordering changes, this Wait() will deadlock.
		RayTracerNS::m_LastTask->Wait();
	}
	RayTracerNS::m_ObjectStatus = ObjectStatus::Terminated;
	return true;
}

ObjectStatus RayTracer::GetStatus()
{
	return RayTracerNS::m_ObjectStatus;
}