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

void StaticMeshComponent::loadMesh(const std::string & meshFileName) const
{
}

void StaticMeshComponent::loadTexture(const std::string & textureFileName) const
{
	m_textureData.loadTexture(textureFileName);
}

void StaticMeshComponent::init()
{
	setVisibleGameEntityType(visibleGameEntityType::STATIC_MESH);
	m_meshData.init();
	m_meshData.addTestCube();
	m_textureData.init();
	loadTexture("container.jpg");
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