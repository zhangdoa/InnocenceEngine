#pragma once

#include "interface/IGame.h"
#include "component/BaseEntity.h"

class InnocenceEditor : public IGame
{
public:
	InnocenceEditor();
	~InnocenceEditor();

	void setup() override;
	void initialize() override;
	void update() override;
	void shutdown() override;
	const objectStatus& getStatus() const override;

	std::string getGameName() const override;

private:
	objectStatus m_objectStatus = objectStatus::SHUTDOWN;
	std::vector<CameraComponent*>& getCameraComponents() override;
	std::vector<InputComponent*>& getInputComponents() override;
	std::vector<LightComponent*>& getLightComponents() override;
	std::vector<VisibleComponent*>& getVisibleComponents() override;

	std::vector<CameraComponent*> m_cameraComponents;
	std::vector<InputComponent*> m_inputComponents;
	std::vector<LightComponent*> m_lightComponents;
	std::vector<VisibleComponent*> m_visibleComponents;
};

InnocenceEditor g_game;
IGame* g_pGame = &g_game;
