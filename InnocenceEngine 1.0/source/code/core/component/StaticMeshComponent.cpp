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
	for (auto i : m_meshData)
	{
		i.update();
	}
	m_textureData.update();
}

void StaticMeshComponent::loadModel(const std::string & modelFileName)
{
	AssetManager::getInstance().loadModel(modelFileName, m_meshData);
}

void StaticMeshComponent::loadTexture(const std::string & textureFileName) const
{
	m_textureData.loadTexture(textureFileName);
}

void StaticMeshComponent::initialize()
{
	setVisibleGameEntityType(visibleGameEntityType::STATIC_MESH);
	m_textureData.init();
}

void StaticMeshComponent::update()
{
	getTransform()->update();
	CoreManager::getInstance().getRenderingManager().render(this);
}

void StaticMeshComponent::shutdown()
{
	m_textureData.shutdown();
	for (auto i : m_meshData)
	{
		i.shutdown();
	}
}