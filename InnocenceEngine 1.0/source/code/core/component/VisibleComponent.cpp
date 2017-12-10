#include "../../main/stdafx.h"
#include "VisibleComponent.h"


VisibleComponent::VisibleComponent()
{
}


VisibleComponent::~VisibleComponent()
{
}

void VisibleComponent::addMeshData(meshDataID& meshDataID)
{
	m_graphicDataMap.emplace(meshDataID, textureDataMap());
}

void VisibleComponent::addTextureData(textureDataPair & textureDataPair)
{
	for (auto& l_graphicData : m_graphicDataMap)
	{
		auto& l_texturePair = l_graphicData.second.find(textureDataPair.first);
		if (l_texturePair == l_graphicData.second.end())
		{
			l_graphicData.second.emplace(textureDataPair);
		}
	}
}


void VisibleComponent::overwriteTextureData(textureDataPair& textureDataPair)
{
	for (auto& l_graphicData : m_graphicDataMap)
	{
		auto& l_texturePair = l_graphicData.second.find(textureDataPair.first);
		if (l_texturePair == l_graphicData.second.end())
		{
			l_graphicData.second.emplace(textureDataPair);
		}
		else
		{
			l_texturePair->second = textureDataPair.second;
		}
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
