#include "../../main/stdafx.h"
#include "VisibleComponent.h"


VisibleComponent::VisibleComponent()
{
}


VisibleComponent::~VisibleComponent()
{
}

const visiblilityType & VisibleComponent::getVisiblilityType() const
{
	return m_visiblilityType;
}

void VisibleComponent::setVisiblilityType(visiblilityType visiblilityType)
{
	m_visiblilityType = visiblilityType;
}

std::vector<StaticMeshData>& VisibleComponent::getMeshData() const
{
	return m_meshData;
}

std::vector<TextureData>& VisibleComponent::getTextureData() const
{
	return m_textureData;
}

void VisibleComponent::addMeshData()
{
	StaticMeshData newMeshData;
	newMeshData.init();
	newMeshData.sendDataToGPU();
	m_meshData.emplace_back(newMeshData);
}

void VisibleComponent::addTextureData()
{
	TextureData newTextureData;
	newTextureData.init();
	m_textureData.emplace_back(newTextureData);
}

void VisibleComponent::initialize()
{
}

void VisibleComponent::update()
{
	getTransform()->update();
}

void VisibleComponent::shutdown()
{
}
