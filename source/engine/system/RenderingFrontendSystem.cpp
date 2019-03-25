#include "RenderingFrontendSystem.h"

#include "ICoreSystem.h"

extern ICoreSystem* g_pCoreSystem;

INNO_PRIVATE_SCOPE InnoRenderingFrontendSystemNS
{
	ObjectStatus m_objectStatus = ObjectStatus::SHUTDOWN;

	TVec2<unsigned int> m_screenResolution = TVec2<unsigned int>(1280, 720);
	std::string m_windowName;
	bool m_fullScreen = false;

	ThreadSafeQueue<MeshDataComponent*> m_uninitializedMDC;
	ThreadSafeQueue<TextureDataComponent*> m_uninitializedTDC;

	std::atomic<bool> m_isMeshDataPackValid = false;

	std::vector<CullingDataPack> m_cullingDataPack;

	CameraDataPack m_cameraDataPack;
	SunDataPack m_sunDataPack;
	std::vector<CSMDataPack> m_CSMDataPacks;
	std::vector<MeshDataPack> m_meshDataPack;
	std::vector<Plane> m_debugPlanes;
	std::vector<Sphere> m_debugSpheres;

	std::vector<vec2> m_haltonSampler;
	int currentHaltonStep = 0;

	std::function<void(RenderPassType)> f_reloadShader;
	std::function<void()> f_captureEnvironment;

	RenderingConfig m_renderingConfig;

	VisibleComponent* m_selectedVisibleComponent;

	bool setup();
	bool initialize();
	bool update();
	bool terminate();

	float radicalInverse(unsigned int n, unsigned int base);
	void initializeHaltonSampler();
}

float InnoRenderingFrontendSystemNS::radicalInverse(unsigned int n, unsigned int base)
{
	float val = 0.0f;
	float invBase = 1.0f / base, invBi = invBase;
	while (n > 0)
	{
		auto d_i = (n % base);
		val += d_i * invBi;
		n *= (unsigned int)invBase;
		invBi *= invBase;
	}
	return val;
};

void InnoRenderingFrontendSystemNS::initializeHaltonSampler()
{
	// in NDC space
	for (unsigned int i = 0; i < 16; i++)
	{
		m_haltonSampler.emplace_back(vec2(radicalInverse(i, 2) * 2.0f - 1.0f, radicalInverse(i, 3) * 2.0f - 1.0f));
	}
}

bool InnoRenderingFrontendSystemNS::setup()
{
	return true;
}

bool InnoRenderingFrontendSystemNS::initialize()
{
	initializeHaltonSampler();
	m_objectStatus = ObjectStatus::ALIVE;

	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "RenderingFrontendSystem has been initialized.");

	return true;
}

bool InnoRenderingFrontendSystemNS::update()
{
	// copy culling data pack for local scope
	auto l_cullingDataPack = g_pCoreSystem->getPhysicsSystem()->getCullingDataPack();
	if (l_cullingDataPack.has_value())
	{
		m_cullingDataPack = l_cullingDataPack.value();
	}

	// main camera render data
	auto l_cameraComponents = g_pCoreSystem->getGameSystem()->get<CameraComponent>();
	auto l_mainCamera = l_cameraComponents[0];
	auto l_mainCameraTransformComponent = g_pCoreSystem->getGameSystem()->get<TransformComponent>(l_mainCamera->m_parentEntity);

	auto l_p = l_mainCamera->m_projectionMatrix;
	auto l_r =
		InnoMath::getInvertRotationMatrix(
			l_mainCameraTransformComponent->m_globalTransformVector.m_rot
		);
	auto l_t =
		InnoMath::getInvertTranslationMatrix(
			l_mainCameraTransformComponent->m_globalTransformVector.m_pos
		);
	auto r_prev = l_mainCameraTransformComponent->m_globalTransformMatrix_prev.m_rotationMat.inverse();
	auto t_prev = l_mainCameraTransformComponent->m_globalTransformMatrix_prev.m_translationMat.inverse();

	m_cameraDataPack.p_Original = l_p;
	m_cameraDataPack.p_Jittered = l_p;

	if (m_renderingConfig.useTAA)
	{
		//TAA jitter for projection matrix
		auto& l_currentHaltonStep = currentHaltonStep;
		if (l_currentHaltonStep >= 16)
		{
			l_currentHaltonStep = 0;
		}
		m_cameraDataPack.p_Jittered.m02 = m_haltonSampler[l_currentHaltonStep].x / m_screenResolution.x;
		m_cameraDataPack.p_Jittered.m12 = m_haltonSampler[l_currentHaltonStep].y / m_screenResolution.y;
		l_currentHaltonStep += 1;
	}

	m_cameraDataPack.r = l_r;
	m_cameraDataPack.t = l_t;
	m_cameraDataPack.r_prev = r_prev;
	m_cameraDataPack.t_prev = t_prev;
	m_cameraDataPack.globalPos = l_mainCameraTransformComponent->m_globalTransformVector.m_pos;

	m_cameraDataPack.WHRatio = l_mainCamera->m_WHRatio;

	// sun/directional light render data
	auto l_directionalLightComponents = g_pCoreSystem->getGameSystem()->get<DirectionalLightComponent>();
	auto l_directionalLight = l_directionalLightComponents[0];
	auto l_directionalLightTransformComponent = g_pCoreSystem->getGameSystem()->get<TransformComponent>(l_directionalLight->m_parentEntity);

	m_sunDataPack.dir = InnoMath::getDirection(direction::BACKWARD, l_directionalLightTransformComponent->m_globalTransformVector.m_rot);
	m_sunDataPack.luminance = l_directionalLight->m_color * l_directionalLight->m_luminousFlux;
	m_sunDataPack.r = InnoMath::getInvertRotationMatrix(l_directionalLightTransformComponent->m_globalTransformVector.m_rot);

	auto l_CSMSize = l_directionalLight->m_projectionMatrices.size();

	m_CSMDataPacks.clear();

	for (size_t j = 0; j < l_directionalLight->m_projectionMatrices.size(); j++)
	{
		m_CSMDataPacks.emplace_back();

		auto l_shadowSplitCorner = vec4(
			l_directionalLight->m_AABBsInWorldSpace[j].m_boundMin.x,
			l_directionalLight->m_AABBsInWorldSpace[j].m_boundMin.z,
			l_directionalLight->m_AABBsInWorldSpace[j].m_boundMax.x,
			l_directionalLight->m_AABBsInWorldSpace[j].m_boundMax.z
		);

		m_CSMDataPacks[j].p = l_directionalLight->m_projectionMatrices[j];
		m_CSMDataPacks[j].splitCorners = l_shadowSplitCorner;

		auto l_lightRotMat = l_directionalLightTransformComponent->m_globalTransformMatrix.m_rotationMat.inverse();

		m_CSMDataPacks[j].v = l_lightRotMat;
	}

	// objects render data
	m_isMeshDataPackValid = false;

	m_meshDataPack.clear();

	for (auto& i : m_cullingDataPack)
	{
		if (i.visibleComponent != nullptr && i.MDC != nullptr)
		{
			if (i.MDC->m_objectStatus == ObjectStatus::ALIVE)
			{
				auto l_modelPair = i.visibleComponent->m_modelMap.find(i.MDC);
				if (l_modelPair != i.visibleComponent->m_modelMap.end())
				{
					MeshDataPack l_meshDataPack;

					l_meshDataPack.m = i.m;
					l_meshDataPack.m_prev = i.m_prev;
					l_meshDataPack.normalMat = i.normalMat;
					l_meshDataPack.MDC = i.MDC;
					l_meshDataPack.material = l_modelPair->second;
					l_meshDataPack.visiblilityType = i.visibleComponent->m_visiblilityType;

					m_meshDataPack.emplace_back(l_meshDataPack);
				}
			}
		}
	}

	m_isMeshDataPackValid = true;

	return true;
}

bool InnoRenderingFrontendSystemNS::terminate()
{
	m_objectStatus = ObjectStatus::SHUTDOWN;
	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "RenderingFrontendSystem has been terminated.");
	return true;
}

INNO_SYSTEM_EXPORT bool InnoRenderingFrontendSystem::setup()
{
	return InnoRenderingFrontendSystemNS::setup();
}

INNO_SYSTEM_EXPORT bool InnoRenderingFrontendSystem::initialize()
{
	return InnoRenderingFrontendSystemNS::initialize();
}

INNO_SYSTEM_EXPORT bool InnoRenderingFrontendSystem::update()
{
	return InnoRenderingFrontendSystemNS::update();
}

INNO_SYSTEM_EXPORT bool InnoRenderingFrontendSystem::terminate()
{
	return InnoRenderingFrontendSystemNS::terminate();
}

INNO_SYSTEM_EXPORT ObjectStatus InnoRenderingFrontendSystem::getStatus()
{
	return InnoRenderingFrontendSystemNS::m_objectStatus;
}

INNO_SYSTEM_EXPORT bool InnoRenderingFrontendSystem::anyUninitializedMeshDataComponent()
{
	return InnoRenderingFrontendSystemNS::m_uninitializedMDC.size() > 0;
}

INNO_SYSTEM_EXPORT bool InnoRenderingFrontendSystem::anyUninitializedTextureDataComponent()
{
	return InnoRenderingFrontendSystemNS::m_uninitializedTDC.size() > 0;
}

INNO_SYSTEM_EXPORT void InnoRenderingFrontendSystem::registerUninitializedMeshDataComponent(MeshDataComponent * rhs)
{
	InnoRenderingFrontendSystemNS::m_uninitializedMDC.push(rhs);
}

INNO_SYSTEM_EXPORT void InnoRenderingFrontendSystem::registerUninitializedTextureDataComponent(TextureDataComponent * rhs)
{
	InnoRenderingFrontendSystemNS::m_uninitializedTDC.push(rhs);
}

INNO_SYSTEM_EXPORT MeshDataComponent * InnoRenderingFrontendSystem::acquireUninitializedMeshDataComponent()
{
	MeshDataComponent* l_result;
	InnoRenderingFrontendSystemNS::m_uninitializedMDC.tryPop(l_result);
	return l_result;
}

INNO_SYSTEM_EXPORT TextureDataComponent * InnoRenderingFrontendSystem::acquireUninitializedTextureDataComponent()
{
	TextureDataComponent* l_result;
	InnoRenderingFrontendSystemNS::m_uninitializedTDC.tryPop(l_result);
	return l_result;
}

INNO_SYSTEM_EXPORT TVec2<unsigned int> InnoRenderingFrontendSystem::getScreenResolution()
{
	return InnoRenderingFrontendSystemNS::m_screenResolution;
}

INNO_SYSTEM_EXPORT bool InnoRenderingFrontendSystem::setScreenResolution(TVec2<unsigned int> screenResolution)
{
	InnoRenderingFrontendSystemNS::m_screenResolution = screenResolution;
	return true;
}

INNO_SYSTEM_EXPORT RenderingConfig InnoRenderingFrontendSystem::getRenderingConfig()
{
	return InnoRenderingFrontendSystemNS::m_renderingConfig;
}

INNO_SYSTEM_EXPORT bool InnoRenderingFrontendSystem::setRenderingConfig(RenderingConfig renderingConfig)
{
	InnoRenderingFrontendSystemNS::m_renderingConfig = renderingConfig;
	return true;
}

INNO_SYSTEM_EXPORT CameraDataPack InnoRenderingFrontendSystem::getCameraDataPack()
{
	return InnoRenderingFrontendSystemNS::m_cameraDataPack;
}

INNO_SYSTEM_EXPORT SunDataPack InnoRenderingFrontendSystem::getSunDataPack()
{
	return InnoRenderingFrontendSystemNS::m_sunDataPack;
}

INNO_SYSTEM_EXPORT std::vector<CSMDataPack>& InnoRenderingFrontendSystem::getCSMDataPack()
{
	return InnoRenderingFrontendSystemNS::m_CSMDataPacks;
}

INNO_SYSTEM_EXPORT std::optional<std::vector<MeshDataPack>> InnoRenderingFrontendSystem::getMeshDataPack()
{
	if (InnoRenderingFrontendSystemNS::m_isMeshDataPackValid)
	{
		return InnoRenderingFrontendSystemNS::m_meshDataPack;
	}

	return std::nullopt;
}

INNO_SYSTEM_EXPORT std::vector<Plane>& InnoRenderingFrontendSystem::getDebugPlane()
{
	return InnoRenderingFrontendSystemNS::m_debugPlanes;
}

INNO_SYSTEM_EXPORT std::vector<Sphere>& InnoRenderingFrontendSystem::getDebugSphere()
{
	return InnoRenderingFrontendSystemNS::m_debugSpheres;
}