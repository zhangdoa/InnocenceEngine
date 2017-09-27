#include "../../main/stdafx.h"
#include "VisibleComponent.h"


VisibleComponent::VisibleComponent()
{
}


VisibleComponent::~VisibleComponent()
{
}

void VisibleComponent::draw()
{
	for (size_t i = 0; i < m_textureData.size(); i++)
	{
		m_textureData[i].draw();
	}
	for (size_t i = 0; i < m_meshData.size(); i++)
	{
		m_meshData[i].draw();
	}
}

const visiblilityType & VisibleComponent::getVisiblilityType() const
{
	return m_visiblilityType;
}

void VisibleComponent::setVisiblilityType(visiblilityType visiblilityType)
{
	m_visiblilityType = visiblilityType;
}

void VisibleComponent::addMeshData()
{
	MeshData newMeshData;
	m_meshData.emplace_back(newMeshData);
}

void VisibleComponent::addTextureData()
{
	TextureData newTextureData;
	m_textureData.emplace_back(newTextureData);
}

std::vector<MeshData>& VisibleComponent::getMeshData()
{
	return m_meshData;
}

std::vector<TextureData>& VisibleComponent::getTextureData()
{
	return m_textureData;
}

void VisibleComponent::initialize()
{
	if (m_visiblilityType == visiblilityType::SKYBOX)
	{
		this->addMeshData();
		m_meshData[0].addTestSkybox();
	}
	else 
	{
		this->addMeshData();
		m_meshData[0].addTestCube();
	}
	for (size_t i = 0; i < m_meshData.size(); i++)
	{
		m_meshData[i].init();
		m_meshData[i].sendDataToGPU();
	}
}

void VisibleComponent::update()
{
	getTransform()->update();
}

void VisibleComponent::shutdown()
{
	for (auto i : m_textureData)
	{
		i.shutdown();
	}
	for (auto i : m_meshData)
	{
		i.shutdown();
	}
	//std::for_each(m_textureData.begin(), m_textureData.end(), [](TextureData val) {val.shutdown(); });
	//std::for_each(m_meshData.begin(), m_meshData.end(), [](MeshData val) {val.shutdown(); });
}
