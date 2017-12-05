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

void VisibleComponent::addMeshData(unsigned long int meshDataIndex)
{
	m_meshDatas.emplace_back();
	m_meshDatas[m_meshDatas.size() - 1] = meshDataIndex;
}

std::vector<unsigned long int>& VisibleComponent::getMeshData()
{
	return m_meshDatas;
}

void VisibleComponent::addTextureData(unsigned long int meshDataIndex)
{
	m_textureDatas.emplace_back(meshDataIndex);
}

std::vector<unsigned long int>& VisibleComponent::getTextureData()
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
