#include "InnocenceEditor.h"

void InnocenceEditor::setup()
{
	GameSystem::setup();
}

void InnocenceEditor::initialize()
{
	GameSystem::initialize();
}

void InnocenceEditor::update()
{
	GameSystem::update();
}

void InnocenceEditor::shutdown()
{
	GameSystem::shutdown();
	m_objectStatus = objectStatus::SHUTDOWN;
}

const objectStatus & InnocenceEditor::getStatus() const
{
	return m_objectStatus;
}

std::string InnocenceEditor::getGameName() const
{
#ifdef INNO_PLATFORM_WIN64
	return std::string{ typeid(*this).name() }.substr(std::string{ typeid(*this).name() }.find("class"), std::string::npos);
#else
	return std::string("GameNameWIP");
#endif // INNO_PLATFORM_WIN64
}

std::vector<TransformComponent*>& InnocenceEditor::getTransformComponents()
{
	return m_transformComponents;
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

