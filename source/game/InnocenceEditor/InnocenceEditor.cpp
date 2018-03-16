#include "InnocenceEditor.h"

InnocenceEditor::InnocenceEditor()
{
}

InnocenceEditor::~InnocenceEditor()
{
}

void InnocenceEditor::setup()
{
}

void InnocenceEditor::initialize()
{
}

void InnocenceEditor::update()
{
}

void InnocenceEditor::shutdown()
{
}

const objectStatus & InnocenceEditor::getStatus() const
{
	return m_objectStatus;
}

std::string InnocenceEditor::getGameName() const
{
	return std::string{ typeid(*this).name() }.substr(std::string{ typeid(*this).name() }.find("class"), std::string::npos);
}

std::vector<CameraComponent*>& InnocenceEditor::getCameraComponents()
{
	return m_cameraComponents;
}

std::vector<InputComponent*>& InnocenceEditor::getInputComponents()
{
	return m_inputComponents;
}

std::vector<LightComponent*>& InnocenceEditor::getLightComponents()
{
	return m_lightComponents;
}

std::vector<VisibleComponent*>& InnocenceEditor::getVisibleComponents()
{
	return m_visibleComponents;
}

