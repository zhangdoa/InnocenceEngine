#include "VisionSystem.h"
#include<atomic>
#include "../../common/ComponentHeaders.h"

#include "../LowLevelSystem/MemorySystem.h"

#include "../../component/LogSystemSingletonComponent.h"

#include "../HighLevelSystem/GameSystem.h"

#include "../../component/WindowSystemSingletonComponent.h"
#include "../../component/RenderingSystemSingletonComponent.h"
#include "../../component/GameSystemSingletonComponent.h"

#include "../HighLevelSystem/DXWindowSystem.h"
#include "../HighLevelSystem/DXRenderingSystem.h"
#include "../HighLevelSystem/DXGuiSystem.h"

#include "../HighLevelSystem/GLWindowSystem.h"
#include "../HighLevelSystem/GLRenderingSystem.h"
#include "../HighLevelSystem/GLGuiSystem.h"

namespace InnoVisionSystem
{
	using WindowSystem = DXWindowSystem::Instance;
	using RenderingSystem = DXRenderingSystem::Instance;
	using GuiSystem = DXGuiSystem::Instance;

	void setupWindow();
	void setupRendering();
	void setupGui();

	void updateCulling();

	void changeDrawPolygonMode();
	void changeDrawTextureMode();
	void changeShadingMode();

	//Rendering Data
	std::atomic<bool> m_canRender;
	std::function<void()> f_changeDrawPolygonMode;
	std::function<void()> f_changeDrawTextureMode;
	std::function<void()> f_changeShadingMode;

	int m_polygonMode = 2;
	int m_textureMode = 0;
	int m_shadingMode = 0;

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
	if (WindowSystemSingletonComponent::getInstance().m_pScmdline)
	{
		std::string l_windowArguments = WindowSystemSingletonComponent::getInstance().m_pScmdline;
		auto l_argPos = l_windowArguments.find("renderer");
		if (l_argPos == 0)
		{
			LogSystemSingletonComponent::getInstance().m_log.push("ERROR::VisionSystem:: No renderer argument found!");
			m_VisionSystemStatus = objectStatus::STANDBY;
			return;
		}
		else
		{
			std::string l_rendererArguments = l_windowArguments.substr(l_argPos + 9);
			if (l_rendererArguments == "DX")
			{
				using WindowSystem = DXWindowSystem::Instance;
				using RenderingSystem = DXRenderingSystem::Instance;
				using GuiSystem = DXGuiSystem::Instance;
			}
			else if (l_rendererArguments == "GL")
			{
				using WindowSystem = GLWindowSystem::Instance;
				using RenderingSystem = GLRenderingSystem::Instance;
				using GuiSystem = GLGuiSystem::Instance;
			}
			else
			{
				LogSystemSingletonComponent::getInstance().m_log.push("ERROR::VisionSystem:: Incorrect renderer argument!");
				m_VisionSystemStatus = objectStatus::STANDBY;
				return;
			}
		}
	}
	WindowSystem::get().setup();
}

void InnoVisionSystem::setupRendering()
{
	//setup rendering
	for (auto i : GameSystemSingletonComponent::getInstance().m_visibleComponents)
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

	RenderingSystem::get().setup();

	m_canRender = true;
}

void InnoVisionSystem::setupGui()
{
	GuiSystem::get().setup();
}

void InnoVisionSystem::initialize()
{
	WindowSystem::get().initialize();
	RenderingSystem::get().initialize();
	GuiSystem::get().initialize();

	LogSystemSingletonComponent::getInstance().m_log.push("VisionSystem has been initialized.");
}

void InnoVisionSystem::updateCulling()
{
	RenderingSystemSingletonComponent::getInstance().m_selectedVisibleComponents.clear();
	RenderingSystemSingletonComponent::getInstance().m_inFrustumVisibleComponents.clear();

	if (GameSystemSingletonComponent::getInstance().m_cameraComponents.size() > 0)
	{
		WindowSystemSingletonComponent::getInstance().m_mouseRay.m_origin = InnoGameSystem::getTransformComponent(GameSystemSingletonComponent::getInstance().m_cameraComponents[0]->m_parentEntity)->m_transform.caclGlobalPos();
		WindowSystemSingletonComponent::getInstance().m_mouseRay.m_direction = WindowSystemSingletonComponent::getInstance().m_mousePositionInWorldSpace;

		auto l_cameraAABB = GameSystemSingletonComponent::getInstance().m_cameraComponents[0]->m_AABB;

		auto l_ray = GameSystemSingletonComponent::getInstance().m_cameraComponents[0]->m_rayOfEye;

		for (auto& j : GameSystemSingletonComponent::getInstance().m_visibleComponents)
		{
			if (j->m_visiblilityType == visiblilityType::STATIC_MESH || j->m_visiblilityType == visiblilityType::EMISSIVE)
			{
				if (InnoMath::intersectCheck(j->m_AABB, WindowSystemSingletonComponent::getInstance().m_mouseRay))
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
	WindowSystem::get().update();

	if (WindowSystem::get().getStatus() == objectStatus::ALIVE)
	{
		updateCulling();

		if (m_canRender)
		{
			m_canRender = false;
			RenderingSystem::get().update();
			GuiSystem::get().update();
			WindowSystem::get().swapBuffer();
			m_canRender = true;
		}

		// update the transform data @TODO: ugly
		std::for_each(GameSystemSingletonComponent::getInstance().m_transformComponents.begin(), GameSystemSingletonComponent::getInstance().m_transformComponents.end(), [&](TransformComponent* val)
		{
			val->m_transform.update();
		});
	}
	else
	{
		LogSystemSingletonComponent::getInstance().m_log.push("VisionSystem is stand-by.");
		m_VisionSystemStatus = objectStatus::STANDBY;
	}
}

void InnoVisionSystem::shutdown()
{

	if (WindowSystem::get().getStatus() == objectStatus::ALIVE)
	{
		GuiSystem::get().shutdown();
		RenderingSystem::get().shutdown();
		WindowSystem::get().shutdown();
	}
	m_VisionSystemStatus = objectStatus::SHUTDOWN;
	LogSystemSingletonComponent::getInstance().m_log.push("VisionSystem has been shutdown.");
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
