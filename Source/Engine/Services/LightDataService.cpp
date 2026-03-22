#include "LightDataService.h"

#include "../Common/LogService.h"
#include "../Common/MathHelper.h"
#include "../Common/GPUDataStructure.h"
#include "EntityRegistry.h"
#include "CameraService.h"
#include "RenderingConfigurationService.h"
#include "../Component/LightComponent.h"
#include "../Component/TransformComponent.h"
#include "../Component/CameraComponent.h"
#include "../Engine.h"

using namespace Inno;

namespace
{
	AABB SnapAABBToShadowMap(const AABB& Rhs, float ShadowMapResolution)
	{
		Vec4 l_UnitsPerTexel = Rhs.m_extend / ShadowMapResolution;
		Vec4 l_TexelPerUnit  = l_UnitsPerTexel.reciprocal();

		Vec4 l_SnappedCenter = Rhs.m_center.scale(l_TexelPerUnit) + 0.5f;
		l_SnappedCenter = Vec4(floor(l_SnappedCenter.x), floor(l_SnappedCenter.y), floor(l_SnappedCenter.z), 1.0f);
		l_SnappedCenter = l_SnappedCenter.scale(l_UnitsPerTexel);

		AABB l_Result;
		l_Result.m_center   = l_SnappedCenter;
		l_Result.m_extend   = Rhs.m_extend;
		l_Result.m_boundMin = l_Result.m_center - l_Result.m_extend * 0.5f;
		l_Result.m_boundMax = l_Result.m_center + l_Result.m_extend * 0.5f;
		return l_Result;
	}

	void AlignMatrixToTexels(Mat4& Matrix, float ShadowMapResolution)
	{
		Matrix.m30 = floor(Matrix.m30 * ShadowMapResolution) / ShadowMapResolution;
		Matrix.m31 = floor(Matrix.m31 * ShadowMapResolution) / ShadowMapResolution;
	}
}

namespace Inno
{
	struct LightDataServiceImpl
	{
		ObjectStatus m_ObjectStatus = ObjectStatus::Terminated;

		std::vector<PointLightConstantBuffer>  m_PointLightCBVector;
		std::vector<SphereLightConstantBuffer> m_SphereLightCBVector;
		std::vector<CSMConstantBuffer>         m_CSMCBVector;

		GPUBufferComponent* m_PointLightGPUBufferComp   = nullptr;
		GPUBufferComponent* m_SphereLightGPUBufferComp  = nullptr;
		GPUBufferComponent* m_CSMGPUBufferComp          = nullptr;
		GPUBufferComponent* m_GICBufferGPUBufferComp    = nullptr;

		bool Setup(IServiceConfig* systemConfig);
		bool Initialize();
		bool Update();
		bool Terminate();

		bool UpdateLightData();
		bool UpdateCSMData();
	};
}

bool LightDataServiceImpl::Setup(IServiceConfig* systemConfig)
{
	auto l_renderingServer = g_Engine->getRenderingServer();

	m_PointLightGPUBufferComp = l_renderingServer->AddGPUBufferComponent("PointLightCBuffer/");
	m_SphereLightGPUBufferComp = l_renderingServer->AddGPUBufferComponent("SphereLightCBuffer/");
	m_CSMGPUBufferComp = l_renderingServer->AddGPUBufferComponent("CSMCBuffer/");
	m_GICBufferGPUBufferComp = l_renderingServer->AddGPUBufferComponent("GICBuffer/");

	m_ObjectStatus = ObjectStatus::Created;
	return true;
}

bool LightDataServiceImpl::Initialize()
{
	if (m_ObjectStatus == ObjectStatus::Created)
	{
		auto l_renderingServer = g_Engine->getRenderingServer();
		auto l_RenderingCapability = g_Engine->Get<RenderingConfigurationService>()->GetRenderingCapability();

		m_PointLightGPUBufferComp->m_ElementCount = l_RenderingCapability.maxPointLights;
		m_PointLightGPUBufferComp->m_ElementSize = sizeof(PointLightConstantBuffer);

		l_renderingServer->Initialize(m_PointLightGPUBufferComp);

		m_SphereLightGPUBufferComp->m_ElementCount = l_RenderingCapability.maxSphereLights;
		m_SphereLightGPUBufferComp->m_ElementSize = sizeof(SphereLightConstantBuffer);

		l_renderingServer->Initialize(m_SphereLightGPUBufferComp);

		m_CSMGPUBufferComp->m_ElementCount = l_RenderingCapability.maxCSMSplits;
		m_CSMGPUBufferComp->m_ElementSize = sizeof(CSMConstantBuffer);

		l_renderingServer->Initialize(m_CSMGPUBufferComp);

		m_GICBufferGPUBufferComp->m_ElementSize = sizeof(GIConstantBuffer);
		m_GICBufferGPUBufferComp->m_ElementCount = 1;

		l_renderingServer->Initialize(m_GICBufferGPUBufferComp);

		m_ObjectStatus = ObjectStatus::Activated;
		Log(Success, "LightDataService has been initialized.");
		return true;
	}
	else
	{
		Log(Error, "LightDataService is not created!");
		return false;
	}
}

bool LightDataServiceImpl::UpdateLightData()
{
	m_PointLightCBVector.clear();
	m_SphereLightCBVector.clear();

	auto& l_Storage = g_Engine->Get<EntityRegistry>()->Storage<LightComponent>();
	const auto& l_Lights = l_Storage.All();
	const auto& l_Owners = l_Storage.AllOwners();

	if (l_Lights.empty())
		return false;

	for (size_t i = 0; i < l_Lights.size(); i++)
	{
		const LightComponent& l_Light = l_Lights[i];
		EntityID l_EntityID = l_Owners[i];
		auto* l_Transform = g_Engine->Get<EntityRegistry>()->Get<TransformComponent>(l_EntityID);

		if (l_Light.m_LightType == LightType::Point)
		{
			PointLightConstantBuffer l_data;
			if (l_Transform)
				l_data.pos = l_Transform->m_LocalPos;
			l_data.luminance = l_Light.m_RGBColor * l_Light.m_LuminousFlux;
			l_data.luminance.w = l_Light.m_Shape.x;
			m_PointLightCBVector.emplace_back(l_data);
		}
		else if (l_Light.m_LightType == LightType::Sphere)
		{
			SphereLightConstantBuffer l_data;
			if (l_Transform)
				l_data.pos = l_Transform->m_LocalPos;
			l_data.luminance = l_Light.m_RGBColor * l_Light.m_LuminousFlux;
			l_data.luminance.w = l_Light.m_Shape.x;
			m_SphereLightCBVector.emplace_back(l_data);
		}
	}

	return true;
}

bool LightDataServiceImpl::UpdateCSMData()
{
	auto& l_LightStorage = g_Engine->Get<EntityRegistry>()->Storage<LightComponent>();
	const auto& l_Lights  = l_LightStorage.All();
	const auto& l_LightOwners = l_LightStorage.AllOwners();

	if (l_Lights.empty())
		return false;

	EntityID l_SunEntityID = INVALID_ENTITY;
	for (size_t i = 0; i < l_Lights.size(); i++)
	{
		if (l_Lights[i].m_LightType == LightType::Directional)
		{
			l_SunEntityID = l_LightOwners[i];
			break;
		}
	}
	if (l_SunEntityID == INVALID_ENTITY)
		return false;

	auto* l_SunTransform = g_Engine->Get<EntityRegistry>()->Get<TransformComponent>(l_SunEntityID);
	if (!l_SunTransform)
		return false;

	auto* l_Camera = static_cast<ICameraService*>(g_Engine->Get<CameraService>())->GetMainCamera();
	if (!l_Camera)
		return false;

	const uint32_t l_MaxCSMCount = 4;
	const float    l_Lambda      = 0.75f;
	const float    l_ZNear       = l_Camera->m_ZNear;
	const float    l_ZFar        = l_Camera->m_ZFar;
	if (l_ZFar <= l_ZNear)
		return false;

	std::array<float, 4> l_SplitFactors;
	for (int i = 1; i <= (int)l_MaxCSMCount; i++)
	{
		float l_Log     = l_ZNear * std::pow(l_ZFar / l_ZNear, (float)i / (float)l_MaxCSMCount);
		float l_Uniform = l_ZNear + (l_ZFar - l_ZNear) * ((float)i / (float)l_MaxCSMCount);
		l_SplitFactors[i - 1] = l_Log * l_Lambda + l_Uniform * (1.0f - l_Lambda);
	}

	const auto& l_FrustumWS = l_Camera->m_FrustumVerticesWS;

	// l_CornerPos layout:
	//   [0..3]        — near-plane corners (shared across all cascades)
	//   [4 + i*4 + j] — far-plane corner j of cascade i  (i in [0,3], j in [0,3])
	std::array<Vec3, 20> l_CornerPos;
	for (size_t i = 0; i < 4; i++)
		l_CornerPos[i] = l_FrustumWS[i].m_pos;
	for (size_t i = 0; i < l_MaxCSMCount; i++)
	{
		for (size_t j = 0; j < 4; j++)
		{
			auto l_Dir = (l_FrustumWS[j + 4].m_pos - l_FrustumWS[j].m_pos).normalize();
			l_CornerPos[4 + i * 4 + j] = l_FrustumWS[j].m_pos + l_Dir * l_SplitFactors[i];
		}
	}

	auto l_RenderingConfig = g_Engine->Get<RenderingConfigurationService>()->GetRenderingConfig();
	auto l_ShadowMapRes    = (float)l_RenderingConfig.shadowMapResolution;
	auto l_RotInv          = Math::toRotationMatrix(l_SunTransform->m_LocalRot).inverse();

	m_CSMCBVector.clear();
	for (size_t i = 0; i < l_MaxCSMCount; i++)
	{
		std::array<Vertex, 8> l_CascadeVerts;
		if (l_RenderingConfig.CSMFitToScene)
		{
			for (size_t j = 0; j < 4; j++)
				l_CascadeVerts[j].m_pos = l_CornerPos[j];
			for (size_t j = 0; j < 4; j++)
				l_CascadeVerts[j + 4].m_pos = l_CornerPos[4 + i * 4 + j];
		}
		else
		{
			if (i == 0)
			{
				for (size_t j = 0; j < 4; j++)
					l_CascadeVerts[j].m_pos = l_CornerPos[j];
			}
			else
			{
				for (size_t j = 0; j < 4; j++)
					l_CascadeVerts[j].m_pos = l_CornerPos[4 + (i - 1) * 4 + j];
			}
			for (size_t j = 0; j < 4; j++)
				l_CascadeVerts[j + 4].m_pos = l_CornerPos[4 + i * 4 + j];
		}

		AABB l_AABBWorld = Math::GenerateAABB(&l_CascadeVerts[0], 8);
		AABB l_AABBLight = Math::ExtendAABBToBoundingSphere(l_AABBWorld);
		l_AABBLight = Math::RotateAABBToNewSpace(l_AABBLight, l_RotInv);
		l_AABBLight = SnapAABBToShadowMap(l_AABBLight, l_ShadowMapRes);

		Mat4 l_View = l_RotInv;
		AlignMatrixToTexels(l_View, l_ShadowMapRes);

		Mat4 l_Proj = Math::GenerateOrthographicMatrix(
			l_AABBLight.m_boundMin.x, l_AABBLight.m_boundMax.x,
			l_AABBLight.m_boundMin.y, l_AABBLight.m_boundMax.y,
			l_AABBLight.m_boundMax.z, l_AABBLight.m_boundMin.z);

		CSMConstantBuffer l_CB;
		l_CB.v       = l_View;
		l_CB.p       = l_Proj;
		l_CB.AABBMax = l_AABBWorld.m_boundMax;
		l_CB.AABBMin = l_AABBWorld.m_boundMin;
		m_CSMCBVector.emplace_back(l_CB);
	}

	return true;
}

bool LightDataServiceImpl::Update()
{
	if (m_ObjectStatus == ObjectStatus::Activated)
	{
		UpdateLightData();
		UpdateCSMData();

		auto l_renderingServer = g_Engine->getRenderingServer();

		if (m_PointLightCBVector.size() > 0)
		{
			l_renderingServer->Upload(m_PointLightGPUBufferComp, m_PointLightCBVector, 0, m_PointLightCBVector.size());
		}
		if (m_SphereLightCBVector.size() > 0)
		{
			l_renderingServer->Upload(m_SphereLightGPUBufferComp, m_SphereLightCBVector, 0, m_SphereLightCBVector.size());
		}
		if (m_CSMCBVector.size() > 0)
		{
			l_renderingServer->Upload(m_CSMGPUBufferComp, m_CSMCBVector, 0, m_CSMCBVector.size());
		}

		return true;
	}
	else
	{
		m_ObjectStatus = ObjectStatus::Suspended;
		return false;
	}
}

bool LightDataServiceImpl::Terminate()
{
	auto l_renderingServer = g_Engine->getRenderingServer();

	l_renderingServer->Delete(m_PointLightGPUBufferComp);
	l_renderingServer->Delete(m_SphereLightGPUBufferComp);
	l_renderingServer->Delete(m_CSMGPUBufferComp);
	l_renderingServer->Delete(m_GICBufferGPUBufferComp);

	m_ObjectStatus = ObjectStatus::Terminated;
	Log(Success, "LightDataService has been terminated.");
	return true;
}

bool LightDataService::Setup(IServiceConfig* systemConfig)
{
	m_Impl = new LightDataServiceImpl();

	return m_Impl->Setup(systemConfig);
}

bool LightDataService::Initialize()
{
	return m_Impl->Initialize();
}

bool LightDataService::Update()
{
	return m_Impl->Update();
}

bool LightDataService::Terminate()
{
	auto result = m_Impl->Terminate();
	delete m_Impl;
	return result;
}

ObjectStatus LightDataService::GetStatus()
{
	return m_Impl->m_ObjectStatus;
}

GPUBufferComponent* LightDataService::GetPointLightBuffer()
{
	return m_Impl->m_PointLightGPUBufferComp;
}

GPUBufferComponent* LightDataService::GetSphereLightBuffer()
{
	return m_Impl->m_SphereLightGPUBufferComp;
}

GPUBufferComponent* LightDataService::GetCSMBuffer()
{
	return m_Impl->m_CSMGPUBufferComp;
}

GPUBufferComponent* LightDataService::GetGIBuffer()
{
	return m_Impl->m_GICBufferGPUBufferComp;
}
