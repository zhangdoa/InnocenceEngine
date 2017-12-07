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
	m_graphicDataMap.emplace(meshDataID, textureDataMap());
}

void VisibleComponent::addTextureData(meshDataID meshDataID, textureDataID textureDataID, textureType textureType)
{
	m_graphicDataMap.find(meshDataID)->second.emplace(textureType, textureDataID);
}

void VisibleComponent::addTextureData(textureDataID textureDataID, textureType textureType)
{
	for (auto& l_graphicDatas : m_graphicDataMap)
	{
		l_graphicDatas.second.emplace(textureType, textureDataID);
	}
}

void VisibleComponent::addTextureData(textureDataPair textureDataPair)
{
	for (auto& l_graphicDatas : m_graphicDataMap)
	{
		l_graphicDatas.second.emplace(textureDataPair);
	}
}

graphicDataMap& VisibleComponent::getGraphicDataMap()
{
	return m_graphicDataMap;
}

void VisibleComponent::setGraphicDataMap(graphicDataMap & graphicDataMap)
{
	m_graphicDataMap = graphicDataMap;
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
