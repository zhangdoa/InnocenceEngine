#include "RenderingManager.h"

void RenderingManager::changeDrawPolygonMode()
{
	GLRenderingManager::getInstance().changeDrawPolygonMode();
}

void RenderingManager::changeDrawTextureMode()
{
	GLRenderingManager::getInstance().changeDrawTextureMode();
}

void RenderingManager::setup()
{
	m_childManager.emplace_back(&GLWindowManager::getInstance());
	m_childManager.emplace_back(&GLInputManager::getInstance());
	m_childManager.emplace_back(&GLRenderingManager::getInstance());
	for (size_t i = 0; i < m_childManager.size(); i++)
	{
		m_childManager[i].get()->setup();
	}
	GLRenderingManager::getInstance().setScreenResolution(GLWindowManager::getInstance().getScreenResolution());

	f_changeDrawPolygonMode = std::bind(&RenderingManager::changeDrawPolygonMode, this);
	f_changeDrawTextureMode = std::bind(&RenderingManager::changeDrawTextureMode, this);
}

void RenderingManager::initialize()
{
	for (size_t i = 0; i < m_childManager.size(); i++)
	{
		m_childManager[i].get()->initialize();
	}

	for (size_t i = 0; i < g_pGameManager->getInputComponents().size(); i++)
	{
		// @TODO: multi input components need to register to multi map
		GLInputManager::getInstance().addKeyboardInputCallback(g_pGameManager->getInputComponents()[i]->getKeyboardInputCallbackImpl());
		GLInputManager::getInstance().addMouseMovementCallback(g_pGameManager->getInputComponents()[i]->getMouseInputCallbackImpl());
	}

	GLInputManager::getInstance().addKeyboardInputCallback(GLFW_KEY_Q, &f_changeDrawPolygonMode);
	GLInputManager::getInstance().addKeyboardInputCallback(GLFW_KEY_E, &f_changeDrawTextureMode);

	m_basicNormalTemplate = GLRenderingManager::getInstance().addTexture(textureType::NORMAL);
	m_basicAlbedoTemplate = GLRenderingManager::getInstance().addTexture(textureType::ALBEDO);
	m_basicMetallicTemplate = GLRenderingManager::getInstance().addTexture(textureType::METALLIC);
	m_basicRoughnessTemplate = GLRenderingManager::getInstance().addTexture(textureType::ROUGHNESS);
	m_basicAOTemplate = GLRenderingManager::getInstance().addTexture(textureType::AMBIENT_OCCLUSION);

	g_pAssetManager->loadTextureFromDisk({ "basic_normal.png" }, textureType::NORMAL, textureWrapMethod::REPEAT, GLRenderingManager::getInstance().getTexture(textureType::NORMAL, m_basicNormalTemplate));
	g_pAssetManager->loadTextureFromDisk({ "basic_albedo.png" }, textureType::ALBEDO, textureWrapMethod::REPEAT, GLRenderingManager::getInstance().getTexture(textureType::NORMAL, m_basicAlbedoTemplate));
	g_pAssetManager->loadTextureFromDisk({"basic_metallic.png"}, textureType::METALLIC, textureWrapMethod::REPEAT, GLRenderingManager::getInstance().getTexture(textureType::NORMAL, m_basicMetallicTemplate));
	g_pAssetManager->loadTextureFromDisk({ "basic_roughness.png" }, textureType::ROUGHNESS, textureWrapMethod::REPEAT, GLRenderingManager::getInstance().getTexture(textureType::NORMAL, m_basicRoughnessTemplate));
	g_pAssetManager->loadTextureFromDisk({ "basic_ao.png" }, textureType::AMBIENT_OCCLUSION, textureWrapMethod::REPEAT, GLRenderingManager::getInstance().getTexture(textureType::NORMAL, m_basicAOTemplate));

	m_UnitQuadTemplate = GLRenderingManager::getInstance().addMesh();
	auto lastQuadMeshData = GLRenderingManager::getInstance().getMesh(m_UnitQuadTemplate);
	lastQuadMeshData->addUnitQuad();
	lastQuadMeshData->setup(meshDrawMethod::TRIANGLE, true, true);
	lastQuadMeshData->initialize();

	m_UnitCubeTemplate = GLRenderingManager::getInstance().addMesh();
	auto lastCubeMeshData = GLRenderingManager::getInstance().getMesh(m_UnitCubeTemplate);
	lastCubeMeshData->addUnitCube();
	lastCubeMeshData->setup(meshDrawMethod::TRIANGLE, false, false);
	lastCubeMeshData->initialize();

	m_UnitSphereTemplate = GLRenderingManager::getInstance().addMesh();
	auto lastSphereMeshData = GLRenderingManager::getInstance().getMesh(m_UnitSphereTemplate);
	lastSphereMeshData->addUnitSphere();
	lastSphereMeshData->setup(meshDrawMethod::TRIANGLE_STRIP, false, false);
	lastSphereMeshData->initialize();

	for (auto i : g_pGameManager->getVisibleComponents())
	{
		if (i->m_modelFileName != "")
		{
			loadModel(i->m_modelFileName, *i);
		}
		if (i->m_textureFileNameMap.size() != 0)
		{
			for (auto& j : i->m_textureFileNameMap)
			{
				loadTexture({ j.second }, j.first, *i);
			}
		}
	}
	this->setStatus(objectStatus::ALIVE);
	g_pLogManager->printLog("RenderingManager has been initialized.");
}

void RenderingManager::update()
{
	GLWindowManager::getInstance().update();

	GLInputManager::getInstance().update();

if (GLWindowManager::getInstance().getStatus() == objectStatus::STANDBY)
	{
		this->setStatus(objectStatus::STANDBY);
		g_pLogManager->printLog("RenderingManager is stand-by.");
	}
}

void RenderingManager::shutdown()
{
	for (size_t i = 0; i < m_childManager.size(); i++)
	{
		// reverse 'destructor'
		m_childManager[m_childManager.size() - 1 - i].get()->shutdown();
	}
	for (size_t i = 0; i < m_childManager.size(); i++)
	{
		// reverse 'destructor'
		m_childManager[m_childManager.size() - 1 - i].release();
	}
	this->setStatus(objectStatus::SHUTDOWN);
	g_pLogManager->printLog("RenderingManager has been shutdown.");
}

const objectStatus & RenderingManager::getStatus() const
{
	return m_objectStatus;
}

void RenderingManager::setStatus(objectStatus objectStatus)
{
	m_objectStatus = objectStatus;
}

void RenderingManager::render()
{
	//prepare rendering global state
	GLRenderingManager::getInstance().update();
	
	//defer render
	GLRenderingManager::getInstance().render(g_pGameManager->getCameraComponents(), g_pGameManager->getLightComponents(), g_pGameManager->getVisibleComponents());
}

void RenderingManager::assignUnitMesh(VisibleComponent & visibleComponent, meshType unitMeshType)
{
	meshID l_UnitMeshTemplate;
	switch (unitMeshType)
	{
	case meshType::QUAD: l_UnitMeshTemplate = m_UnitQuadTemplate; break;
	case meshType::CUBE: l_UnitMeshTemplate = m_UnitCubeTemplate; break;
	case meshType::SPHERE: l_UnitMeshTemplate = m_UnitSphereTemplate; break;
	}
	visibleComponent.addMeshData(l_UnitMeshTemplate);
	assignDefaultTextures(textureAssignType::OVERWRITE, visibleComponent);
}

void RenderingManager::assignLoadedTexture(textureAssignType textureAssignType, texturePair& loadedtexturePair, VisibleComponent & visibleComponent)
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

void RenderingManager::assignDefaultTextures(textureAssignType textureAssignType, VisibleComponent & visibleComponent)
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

void RenderingManager::assignloadedModel(modelMap& loadedmodelMap, VisibleComponent & visibleComponent)
{
	visibleComponent.setModelMap(loadedmodelMap);
	assignDefaultTextures(textureAssignType::ADD_DEFAULT, visibleComponent);
}

void RenderingManager::loadTexture(const std::vector<std::string> &fileName, textureType textureType, VisibleComponent & visibleComponent)
{
	for (auto& i : fileName)
	{
		auto l_loadedTexturePair = m_loadedTextureMap.find(i);
		// check if this file has already loaded
		if (l_loadedTexturePair != m_loadedTextureMap.end())
		{
			assignLoadedTexture(textureAssignType::OVERWRITE, l_loadedTexturePair->second, visibleComponent);
			g_pLogManager->printLog("innoTexture: " + i + " is already loaded, successfully assigned loaded textureID.");
		}
		else
		{
			auto l_textureID = GLRenderingManager::getInstance().addTexture(textureType);
			auto l_baseTexture = GLRenderingManager::getInstance().getTexture(textureType,l_textureID);
			g_pAssetManager->loadTextureFromDisk({ i }, textureType, visibleComponent.m_textureWrapMethod, l_baseTexture);
			m_loadedTextureMap.emplace(i, texturePair(textureType, l_textureID));
			assignLoadedTexture(textureAssignType::OVERWRITE, texturePair(textureType, l_textureID), visibleComponent);
		}
	}
	
}

void RenderingManager::loadModel(const std::string & fileName, VisibleComponent & visibleComponent)
{
	auto l_convertedFilePath = fileName.substr(0, fileName.find(".")) + ".innoModel";

	// check if this file has already been loaded once
	auto l_loadedmodelMap = m_loadedModelMap.find(l_convertedFilePath);
	if (l_loadedmodelMap != m_loadedModelMap.end())
	{
		assignloadedModel(l_loadedmodelMap->second, visibleComponent);
		g_pLogManager->printLog("innoMesh: " + l_convertedFilePath + " is already loaded, successfully assigned loaded modelMap.");
	}
	else
	{
		auto l_modelPointerMap = g_pAssetManager->loadModelFromDisk(l_convertedFilePath);
		
		std::vector<BaseMesh*> l_baseMesh;
		std::vector<BaseTexture*> l_baseTexture;
		modelMap l_modelMap;

		//construct model data redirector
		for (auto & l_meshRawDataPair : l_modelPointerMap)
		{
			auto l_meshID = GLRenderingManager::getInstance().addMesh();
			l_baseMesh.emplace_back(GLRenderingManager::getInstance().getMesh(l_meshID));

			textureMap l_textureMap;
			for (auto & l_textureFileNamePair : l_meshRawDataPair.second)
			{
				auto l_textureID = GLRenderingManager::getInstance().addTexture(l_textureFileNamePair.first);
				l_baseTexture.emplace_back(GLRenderingManager::getInstance().getTexture(l_textureFileNamePair.first, l_textureID));
				l_textureMap.emplace(l_textureFileNamePair.first, l_textureID);
			}

			l_modelMap.emplace(l_meshID, l_textureMap);
		}

		//then set all of the raw data to the inno version one
		g_pAssetManager->parseloadRawModelData(l_modelPointerMap, visibleComponent.m_meshDrawMethod, visibleComponent.m_textureWrapMethod, l_baseMesh, l_baseTexture);
		
		//mark as loaded
		m_loadedModelMap.emplace(l_convertedFilePath, l_modelMap);
		assignloadedModel(l_modelMap, visibleComponent);
	}
}
