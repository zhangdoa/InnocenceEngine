#include "PerFrameDataService.h"

#include "../Common/LogService.h"
#include "CameraService.h"
#include "EntityRegistry.h"
#include "../Component/TransformComponent.h"
#include "RenderingConfigurationService.h"
#include "DrawCallService.h"

#include "../Engine.h"
using namespace Inno;

namespace Inno
{
	struct PerFrameDataServiceImpl
	{
		ObjectStatus m_ObjectStatus = ObjectStatus::Terminated;

		mutable std::shared_mutex m_Mutex;

		std::vector<PerFrameConstantBuffer> m_perFrameCBs;

		GPUBufferComponent* m_PerFrameCBufferGPUBufferComp;
		GPUBufferComponent* m_PerFrameCBufferPrevGPUBufferComp;

		bool Setup(IServiceConfig* systemConfig);
		bool Initialize();
		bool Update();
		bool Terminate();

		float RadicalInverse(uint32_t n, uint32_t base);
		bool UpdatePerFrameConstantBuffer();

		GPUBufferComponent* GetCurrentFramePerFrameBuffer();
		GPUBufferComponent* GetPreviousFramePerFrameBuffer();
	};
}

float PerFrameDataServiceImpl::RadicalInverse(uint32_t n, uint32_t base)
{
	float val = 0.0f;
	float invBase = 1.0f / base;
	float invBi = invBase;

	while (n > 0)
	{
		uint32_t d_i = (n % base);
		val += d_i * invBi;
		n /= base;
		invBi *= invBase;
	}
	return val;
}

GPUBufferComponent* PerFrameDataServiceImpl::GetCurrentFramePerFrameBuffer()
{
	auto l_frameCount = g_Engine->getGraphicsService()->GetFrameCountSinceLaunch();
	auto l_isOddFrame = l_frameCount % 2 == 1;
	return l_isOddFrame ? m_PerFrameCBufferGPUBufferComp : m_PerFrameCBufferPrevGPUBufferComp;
}

GPUBufferComponent* PerFrameDataServiceImpl::GetPreviousFramePerFrameBuffer()
{
	auto l_frameCount = g_Engine->getGraphicsService()->GetFrameCountSinceLaunch();
	auto l_isOddFrame = l_frameCount % 2 == 1;
	return l_isOddFrame ? m_PerFrameCBufferPrevGPUBufferComp : m_PerFrameCBufferGPUBufferComp;
}

bool PerFrameDataServiceImpl::Setup(IServiceConfig* systemConfig)
{
	auto l_renderingServer = g_Engine->getGraphicsService();

	m_PerFrameCBufferGPUBufferComp = l_renderingServer->AddGPUBufferComponent("PerFrameCBuffer/");
	m_PerFrameCBufferPrevGPUBufferComp = l_renderingServer->AddGPUBufferComponent("PerFrameCBufferPrev/");

	m_ObjectStatus = ObjectStatus::Created;
	return true;
}

bool PerFrameDataServiceImpl::Initialize()
{
	if (m_ObjectStatus == ObjectStatus::Created)
	{
		m_perFrameCBs.resize(g_Engine->getGraphicsService()->GetSwapChainImageCount());

		auto l_renderingServer = g_Engine->getGraphicsService();

		m_PerFrameCBufferGPUBufferComp->m_GPUAccessibility = Accessibility::ReadOnly;
		m_PerFrameCBufferGPUBufferComp->m_ElementCount = 1;
		m_PerFrameCBufferGPUBufferComp->m_ElementSize = sizeof(PerFrameConstantBuffer);

		l_renderingServer->Initialize(m_PerFrameCBufferGPUBufferComp);

		m_PerFrameCBufferPrevGPUBufferComp->m_GPUAccessibility = Accessibility::ReadOnly;
		m_PerFrameCBufferPrevGPUBufferComp->m_ElementCount = 1;
		m_PerFrameCBufferPrevGPUBufferComp->m_ElementSize = sizeof(PerFrameConstantBuffer);

		l_renderingServer->Initialize(m_PerFrameCBufferPrevGPUBufferComp);

		m_ObjectStatus = ObjectStatus::Activated;
		Log(Success, "PerFrameDataService has been initialized.");
		return true;
	}
	else
	{
		Log(Error, "PerFrameDataService is not created!");
		return false;
	}
}

bool PerFrameDataServiceImpl::UpdatePerFrameConstantBuffer()
{
	auto l_camera = g_Engine->Get<CameraService>()->GetActiveCamera();
	if (l_camera == nullptr)
		return false;

	auto l_p = l_camera->m_ProjectionMatrix;

	PerFrameConstantBuffer l_perFrameCB = {};
	l_perFrameCB.frameIndex = g_Engine->getGraphicsService()->GetFrameCountSinceLaunch();
	l_perFrameCB.modelCount = static_cast<uint32_t>(g_Engine->Get<DrawCallService>()->GetGPUModelData().size());
	l_perFrameCB.p_original = l_p;
	l_perFrameCB.p_jittered = l_p;

	auto l_renderingConfigurationService = g_Engine->Get<RenderingConfigurationService>();
	auto l_renderingConfig = l_renderingConfigurationService->GetRenderingConfig();
	auto l_screenResolution = l_renderingConfigurationService->GetScreenResolution();
	if (l_renderingConfig.useTAA)
	{
		l_perFrameCB.p_jittered.m02 = (RadicalInverse(l_perFrameCB.frameIndex, 3) * 2.0f - 1.0f) / l_screenResolution.x;
		l_perFrameCB.p_jittered.m12 = (RadicalInverse(l_perFrameCB.frameIndex, 4) * 2.0f - 1.0f) / l_screenResolution.y;
	}

	l_perFrameCB.radianceCacheHaltonJitter = Vec2(RadicalInverse(l_perFrameCB.frameIndex, 3) * 8.0f, RadicalInverse(l_perFrameCB.frameIndex, 5) * 8.0f);

	auto& l_CameraStorage = g_Engine->Get<EntityRegistry>()->Storage<CameraComponent>();
	const auto& l_CameraAll = l_CameraStorage.All();
	const auto& l_CameraOwners = l_CameraStorage.AllOwners();
	EntityID l_CameraEntityID = INVALID_ENTITY;
	for (size_t ci = 0; ci < l_CameraAll.size(); ci++)
	{
		if (&l_CameraAll[ci] == l_camera)
		{
			l_CameraEntityID = l_CameraOwners[ci];
			break;
		}
	}
	auto* l_CameraTransform = l_CameraEntityID != INVALID_ENTITY ? g_Engine->Get<EntityRegistry>()->Get<TransformComponent>(l_CameraEntityID) : nullptr;
	auto r = l_CameraTransform ? Math::getInvertRotationMatrix(l_CameraTransform->m_LocalRot) : Mat4();
	auto t = l_CameraTransform ? Math::getInvertTranslationMatrix(Vec4(l_CameraTransform->m_LocalPos, 1.0f)) : Mat4();

	if (l_CameraTransform) l_perFrameCB.camera_posWS = l_CameraTransform->m_LocalPos;
	l_perFrameCB.v = r * t;

	l_perFrameCB.zNear = l_camera->m_ZNear;
	l_perFrameCB.zFar = l_camera->m_ZFar;

	l_perFrameCB.p_inv = l_p.inverse();
	l_perFrameCB.v_inv = l_perFrameCB.v.inverse();
	l_perFrameCB.viewportSize.x = (float)l_screenResolution.x;
	l_perFrameCB.viewportSize.y = (float)l_screenResolution.y;
	l_perFrameCB.minLogLuminance = -10.0f;
	l_perFrameCB.maxLogLuminance = 16.0f;
	l_perFrameCB.aperture = l_camera->m_Aperture;
	l_perFrameCB.shutterTime = l_camera->m_ShutterTime;
	l_perFrameCB.ISO = l_camera->m_ISO;

	auto& l_LightStorage = g_Engine->Get<EntityRegistry>()->Storage<LightComponent>();
	if (l_LightStorage.All().empty())
		return false;
	auto& l_sun = l_LightStorage.All()[0];

	EntityID l_SunEntityID = l_LightStorage.AllOwners()[0];
	auto* l_SunTransform = g_Engine->Get<EntityRegistry>()->Get<TransformComponent>(l_SunEntityID);
	if (l_SunTransform)
		l_perFrameCB.sun_direction = Math::getDirection(Direction::Forward, l_SunTransform->m_LocalRot);
	l_perFrameCB.sun_illuminance = l_sun.m_RGBColor * l_sun.m_LuminousFlux;

	static uint32_t currentCascade = 0;
	auto l_renderingCapability = l_renderingConfigurationService->GetRenderingCapability();
	currentCascade = currentCascade < l_renderingCapability.maxCSMSplits - 1 ? ++currentCascade : 0;
	l_perFrameCB.activeCascade = currentCascade;

	m_perFrameCBs[g_Engine->getGraphicsService()->GetCurrentFrame()] = l_perFrameCB;

	return true;
}

bool PerFrameDataServiceImpl::Update()
{
	if (m_ObjectStatus == ObjectStatus::Activated)
	{
		std::lock_guard<std::shared_mutex> l_lock(m_Mutex);

		UpdatePerFrameConstantBuffer();

		auto l_renderingServer = g_Engine->getGraphicsService();
		auto l_currentFramePerFrameBuffer = GetCurrentFramePerFrameBuffer();
		l_renderingServer->Upload(l_currentFramePerFrameBuffer, &m_perFrameCBs[l_renderingServer->GetCurrentFrame()]);

		return true;
	}
	else
	{
		m_ObjectStatus = ObjectStatus::Suspended;
		return false;
	}
}

bool PerFrameDataServiceImpl::Terminate()
{
	auto l_renderingServer = g_Engine->getGraphicsService();

	l_renderingServer->Delete(m_PerFrameCBufferGPUBufferComp);
	l_renderingServer->Delete(m_PerFrameCBufferPrevGPUBufferComp);

	m_ObjectStatus = ObjectStatus::Terminated;
	Log(Success, "PerFrameDataService has been terminated.");
	return true;
}

bool PerFrameDataService::Setup(IServiceConfig* systemConfig)
{
	m_Impl = new PerFrameDataServiceImpl();

	return m_Impl->Setup(systemConfig);
}

bool PerFrameDataService::Initialize()
{
	return m_Impl->Initialize();
}

bool PerFrameDataService::Update()
{
	return m_Impl->Update();
}

bool PerFrameDataService::Terminate()
{
	auto result = m_Impl->Terminate();
	delete m_Impl;
	return result;
}

ObjectStatus PerFrameDataService::GetStatus()
{
	return m_Impl->m_ObjectStatus;
}

const PerFrameConstantBuffer& PerFrameDataService::GetPerFrameConstantBuffer()
{
	std::lock_guard<std::shared_mutex> l_lock(m_Impl->m_Mutex);
	return m_Impl->m_perFrameCBs[g_Engine->getGraphicsService()->GetCurrentFrame()];
}

GPUBufferComponent* PerFrameDataService::GetCurrentFrameBuffer()
{
	return m_Impl->GetCurrentFramePerFrameBuffer();
}

GPUBufferComponent* PerFrameDataService::GetPreviousFrameBuffer()
{
	return m_Impl->GetPreviousFramePerFrameBuffer();
}
