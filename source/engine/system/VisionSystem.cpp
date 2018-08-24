#include "VisionSystem.h"

void VisionSystem::setup()
{
	WindowSystemSingletonComponent::getInstance().m_windowName = g_pGameSystem->getGameName();
	setupWindow();
	setupRendering();
	setupGui();

	m_objectStatus = objectStatus::ALIVE;
}

void VisionSystem::setupWindow()
{
#if defined (INNO_RENDERER_OPENGL)
	m_WindowSystem = g_pMemorySystem->spawn<GLWindowSystem>();
#elif defined (INNO_RENDERER_DX)
	m_WindowSystem = g_pMemorySystem->spawn<DXWindowSystem>();
#elif defined (INNO_RENDERER_VULKAN)
#elif defined (INNO_RENDERER_METAL)
#endif
	m_WindowSystem->setup();
}

void VisionSystem::setupRendering()
{
	//setup rendering
	for (auto i : g_pGameSystem->getVisibleComponents())
	{
		if (i->m_visiblilityType != visiblilityType::INVISIBLE)
		{
			if (i->m_visiblilityType == visiblilityType::EMISSIVE)
			{
				RenderingSystemSingletonComponent::getInstance().m_emissiveVisibleComponents.emplace_back(i);
			}
			else if (i->m_visiblilityType == visiblilityType::STATIC_MESH)
			{
				RenderingSystemSingletonComponent::getInstance().m_staticMeshVisibleComponents.emplace_back(i);
			}
		}
	}

#if defined (INNO_RENDERER_OPENGL)
	m_RenderingSystem = g_pMemorySystem->spawn<GLRenderingSystem>();
#elif defined (INNO_RENDERER_DX)
	m_RenderingSystem = g_pMemorySystem->spawn<DXRenderingSystem>();
#elif defined (INNO_RENDERER_VULKAN)
#elif defined (INNO_RENDERER_METAL)
#endif
	m_RenderingSystem->setup();

	m_canRender = true;
}

inline void VisionSystem::setupGui()
{
#if defined (INNO_RENDERER_OPENGL)
	m_GuiSystem = g_pMemorySystem->spawn<GLGuiSystem>();
#elif defined (INNO_RENDERER_DX)
	m_GuiSystem = g_pMemorySystem->spawn<DXGuiSystem>();
#elif defined (INNO_RENDERER_VULKAN)
#elif defined (INNO_RENDERER_METAL)
#endif
	m_GuiSystem->setup();
}

void VisionSystem::initialize()
{
	m_WindowSystem->initialize();
	m_RenderingSystem->initialize();
	m_GuiSystem->initialize();

	g_pLogSystem->printLog("VisionSystem has been initialized.");
}

void VisionSystem::updatePhysics()
{
	RenderingSystemSingletonComponent::getInstance().m_selectedVisibleComponents.clear();
	RenderingSystemSingletonComponent::getInstance().m_inFrustumVisibleComponents.clear();

	if (g_pGameSystem->getCameraComponents().size() > 0)
	{
		m_mouseRay.m_origin = g_pGameSystem->getTransformComponent(g_pGameSystem->getCameraComponents()[0]->getParentEntity())->m_transform.caclGlobalPos();
		m_mouseRay.m_direction = m_WindowSystem->calcMousePositionInWorldSpace();

		auto l_cameraAABB = g_pGameSystem->getCameraComponents()[0]->m_AABB;

		auto l_ray = g_pGameSystem->getCameraComponents()[0]->m_rayOfEye;

		for (auto& j : g_pGameSystem->getVisibleComponents())
		{
			if (j->m_visiblilityType == visiblilityType::STATIC_MESH || j->m_visiblilityType == visiblilityType::EMISSIVE)
			{
				if (j->m_AABB.intersectCheck(m_mouseRay))
				{
					RenderingSystemSingletonComponent::getInstance().m_selectedVisibleComponents.emplace_back(j);
				}
				if (l_cameraAABB.intersectCheck(j->m_AABB))
				{
					RenderingSystemSingletonComponent::getInstance().m_inFrustumVisibleComponents.emplace_back(j);
				}
			}
		}
	}
}

void VisionSystem::update()
{
	m_WindowSystem->update();

	if (m_WindowSystem->getStatus() == objectStatus::ALIVE)
	{
		updatePhysics();

		if (m_canRender)
		{
			m_canRender = false;
			m_RenderingSystem->update();
			m_GuiSystem->update();
			m_WindowSystem->swapBuffer();
			m_canRender = true;
		}

		// update the transform data @TODO: ugly
		std::for_each(g_pGameSystem->getTransformComponents().begin(), g_pGameSystem->getTransformComponents().end(), [&](TransformComponent* val)
		{
			val->m_transform.update();
		});
	}
	else
	{
		g_pLogSystem->printLog("VisionSystem is stand-by.");
		m_objectStatus = objectStatus::STANDBY;
	}
}

void VisionSystem::shutdown()
{
	if (m_WindowSystem->getStatus() == objectStatus::ALIVE)
	{
		m_GuiSystem->shutdown();
		m_RenderingSystem->shutdown();
		m_WindowSystem->shutdown();
	}
	m_objectStatus = objectStatus::SHUTDOWN;
	g_pLogSystem->printLog("VisionSystem has been shutdown.");
}

const objectStatus & VisionSystem::getStatus() const
{
	return m_objectStatus;
}

void VisionSystem::changeDrawPolygonMode()
{
	if (m_polygonMode == 2)
	{
		m_polygonMode = 0;
	}
	else
	{
		m_polygonMode += 1;
	}
}

void VisionSystem::changeDrawTextureMode()
{
	if (m_textureMode == 4)
	{
		m_textureMode = 0;
	}
	else
	{
		m_textureMode += 1;
	}
}

void VisionSystem::changeShadingMode()
{
	if (m_shadingMode == 1)
	{
		m_shadingMode = 0;
	}
	else
	{
		m_shadingMode += 1;
	}
}
