#include "RenderingManager.h"

void RenderingManager::changeDrawPolygonMode()
{
	GLRenderingManager::getInstance().changeDrawPolygonMode();
}

void RenderingManager::changeDrawTextureMode()
{
	GLRenderingManager::getInstance().changeDrawTextureMode();
}

meshID RenderingManager::addMesh()
{
	return GLRenderingManager::getInstance().addMesh();
}

textureID RenderingManager::add2DTexture()
{
	return GLRenderingManager::getInstance().add2DTexture();
}

textureID RenderingManager::add2DHDRTexture()
{
	return  GLRenderingManager::getInstance().add2DHDRTexture();
}

textureID RenderingManager::add3DTexture()
{
	return GLRenderingManager::getInstance().add3DTexture();
}

textureID RenderingManager::add3DHDRTexture()
{
	return  GLRenderingManager::getInstance().add3DHDRTexture();
}

BaseMesh* RenderingManager::getMesh(meshID meshID)
{
	return GLRenderingManager::getInstance().getMesh(meshID);
}

Base2DTexture* RenderingManager::get2DTexture(textureID textureID)
{
	return  GLRenderingManager::getInstance().get2DTexture(textureID);
}

Base2DTexture * RenderingManager::get2DHDRTexture(textureID textureID)
{
	return GLRenderingManager::getInstance().get2DHDRTexture(textureID);
}

Base3DTexture * RenderingManager::get3DTexture(textureID textureID)
{
	return GLRenderingManager::getInstance().get3DTexture(textureID);
}

Base3DTexture * RenderingManager::get3DHDRTexture(textureID textureID)
{
	return GLRenderingManager::getInstance().get3DHDRTexture(textureID);
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

	for (size_t i = 0; i < m_InputComponents.size(); i++)
	{
		// @TODO: multi input components need to register to multi map
		GLInputManager::getInstance().addKeyboardInputCallback(m_InputComponents[i]->getKeyboardInputCallbackImpl());
		GLInputManager::getInstance().addMouseMovementCallback(m_InputComponents[i]->getMouseInputCallbackImpl());
	}

	GLInputManager::getInstance().addKeyboardInputCallback(GLFW_KEY_Q, &f_changeDrawPolygonMode);
	GLInputManager::getInstance().addKeyboardInputCallback(GLFW_KEY_E, &f_changeDrawTextureMode);

	m_basicNormalTemplate = AssetManager::loadTextureFromDisk("basic_normal.png", textureType::NORMAL, textureWrapMethod::REPEAT);
	m_basicAlbedoTemplate = AssetManager::loadTextureFromDisk("basic_albedo.png", textureType::ALBEDO, textureWrapMethod::REPEAT);
	m_basicMetallicTemplate = AssetManager::loadTextureFromDisk("basic_metallic.png", textureType::METALLIC, textureWrapMethod::REPEAT);
	m_basicRoughnessTemplate = AssetManager::loadTextureFromDisk("basic_roughness.png", textureType::ROUGHNESS, textureWrapMethod::REPEAT);
	m_basicAOTemplate = AssetManager::loadTextureFromDisk("basic_ao.png", textureType::AMBIENT_OCCLUSION, textureWrapMethod::REPEAT);

	m_UnitCubeTemplate = this->addMesh();
	auto lastMeshData = this->getMesh(m_UnitCubeTemplate);
	lastMeshData->addUnitCube();
	lastMeshData->setup(meshDrawMethod::TRIANGLE, false, false);
	lastMeshData->initialize();

	m_UnitSphereTemplate = this->addMesh();
	lastMeshData = this->getMesh(m_UnitSphereTemplate);
	lastMeshData->addUnitSphere();
	lastMeshData->setup(meshDrawMethod::TRIANGLE_STRIP, false, false);
	lastMeshData->initialize();

	m_UnitQuadTemplate = this->addMesh();
	lastMeshData = this->getMesh(m_UnitQuadTemplate);
	lastMeshData->addUnitQuad();
	lastMeshData->setup(meshDrawMethod::TRIANGLE, true, true);
	lastMeshData->initialize();

	this->setStatus(objectStatus::ALIVE);
	LogManager::getInstance().printLog("RenderingManager has been initialized.");
}

void RenderingManager::update()
{
	GLWindowManager::getInstance().update();

	GLInputManager::getInstance().update();

if (GLWindowManager::getInstance().getStatus() == objectStatus::STANDBY)
	{
		this->setStatus(objectStatus::STANDBY);
		LogManager::getInstance().printLog("RenderingManager is stand-by.");
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
	LogManager::getInstance().printLog("RenderingManager has been shutdown.");
}

void RenderingManager::render()
{
	//prepare rendering global state
	GLRenderingManager::getInstance().update();
	
	//defer render
	GLRenderingManager::getInstance().Render(m_CameraComponents, m_LightComponents, m_VisibleComponents);
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
			LogManager::getInstance().printLog("innoTexture: " + i + " is already loaded, successfully assigned loaded textureID.");
		}
		else
		{
			auto l_texturePair = texturePair(textureType, AssetManager::loadTextureFromDisk(i, textureType, visibleComponent.m_textureWrapMethod));
			m_loadedTextureMap.emplace(i, l_texturePair);
			assignLoadedTexture(textureAssignType::OVERWRITE, l_texturePair, visibleComponent);
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
		LogManager::getInstance().printLog("innoMesh: " + l_convertedFilePath + " is already loaded, successfully assigned loaded modelMap.");
	}
	else
	{
		auto l_modelMap = AssetManager::loadModelFromDisk(l_convertedFilePath, visibleComponent.m_meshDrawMethod, visibleComponent.m_textureWrapMethod);
		m_loadedModelMap.emplace(l_convertedFilePath, l_modelMap);
		assignloadedModel(l_modelMap, visibleComponent); 
	}
}
