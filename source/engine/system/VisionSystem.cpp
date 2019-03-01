#include "VisionSystem.h"

#include "../component/RenderingSystemComponent.h"
#include "../component/WindowSystemComponent.h"
#include "../component/GameSystemComponent.h"
#include "../component/PhysicsSystemComponent.h"

#if defined INNO_PLATFORM_WIN
#include "DXWindowSystem.h"
#include "DXRenderingSystem.h"
#include "DXGuiSystem.h"
#endif

#include "GLWindowSystem.h"
#include "GLRenderingSystem.h"
#include "GLGuiSystem.h"

#if defined INNO_PLATFORM_LINUX
#include "VKWindowSystem.h"
#include "VKRenderingSystem.h"
#include "VKGuiSystem.h"
#endif

#include "ICoreSystem.h"

extern ICoreSystem* g_pCoreSystem;

INNO_PRIVATE_SCOPE InnoVisionSystemNS
{
	IWindowSystem* m_windowSystem;
	IRenderingSystem* m_renderingSystem;
	IGuiSystem* m_guiSystem;

	bool setupWindow(void* hInstance, void* hPrevInstance, char* pScmdline, int nCmdshow);
	bool setupRendering();
	bool setupGui();

	std::vector<InnoFuture<void>> m_asyncTask;
	std::vector<CullingDataPack> m_cullingDataPack;
	ObjectStatus m_objectStatus = ObjectStatus::SHUTDOWN;
}

INNO_SYSTEM_EXPORT bool InnoVisionSystem::setup(void* hInstance, void* hPrevInstance, char* pScmdline, int nCmdshow)
{
	std::string l_windowArguments = pScmdline;

	if (l_windowArguments == "")
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "VisionSystem: No arguments found!");
		InnoVisionSystemNS::m_objectStatus = ObjectStatus::STANDBY;
		return false;
	}

	auto l_argPos = l_windowArguments.find("renderer");
	if (l_argPos == 0)
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "VisionSystem: No renderer argument found!");
		InnoVisionSystemNS::m_objectStatus = ObjectStatus::STANDBY;
		return false;
	}

	std::string l_rendererArguments = l_windowArguments.substr(l_argPos + 9);

	if (l_rendererArguments == "DX")
	{
#if defined INNO_PLATFORM_WIN
		InnoVisionSystemNS::m_windowSystem = new DXWindowSystem();
		InnoVisionSystemNS::m_renderingSystem = new DXRenderingSystem();
		InnoVisionSystemNS::m_guiSystem = new DXGuiSystem();
#else
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "VisionSystem: DirectX is only supported on Windows OS!");
		return false;
#endif
	}
	else if (l_rendererArguments == "GL")
	{
		InnoVisionSystemNS::m_windowSystem = new GLWindowSystem();
		InnoVisionSystemNS::m_renderingSystem = new GLRenderingSystem();
		InnoVisionSystemNS::m_guiSystem = new GLGuiSystem();
	}
	else if (l_rendererArguments == "VK")
	{
#if defined INNO_PLATFORM_LINUX
		InnoVisionSystemNS::m_windowSystem = new VKWindowSystem();
		InnoVisionSystemNS::m_renderingSystem = new VKRenderingSystem();
		InnoVisionSystemNS::m_guiSystem = new VKGuiSystem();
#else
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "VisionSystem: Vulkan is only supported on Linux OS!");
		return false;
#endif
	}
	else
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "VisionSystem: Incorrect renderer argument " + l_rendererArguments + " !");
		InnoVisionSystemNS::m_objectStatus = ObjectStatus::STANDBY;
		return false;
	}
	if (!InnoVisionSystemNS::setupWindow(hInstance, hPrevInstance, pScmdline, nCmdshow))
	{
		InnoVisionSystemNS::m_objectStatus = ObjectStatus::STANDBY;
		return false;
	};
	if (!InnoVisionSystemNS::setupRendering())
	{
		InnoVisionSystemNS::m_objectStatus = ObjectStatus::STANDBY;
		return false;
	}
	if (!InnoVisionSystemNS::setupGui())
	{
		InnoVisionSystemNS::m_objectStatus = ObjectStatus::STANDBY;
		return false;
	}

	return true;
}

bool InnoVisionSystemNS::setupWindow(void* hInstance, void* hPrevInstance, char* pScmdline, int nCmdshow)
{
	if (!InnoVisionSystemNS::m_windowSystem->setup(hInstance, hPrevInstance, pScmdline, nCmdshow))
	{
		return false;
	}
	return true;
}

bool InnoVisionSystemNS::setupRendering()
{
	if (!InnoVisionSystemNS::m_renderingSystem->setup())
	{
		return false;
	}

	return true;
}

bool InnoVisionSystemNS::setupGui()
{
	if (!InnoVisionSystemNS::m_guiSystem->setup())
	{
		return false;
	}
	return true;
}

INNO_SYSTEM_EXPORT bool InnoVisionSystem::initialize()
{
	InnoVisionSystemNS::m_windowSystem->initialize();
	InnoVisionSystemNS::m_renderingSystem->initialize();
	InnoVisionSystemNS::m_guiSystem->initialize();

	Sphere testSphere;
	testSphere.m_center = vec4(2.0f, 6.0f, -4.0f, 1.0f);
	testSphere.m_radius = 4.0f;
	RenderingSystemComponent::get().m_debugSpheres.emplace_back(testSphere);

	Plane testPlane;
	testPlane.m_normal = vec4(0.0f, 1.0f, 0.0f, 0.0f);
	testPlane.m_distance = 3.0f;
	RenderingSystemComponent::get().m_debugPlanes.emplace_back(testPlane);

	InnoVisionSystemNS::m_objectStatus = ObjectStatus::ALIVE;
	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "VisionSystem has been initialized.");
	return true;
}

INNO_SYSTEM_EXPORT bool InnoVisionSystem::update()
{
	if (GameSystemComponent::get().m_isLoadingScene)
	{
		return true;
	}

	if (!RenderingSystemComponent::get().m_allowRender)
	{
		// copy culling data pack for local scope
		if (PhysicsSystemComponent::get().m_isCullingDataPackValid)
		{
			InnoVisionSystemNS::m_cullingDataPack = PhysicsSystemComponent::get().m_cullingDataPack.getRawData();
		}

		// main camera render data
		auto l_mainCamera = GameSystemComponent::get().m_CameraComponents[0];
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

		RenderingSystemComponent::get().m_CamProjOriginal = l_p;
		RenderingSystemComponent::get().m_CamProjJittered = l_p;

		if (RenderingSystemComponent::get().m_useTAA)
		{
			//TAA jitter for projection matrix
			auto& l_currentHaltonStep = RenderingSystemComponent::get().currentHaltonStep;
			if (l_currentHaltonStep >= 16)
			{
				l_currentHaltonStep = 0;
			}
			RenderingSystemComponent::get().m_CamProjJittered.m02 = RenderingSystemComponent::get().HaltonSampler[l_currentHaltonStep].x / WindowSystemComponent::get().m_windowResolution.x;
			RenderingSystemComponent::get().m_CamProjJittered.m12 = RenderingSystemComponent::get().HaltonSampler[l_currentHaltonStep].y / WindowSystemComponent::get().m_windowResolution.y;
			l_currentHaltonStep += 1;
		}

		RenderingSystemComponent::get().m_CamRot = l_r;
		RenderingSystemComponent::get().m_CamTrans = l_t;
		RenderingSystemComponent::get().m_CamRot_prev = r_prev;
		RenderingSystemComponent::get().m_CamTrans_prev = t_prev;
		RenderingSystemComponent::get().m_CamGlobalPos = l_mainCameraTransformComponent->m_globalTransformVector.m_pos;

		// sun/directional light render data
		auto l_directionalLight = GameSystemComponent::get().m_DirectionalLightComponents[0];
		auto l_directionalLightTransformComponent = g_pCoreSystem->getGameSystem()->get<TransformComponent>(l_directionalLight->m_parentEntity);

		RenderingSystemComponent::get().m_sunDir = InnoMath::getDirection(direction::BACKWARD, l_directionalLightTransformComponent->m_globalTransformVector.m_rot);
		RenderingSystemComponent::get().m_sunLuminance = l_directionalLight->m_color * l_directionalLight->m_luminousFlux;
		RenderingSystemComponent::get().m_sunRot = InnoMath::getInvertRotationMatrix(l_directionalLightTransformComponent->m_globalTransformVector.m_rot);

		auto l_CSMSize = l_directionalLight->m_projectionMatrices.size();

		RenderingSystemComponent::get().m_CSMProjs.clear();
		RenderingSystemComponent::get().m_CSMProjs.reserve(l_CSMSize);
		RenderingSystemComponent::get().m_CSMSplitCorners.clear();
		RenderingSystemComponent::get().m_CSMSplitCorners.reserve(l_CSMSize);
		RenderingSystemComponent::get().m_CSMViews.clear();
		RenderingSystemComponent::get().m_CSMViews.reserve(l_CSMSize);

		for (size_t j = 0; j < l_directionalLight->m_projectionMatrices.size(); j++)
		{
			RenderingSystemComponent::get().m_CSMProjs.emplace_back();
			RenderingSystemComponent::get().m_CSMSplitCorners.emplace_back();
			RenderingSystemComponent::get().m_CSMViews.emplace_back();

			auto l_shadowSplitCorner = vec4(
				l_directionalLight->m_AABBsInWorldSpace[j].m_boundMin.x,
				l_directionalLight->m_AABBsInWorldSpace[j].m_boundMin.z,
				l_directionalLight->m_AABBsInWorldSpace[j].m_boundMax.x,
				l_directionalLight->m_AABBsInWorldSpace[j].m_boundMax.z
			);

			RenderingSystemComponent::get().m_CSMProjs[j] = l_directionalLight->m_projectionMatrices[j];
			RenderingSystemComponent::get().m_CSMSplitCorners[j] = l_shadowSplitCorner;

			auto l_lightRotMat = l_directionalLightTransformComponent->m_globalTransformMatrix.m_rotationMat.inverse();

			RenderingSystemComponent::get().m_CSMViews[j] = l_lightRotMat;
		}

		// objects render data
		RenderingSystemComponent::get().m_isRenderDataPackValid = false;

		RenderingSystemComponent::get().m_renderDataPack.clear();

		for (auto& i : InnoVisionSystemNS::m_cullingDataPack)
		{
			if (i.visibleComponent != nullptr && i.MDC != nullptr)
			{
				if (i.MDC->m_objectStatus == ObjectStatus::ALIVE)
				{
					auto l_modelPair = i.visibleComponent->m_modelMap.find(i.MDC);
					if (l_modelPair != i.visibleComponent->m_modelMap.end())
					{
						RenderDataPack l_renderDataPack;

						l_renderDataPack.m = i.m;
						l_renderDataPack.m_prev = i.m_prev;
						l_renderDataPack.normalMat = i.normalMat;
						l_renderDataPack.MDC = i.MDC;
						l_renderDataPack.material = l_modelPair->second;
						l_renderDataPack.visiblilityType = i.visibleComponent->m_visiblilityType;

						RenderingSystemComponent::get().m_renderDataPack.emplace_back(l_renderDataPack);
					}
				}
			}
		}

		RenderingSystemComponent::get().m_isRenderDataPackValid = true;

		RenderingSystemComponent::get().m_selectedVisibleComponent = PhysicsSystemComponent::get().m_selectedVisibleComponent;

		RenderingSystemComponent::get().m_allowRender = true;

		auto prepareRenderDataTask = g_pCoreSystem->getTaskSystem()->submit([]()
		{
		});

		InnoVisionSystemNS::m_asyncTask.emplace_back(std::move(prepareRenderDataTask));
	}

	g_pCoreSystem->getTaskSystem()->shrinkFutureContainer(InnoVisionSystemNS::m_asyncTask);

	if (InnoVisionSystemNS::m_windowSystem->getStatus() == ObjectStatus::ALIVE)
	{
		InnoVisionSystemNS::m_windowSystem->update();

		if (!RenderingSystemComponent::get().m_isRendering && RenderingSystemComponent::get().m_allowRender)
		{
			RenderingSystemComponent::get().m_allowRender = false;

			RenderingSystemComponent::get().m_isRendering = true;

			InnoVisionSystemNS::m_renderingSystem->update();
			InnoVisionSystemNS::m_guiSystem->update();
			InnoVisionSystemNS::m_windowSystem->swapBuffer();

			RenderingSystemComponent::get().m_isRendering = false;
		}
		return true;
	}
	else
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_WARNING, "VisionSystem is stand-by.");
		InnoVisionSystemNS::m_objectStatus = ObjectStatus::STANDBY;
		return false;
	}
}

INNO_SYSTEM_EXPORT bool InnoVisionSystem::terminate()
{
	if (!InnoVisionSystemNS::m_guiSystem->terminate())
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "GuiSystem can't be terminated!");
		return false;
	}
	if (!InnoVisionSystemNS::m_renderingSystem->terminate())
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "RenderingSystem can't be terminated!");
		return false;
	}
	if (!InnoVisionSystemNS::m_windowSystem->terminate())
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "WindowSystem can't be terminated!");
		return false;
	}
	InnoVisionSystemNS::m_objectStatus = ObjectStatus::SHUTDOWN;
	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "VisionSystem has been terminated.");
	return true;
}

INNO_SYSTEM_EXPORT ObjectStatus InnoVisionSystem::getStatus()
{
	return InnoVisionSystemNS::m_objectStatus;
}

INNO_SYSTEM_EXPORT bool InnoVisionSystem::resize()
{
	InnoVisionSystemNS::m_renderingSystem->resize();
	return true;
}
