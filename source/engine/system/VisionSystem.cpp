#include "VisionSystem.h"
#include<atomic>
#include "../common/ComponentHeaders.h"
#if defined (INNO_RENDERER_OPENGL)
#include "GLRenderer/GLWindowSystem.h"
#include "GLRenderer/GLRenderingSystem.h"
#include "GLRenderer/GLGuiSystem.h"
#elif defined (INNO_RENDERER_DX)
#include "DXRenderer/DXWindowSystem.h"
#include "DXRenderer/DXRenderingSystem.h"
#include "DXRenderer/DXGuiSystem.h"
#endif
#include "MemorySystem.h"
#include "LogSystem.h"
#include "GameSystem.h"
#include "PhysicsSystem.h"
#include "InputSystem.h"

#include "../component/RenderingSystemSingletonComponent.h"

namespace InnoVisionSystem
{
	void setupWindow();
	void setupRendering();
	void setupGui();

	void updatePhysics();

	void changeDrawPolygonMode();
	void changeDrawTextureMode();
	void changeShadingMode();

	//Physics data
	Ray m_mouseRay;

	//Rendering Data
	std::atomic<bool> m_canRender;
	std::function<void()> f_changeDrawPolygonMode;
	std::function<void()> f_changeDrawTextureMode;
	std::function<void()> f_changeShadingMode;

	int m_polygonMode = 2;
	int m_textureMode = 0;
	int m_shadingMode = 0;

#if defined (INNO_RENDERER_OPENGL)
#define WindowSystem GLWindowSystem
#define RenderingSystem GLRenderingSystem
#define GuiSystem GLGuiSystem
#elif defined (INNO_RENDERER_DX)
#define WindowSystem DXWindowSystem
#define RenderingSystem DXRenderingSystem
#define GuiSystem DXGuiSystem
#elif defined (INNO_RENDERER_VULKAN)
#elif defined (INNO_RENDERER_METAL)
#endif

	objectStatus m_VisionSystemStatus = objectStatus::SHUTDOWN;
}

void InnoVisionSystem::setup()
{
	WindowSystemSingletonComponent::getInstance().m_windowName = InnoGameSystem::getGameName();
	setupWindow();
	setupRendering();
	setupGui();

	m_VisionSystemStatus = objectStatus::ALIVE;
}

void InnoVisionSystem::setupWindow()
{
	WindowSystem::setup();
}

void InnoVisionSystem::setupRendering()
{
	//setup rendering
	for (auto i : InnoGameSystem::getVisibleComponents())
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

	RenderingSystem::setup();

	m_canRender = true;
}

void InnoVisionSystem::setupGui()
{
	GuiSystem::setup();
}

void InnoVisionSystem::initialize()
{
	WindowSystem::initialize();
	RenderingSystem::initialize();
	GuiSystem::initialize();

	InnoLogSystem::printLog("VisionSystem has been initialized.");
}

void InnoVisionSystem::updatePhysics()
{
	RenderingSystemSingletonComponent::getInstance().m_selectedVisibleComponents.clear();
	RenderingSystemSingletonComponent::getInstance().m_inFrustumVisibleComponents.clear();

	if (InnoGameSystem::getCameraComponents().size() > 0)
	{
		m_mouseRay.m_origin = InnoGameSystem::getTransformComponent(InnoGameSystem::getCameraComponents()[0]->m_parentEntity)->m_transform.caclGlobalPos();
		m_mouseRay.m_direction = InnoInputSystem::calcMousePositionInWorldSpace();

		auto l_cameraAABB = InnoGameSystem::getCameraComponents()[0]->m_AABB;

		auto l_ray = InnoGameSystem::getCameraComponents()[0]->m_rayOfEye;

		for (auto& j : InnoGameSystem::getVisibleComponents())
		{
			if (j->m_visiblilityType == visiblilityType::STATIC_MESH || j->m_visiblilityType == visiblilityType::EMISSIVE)
			{
				if (InnoMath::intersectCheck(j->m_AABB, m_mouseRay))
				{
					RenderingSystemSingletonComponent::getInstance().m_selectedVisibleComponents.emplace_back(j);
				}
				if (InnoMath::intersectCheck(l_cameraAABB, j->m_AABB))
				{
					RenderingSystemSingletonComponent::getInstance().m_inFrustumVisibleComponents.emplace_back(j);
				}
			}
		}
	}
}

void InnoVisionSystem::update()
{
	WindowSystem::update();

	if (WindowSystem::getStatus() == objectStatus::ALIVE)
	{
		updatePhysics();

		if (m_canRender)
		{
			m_canRender = false;
			RenderingSystem::update();
			GuiSystem::update();
			WindowSystem::swapBuffer();
			m_canRender = true;
		}

		// update the transform data @TODO: ugly
		std::for_each(InnoGameSystem::getTransformComponents().begin(), InnoGameSystem::getTransformComponents().end(), [&](TransformComponent* val)
		{
			val->m_transform.update();
		});
	}
	else
	{
		InnoLogSystem::printLog("VisionSystem is stand-by.");
		m_VisionSystemStatus = objectStatus::STANDBY;
	}
}

void InnoVisionSystem::shutdown()
{

	if (WindowSystem::getStatus() == objectStatus::ALIVE)
	{
		GuiSystem::shutdown();
		RenderingSystem::shutdown();
		WindowSystem::shutdown();
	}
	m_VisionSystemStatus = objectStatus::SHUTDOWN;
	InnoLogSystem::printLog("VisionSystem has been shutdown.");
}

objectStatus InnoVisionSystem::getStatus()
{
	return m_VisionSystemStatus;
}

void InnoVisionSystem::changeDrawPolygonMode()
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

void InnoVisionSystem::changeDrawTextureMode()
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

void InnoVisionSystem::changeShadingMode()
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
