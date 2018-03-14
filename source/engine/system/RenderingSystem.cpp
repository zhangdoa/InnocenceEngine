#include "RenderingSystem.h"

void RenderingSystem::changeDrawPolygonMode()
{
	GLRenderingSystem::getInstance().changeDrawPolygonMode();
}

void RenderingSystem::changeDrawTextureMode()
{
	GLRenderingSystem::getInstance().changeDrawTextureMode();
}

void RenderingSystem::setup()
{
	m_childSystem.emplace_back(&GLWindowSystem::getInstance());
	m_childSystem.emplace_back(&GLInputSystem::getInstance());
	m_childSystem.emplace_back(&GLRenderingSystem::getInstance());
	for (size_t i = 0; i < m_childSystem.size(); i++)
	{
		m_childSystem[i].get()->setup();
	}
	GLRenderingSystem::getInstance().setScreenResolution(GLWindowSystem::getInstance().getScreenResolution());

	f_changeDrawPolygonMode = std::bind(&RenderingSystem::changeDrawPolygonMode, this);
	f_changeDrawTextureMode = std::bind(&RenderingSystem::changeDrawTextureMode, this);
}

void RenderingSystem::initialize()
{
	for (size_t i = 0; i < m_childSystem.size(); i++)
	{
		m_childSystem[i].get()->initialize();
	}

	for (size_t i = 0; i < g_pGameSystem->getInputComponents().size(); i++)
	{
		// @TODO: multi input components need to register to multi map
		GLInputSystem::getInstance().addKeyboardInputCallback(g_pGameSystem->getInputComponents()[i]->getKeyboardInputCallbackImpl());
		GLInputSystem::getInstance().addMouseMovementCallback(g_pGameSystem->getInputComponents()[i]->getMouseInputCallbackImpl());
	}

	GLInputSystem::getInstance().addKeyboardInputCallback(GLFW_KEY_Q, &f_changeDrawPolygonMode);
	GLInputSystem::getInstance().addKeyboardInputCallback(GLFW_KEY_E, &f_changeDrawTextureMode);

	m_basicNormalTemplate = GLRenderingSystem::getInstance().addTexture(textureType::NORMAL);
	m_basicAlbedoTemplate = GLRenderingSystem::getInstance().addTexture(textureType::ALBEDO);
	m_basicMetallicTemplate = GLRenderingSystem::getInstance().addTexture(textureType::METALLIC);
	m_basicRoughnessTemplate = GLRenderingSystem::getInstance().addTexture(textureType::ROUGHNESS);
	m_basicAOTemplate = GLRenderingSystem::getInstance().addTexture(textureType::AMBIENT_OCCLUSION);

	g_pAssetSystem->loadTextureFromDisk({ "basic_normal.png" }, textureType::NORMAL, textureWrapMethod::REPEAT, GLRenderingSystem::getInstance().getTexture(textureType::NORMAL, m_basicNormalTemplate));
	g_pAssetSystem->loadTextureFromDisk({ "basic_albedo.png" }, textureType::ALBEDO, textureWrapMethod::REPEAT, GLRenderingSystem::getInstance().getTexture(textureType::NORMAL, m_basicAlbedoTemplate));
	g_pAssetSystem->loadTextureFromDisk({"basic_metallic.png"}, textureType::METALLIC, textureWrapMethod::REPEAT, GLRenderingSystem::getInstance().getTexture(textureType::NORMAL, m_basicMetallicTemplate));
	g_pAssetSystem->loadTextureFromDisk({ "basic_roughness.png" }, textureType::ROUGHNESS, textureWrapMethod::REPEAT, GLRenderingSystem::getInstance().getTexture(textureType::NORMAL, m_basicRoughnessTemplate));
	g_pAssetSystem->loadTextureFromDisk({ "basic_ao.png" }, textureType::AMBIENT_OCCLUSION, textureWrapMethod::REPEAT, GLRenderingSystem::getInstance().getTexture(textureType::NORMAL, m_basicAOTemplate));

	m_UnitQuadTemplate = GLRenderingSystem::getInstance().addMesh();
	auto lastQuadMeshData = GLRenderingSystem::getInstance().getMesh(m_UnitQuadTemplate);
	lastQuadMeshData->addUnitQuad();
	lastQuadMeshData->setup(meshDrawMethod::TRIANGLE, true, true);
	lastQuadMeshData->initialize();

	m_UnitCubeTemplate = GLRenderingSystem::getInstance().addMesh();
	auto lastCubeMeshData = GLRenderingSystem::getInstance().getMesh(m_UnitCubeTemplate);
	lastCubeMeshData->addUnitCube();
	lastCubeMeshData->setup(meshDrawMethod::TRIANGLE, false, false);
	lastCubeMeshData->initialize();

	m_UnitSphereTemplate = GLRenderingSystem::getInstance().addMesh();
	auto lastSphereMeshData = GLRenderingSystem::getInstance().getMesh(m_UnitSphereTemplate);
	lastSphereMeshData->addUnitSphere();
	lastSphereMeshData->setup(meshDrawMethod::TRIANGLE_STRIP, false, false);
	lastSphereMeshData->initialize();

	for (auto i : g_pGameSystem->getVisibleComponents())
	{
		if (i->m_meshType == meshType::CUSTOM)
		{
			if (i->m_modelFileName != "")
			{
				loadModel(i->m_modelFileName, *i);
			}
		}
		else
		{
			assignUnitMesh(*i, i->m_meshType);
		}
		if (i->m_textureFileNameMap.size() != 0)
		{
			for (auto& j : i->m_textureFileNameMap)
			{
				loadTexture({ j.second }, j.first, *i);
			}
		}
	}
	m_objectStatus = objectStatus::ALIVE;
	g_pLogSystem->printLog("RenderingSystem has been initialized.");
}

void RenderingSystem::update()
{
	GLWindowSystem::getInstance().update();

	GLInputSystem::getInstance().update();

if (GLWindowSystem::getInstance().getStatus() == objectStatus::STANDBY)
	{
		m_objectStatus = objectStatus::STANDBY;
		g_pLogSystem->printLog("RenderingSystem is stand-by.");
	}
}

void RenderingSystem::shutdown()
{
	for (size_t i = 0; i < m_childSystem.size(); i++)
	{
		// reverse 'destructor'
		m_childSystem[m_childSystem.size() - 1 - i].get()->shutdown();
	}
	for (size_t i = 0; i < m_childSystem.size(); i++)
	{
		// reverse 'destructor'
		m_childSystem[m_childSystem.size() - 1 - i].release();
	}
	m_objectStatus = objectStatus::SHUTDOWN;
	g_pLogSystem->printLog("RenderingSystem has been shutdown.");
}

const objectStatus & RenderingSystem::getStatus() const
{
	return m_objectStatus;
}

void RenderingSystem::render()
{
	//prepare rendering global state
	GLRenderingSystem::getInstance().update();
	
	//defer render
	GLRenderingSystem::getInstance().render(g_pGameSystem->getCameraComponents(), g_pGameSystem->getLightComponents(), g_pGameSystem->getVisibleComponents());
}

meshID RenderingSystem::addMesh()
{
	return GLRenderingSystem::getInstance().addMesh();
}

textureID RenderingSystem::addTexture(textureType textureType)
{
	return GLRenderingSystem::getInstance().addTexture(textureType);
}

BaseMesh * RenderingSystem::getMesh(meshID meshID)
{
	return GLRenderingSystem::getInstance().getMesh(meshID);
}

BaseTexture * RenderingSystem::getTexture(textureType textureType, textureID textureID)
{
	return GLRenderingSystem::getInstance().getTexture(textureType, textureID);
}

void RenderingSystem::assignUnitMesh(VisibleComponent & visibleComponent, meshType meshType)
{
	meshID l_UnitMeshTemplate;
	switch (meshType)
	{
	case meshType::QUAD: l_UnitMeshTemplate = m_UnitQuadTemplate; break;
	case meshType::CUBE: l_UnitMeshTemplate = m_UnitCubeTemplate; break;
	case meshType::SPHERE: l_UnitMeshTemplate = m_UnitSphereTemplate; break;
	}
	visibleComponent.addMeshData(l_UnitMeshTemplate);
	assignDefaultTextures(textureAssignType::OVERWRITE, visibleComponent);
}

void RenderingSystem::assignLoadedTexture(textureAssignType textureAssignType, texturePair& loadedtexturePair, VisibleComponent & visibleComponent)
{
	if (textureAssignType == textureAssignType::ADD_DEFAULT)
	{
		visibleComponent.addTextureData(loadedtexturePair);
	}
	else if (textureAssignType == textureAssignType::OVERWRITE)
	{
		visibleComponent.overwriteTextureData(loadedtexturePair);
	}
}

void RenderingSystem::assignDefaultTextures(textureAssignType textureAssignType, VisibleComponent & visibleComponent)
{
	if (visibleComponent.m_visiblilityType == visiblilityType::STATIC_MESH)
	{
		assignLoadedTexture(textureAssignType, texturePair(textureType::NORMAL, m_basicNormalTemplate), visibleComponent);
		assignLoadedTexture(textureAssignType, texturePair(textureType::ALBEDO, m_basicAlbedoTemplate), visibleComponent);
		assignLoadedTexture(textureAssignType, texturePair(textureType::METALLIC, m_basicMetallicTemplate), visibleComponent);
		assignLoadedTexture(textureAssignType, texturePair(textureType::ROUGHNESS, m_basicRoughnessTemplate), visibleComponent);
		assignLoadedTexture(textureAssignType, texturePair(textureType::AMBIENT_OCCLUSION, m_basicAOTemplate), visibleComponent);
	}
}

void RenderingSystem::assignloadedModel(modelMap& loadedmodelMap, VisibleComponent & visibleComponent)
{
	visibleComponent.setModelMap(loadedmodelMap);
	assignDefaultTextures(textureAssignType::ADD_DEFAULT, visibleComponent);
}

void RenderingSystem::loadTexture(const std::vector<std::string> &fileName, textureType textureType, VisibleComponent & visibleComponent)
{
	for (auto& i : fileName)
	{
		auto l_loadedTexturePair = m_loadedTextureMap.find(i);
		// check if this file has already loaded
		if (l_loadedTexturePair != m_loadedTextureMap.end())
		{
			assignLoadedTexture(textureAssignType::OVERWRITE, l_loadedTexturePair->second, visibleComponent);
			g_pLogSystem->printLog("innoTexture: " + i + " is already loaded, successfully assigned loaded textureID.");
		}
		else
		{
			auto l_textureID = GLRenderingSystem::getInstance().addTexture(textureType);
			auto l_baseTexture = GLRenderingSystem::getInstance().getTexture(textureType,l_textureID);
			g_pAssetSystem->loadTextureFromDisk({ i }, textureType, visibleComponent.m_textureWrapMethod, l_baseTexture);
			m_loadedTextureMap.emplace(i, texturePair(textureType, l_textureID));
			assignLoadedTexture(textureAssignType::OVERWRITE, texturePair(textureType, l_textureID), visibleComponent);
		}
	}
	
}

void RenderingSystem::loadModel(const std::string & fileName, VisibleComponent & visibleComponent)
{
	auto l_convertedFilePath = fileName.substr(0, fileName.find(".")) + ".innoModel";

	// check if this file has already been loaded once
	auto l_loadedmodelMap = m_loadedModelMap.find(l_convertedFilePath);
	if (l_loadedmodelMap != m_loadedModelMap.end())
	{
		assignloadedModel(l_loadedmodelMap->second, visibleComponent);
		g_pLogSystem->printLog("innoMesh: " + l_convertedFilePath + " is already loaded, successfully assigned loaded modelMap.");
	}
	else
	{
		modelMap l_modelMap;
		g_pAssetSystem->loadModelFromDisk(fileName, l_modelMap, visibleComponent.m_meshDrawMethod, visibleComponent.m_textureWrapMethod);

		//mark as loaded
		m_loadedModelMap.emplace(l_convertedFilePath, l_modelMap);
		assignloadedModel(l_modelMap, visibleComponent);
	}
}
