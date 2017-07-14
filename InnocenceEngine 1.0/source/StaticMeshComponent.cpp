#include "stdafx.h"
#include "StaticMeshComponent.h"

StaticMeshComponent::StaticMeshComponent()
{
}


StaticMeshComponent::~StaticMeshComponent()
{
}

void StaticMeshComponent::loadMesh(const std::string & meshFileName) const
{
}

void StaticMeshComponent::loadTexture(const std::string & textureFileName) const
{
	m_textureData.loadTexture(textureFileName);
}

void StaticMeshComponent::render()
{
}

void StaticMeshComponent::init()
{
	m_textureData.init();
	loadTexture("container.jpg");
	m_meshData.init();
}

void StaticMeshComponent::update()
{
	getTransform()->update();
	m_textureData.update();
	m_meshData.update();
}

void StaticMeshComponent::shutdown()
{
	m_textureData.shutdown();
	m_meshData.shutdown();
}