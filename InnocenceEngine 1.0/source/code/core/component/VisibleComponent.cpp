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

void VisibleComponent::addMeshData(meshDataID meshDataID)
{
	m_graphicDatas.emplace(meshDataID, textureDataMap());
}

void VisibleComponent::addTextureData(meshDataID meshDataID, textureDataID textureDataID, textureType textureType)
{
	m_graphicDatas.find(meshDataID)->second.emplace(textureType, textureDataID);
}

void VisibleComponent::addTextureData(textureDataID textureDataID, textureType textureType)
{
	for (auto& l_graphicDatas : m_graphicDatas)
	{
		l_graphicDatas.second.emplace(textureType, textureDataID);
	}
}

graphicDataMap& VisibleComponent::getGraphicDatas()
{
	return m_graphicDatas;
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
