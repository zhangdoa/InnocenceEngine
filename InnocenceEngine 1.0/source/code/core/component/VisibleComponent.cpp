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
	for (size_t i = 0; i < m_graphicData.size(); i++)
	{
		m_graphicData[i].draw();
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

void VisibleComponent::addGraphicData()
{
	GraphicData newGraphicData;
	m_graphicData.emplace_back(newGraphicData);
}

std::vector<GraphicData>& VisibleComponent::getGraphicData()
{
	return m_graphicData;
}

void VisibleComponent::initialize()
{
	if (m_visiblilityType == visiblilityType::SKYBOX)
	{
		addGraphicData();
	}
	if (m_visiblilityType == visiblilityType::BILLBOARD)
	{
		addGraphicData();
	}
	for (size_t i = 0; i < m_graphicData.size(); i++)
	{
		m_graphicData[i].setVisiblilityType(m_visiblilityType);
		m_graphicData[i].init();
	}
}

void VisibleComponent::update()
{
	getTransform()->update();
}

void VisibleComponent::shutdown()
{
	for (auto i : m_graphicData)
	{
		i.shutdown();
	}
}
