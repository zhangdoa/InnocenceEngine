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
	setVisibleGameEntityType(visibleGameEntityType::STATIC_MESH);
	m_meshData.init();
	m_textureData.init();
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