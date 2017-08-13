#include "../../main/stdafx.h"
#include "StaticMeshComponent.h"

StaticMeshComponent::StaticMeshComponent()
{
}


StaticMeshComponent::~StaticMeshComponent()
{
}

void StaticMeshComponent::render()
{
	m_meshData.update();
	m_textureData.update();
}

void StaticMeshComponent::loadMesh(const std::string & meshFileName)
{
	m_meshData.loadData(meshFileName);
}

void StaticMeshComponent::loadTexture(const std::string & textureFileName) const
{
	m_textureData.loadTexture(textureFileName);
}

void StaticMeshComponent::init()
{
	AssetManager::getInstance().loadModel("nanosuit/nanosuit.obj");
	setVisibleGameEntityType(visibleGameEntityType::STATIC_MESH);
	m_meshData.init();
	//TODO: generic mesh loading implementation
	loadMesh("test.inno_mesh");
	m_textureData.init();
	loadTexture("nanosuit/body_dif.png");
	

	
}

void StaticMeshComponent::update()
{
	getTransform()->update();
}

void StaticMeshComponent::shutdown()
{
	m_textureData.shutdown();
	m_meshData.shutdown();
}