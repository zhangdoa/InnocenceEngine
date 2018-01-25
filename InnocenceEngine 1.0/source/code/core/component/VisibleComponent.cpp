#include "../../main/stdafx.h"
#include "VisibleComponent.h"


VisibleComponent::VisibleComponent()
{
}


VisibleComponent::~VisibleComponent()
{
}

void VisibleComponent::addMeshData(meshID& meshID)
{
	m_modelMap.emplace(meshID, textureMap());
}

void VisibleComponent::addTextureData(texturePair & texturePair)
{
	for (auto& l_model : m_modelMap)
	{
		auto& l_texturePair = l_model.second.find(texturePair.first);
		if (l_texturePair == l_model.second.end())
		{
			l_model.second.emplace(texturePair);
		}
	}
}


void VisibleComponent::overwriteTextureData(texturePair& texturePair)
{
	for (auto& l_model : m_modelMap)
	{
		auto& l_texturePair = l_model.second.find(texturePair.first);
		if (l_texturePair == l_model.second.end())
		{
			l_model.second.emplace(texturePair);
		}
		else
		{
			l_texturePair->second = texturePair.second;
		}
	}
}

modelMap& VisibleComponent::getModelMap()
{
	return m_modelMap;
}

void VisibleComponent::setModelMap(modelMap & modelMap)
{
	m_modelMap = modelMap;
}


void VisibleComponent::setup()
{
	IEntity::setup();
}

void VisibleComponent::initialize()
{
}

void VisibleComponent::update()
{
}

void VisibleComponent::shutdown()
{
}
