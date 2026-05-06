#include "RayTracer_Internal.h"

#include "../Common/LogService.h"

#include "../Services/AssetService.h"
#include "../Services/CameraService.h"
#include "../Services/EntityRegistry.h"

#include "../Component/MeshComponent.h"
#include "../Component/TransformComponent.h"
#include "../Component/MaterialComponent.h"
#include "../Component/CameraComponent.h"
#include "../Component/LightComponent.h"

#include "../Engine.h"

using namespace Inno;
using namespace RayTracerNS;

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

	// Collect directional light (sun) for NEE
	for (auto l_entityID : l_entityIDs)
	{
		auto* l_light = l_registry->Get<LightComponent>(l_entityID);
		auto* l_xf    = l_registry->Get<TransformComponent>(l_entityID);
		if (l_light && l_xf && l_light->m_LightType == LightType::Directional)
		{
			auto l_fwd = Vec4(0.0f, 0.0f, -1.0f, 0.0f).rotateDirectionByQuat(l_xf->m_LocalRot);
			m_sunDir   = Vec4(-l_fwd.x, -l_fwd.y, -l_fwd.z, 0.0f).normalize();
			m_sunColor = l_light->m_RGBColor;
			break;
		}
	}

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
		auto* l_meshResource = AssetService::GetMeshAsset(l_mesh->m_Asset);
		if (!l_meshResource)
			continue;
		auto& l_aabb = l_meshResource->m_AABB;
		if (l_aabb.m_extend.x <= 0.0f && l_aabb.m_extend.y <= 0.0f && l_aabb.m_extend.z <= 0.0f)
			continue;

		auto* l_matComp = l_registry->Get<MaterialComponent>(l_entityID);
		auto* l_matAsset = l_matComp ? AssetService::GetMaterialAsset(l_matComp->m_Asset) : nullptr;

		auto* l_hitable = new HitableCube();
		l_hitable->m_AABB = BuildWorldAABB(l_meshResource->m_AABB, *l_xf);

		float roughness = l_matAsset ? l_matAsset->m_Attributes.Roughness : 0.8f;
		float emissive  = l_matAsset ? (l_matAsset->m_Attributes.AlbedoR +
		                                l_matAsset->m_Attributes.AlbedoG +
		                                l_matAsset->m_Attributes.AlbedoB) / 3.0f : 0.0f;

		if (l_matAsset && emissive > 0.5f && l_matAsset->m_ShaderModel == ShaderModel::Emissive)
		{
			auto* m = new Emissive();
			m->Albedo = Vec4(l_matAsset->m_Attributes.AlbedoR,
			                 l_matAsset->m_Attributes.AlbedoG,
			                 l_matAsset->m_Attributes.AlbedoB, 1.0f);
			l_hitable->m_Material = m;
		}
		else if (roughness > 0.5f)
		{
			auto* m = new Lambertian();
			m->Albedo = Vec4(l_matAsset ? l_matAsset->m_Attributes.AlbedoR : 0.8f,
			                 l_matAsset ? l_matAsset->m_Attributes.AlbedoG : 0.8f,
			                 l_matAsset ? l_matAsset->m_Attributes.AlbedoB : 0.8f, 1.0f);
			m->MRAT.y = roughness;
			l_hitable->m_Material = m;
		}
		else
		{
			auto* m = new Metal();
			m->Albedo = Vec4(l_matAsset ? l_matAsset->m_Attributes.AlbedoR : 0.8f,
			                 l_matAsset ? l_matAsset->m_Attributes.AlbedoG : 0.8f,
			                 l_matAsset ? l_matAsset->m_Attributes.AlbedoB : 0.8f, 1.0f);
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
