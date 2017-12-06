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

void VisibleComponent::addGraphicData()
{
}

void VisibleComponent::addMeshData(GameObjectID meshDataIndex)
{
	m_meshDatas.emplace_back(meshDataIndex);
}

std::vector<GameObjectID>& VisibleComponent::getMeshData()
{
	return m_meshDatas;
}

void VisibleComponent::addTextureData(GameObjectID textureDataIndex)
{
	m_textureDatas.emplace_back(textureDataIndex);
}

std::vector<GameObjectID>& VisibleComponent::getTextureData()
{
	return m_textureDatas;
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
