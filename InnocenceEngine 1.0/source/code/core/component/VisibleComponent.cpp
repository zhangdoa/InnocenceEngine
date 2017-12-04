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
	for (size_t i = 0; i < m_meshDatas.size(); i++)
	{
		m_meshDatas[i]->draw();
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

const meshDrawMethod & VisibleComponent::getMeshDrawMethod() const
{
	return m_meshDrawMethod;
}

void VisibleComponent::setMeshDrawMethod(meshDrawMethod meshDrawMethod)
{
	m_meshDrawMethod = meshDrawMethod;
}

const textureWrapMethod & VisibleComponent::getTextureWrapMethod() const
{
	return m_textureWrapMethod;
}

void VisibleComponent::setTextureWrapMethod(textureWrapMethod textureWrapMethod)
{
	m_textureWrapMethod = textureWrapMethod;
}

void VisibleComponent::addMeshData(MeshData* meshData)
{
	m_meshDatas.emplace_back(meshData);
}

void VisibleComponent::addTextureData(TextureData* textureData)
{
	m_textureDatas.emplace_back(textureData);
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
