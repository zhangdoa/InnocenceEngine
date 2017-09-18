#include "../../main/stdafx.h"
#include "VisibleComponent.h"


VisibleComponent::VisibleComponent()
{
}


VisibleComponent::~VisibleComponent()
{
}

void VisibleComponent::draw() const
{
	for (auto i : m_textureData)
	{
		i.draw();
	}
	for (auto i : m_meshData)
	{
		i.draw();
	}
	//std::for_each(m_meshData.begin(), m_meshData.end(), [&](MeshData val) {val.draw();});
	//std::for_each(m_textureData.begin(), m_textureData.end(), [&](TextureData val) {val.draw();});
}

const visiblilityType & VisibleComponent::getVisiblilityType() const
{
	return m_visiblilityType;
}

void VisibleComponent::setVisiblilityType(visiblilityType visiblilityType)
{
	m_visiblilityType = visiblilityType;
}

void VisibleComponent::addMeshData(MeshData & MeshData)
{
	m_meshData.emplace_back(MeshData);
}

void VisibleComponent::addTextureData(TextureData & textureData)
{
	m_textureData.emplace_back(textureData);
}

void VisibleComponent::initialize()
{
	if (m_visiblilityType == visiblilityType::SKYBOX)
	{
		MeshData newMeshData;
		newMeshData.addTestSkybox();
		m_meshData.emplace_back(newMeshData);
	}
	for (auto i : m_meshData)
	{
		i.init();
		i.sendDataToGPU();
	}
	for (auto i : m_textureData)
	{
		i.init();
		i.sendDataToGPU();
	}
	//std::for_each(m_meshData.begin(), m_meshData.end(), [&](MeshData val) {val.init(); val.sendDataToGPU();});
	//std::for_each(m_textureData.begin(), m_textureData.end(), [&](TextureData val) {val.init(); val.sendDataToGPU();});
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
