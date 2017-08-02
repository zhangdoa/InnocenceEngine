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
	setVisibleGameEntityType(STATIC_MESH);
	m_textureData.init();
	loadTexture("container.jpg");
	m_meshData.init();
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