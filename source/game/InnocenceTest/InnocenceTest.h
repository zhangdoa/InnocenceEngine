#pragma once
#include "interface/IGame.h"
#include "component/BaseEntity.h"

#include "interface/ILogSystem.h"
#include "interface/IMemorySystem.h"

extern ILogSystem* g_pLogSystem;
extern IMemorySystem* g_pMemorySystem;

class InnocenceTest : public IGame
{
public:
	InnocenceTest();
	~InnocenceTest();

	void setup() override;
	void initialize() override;
	void update() override;
	void shutdown() override;
	const objectStatus& getStatus() const override;

	std::string getGameName() const override;
	std::vector<TransformComponent*>& getTransformComponents() override;
	std::vector<CameraComponent*>& getCameraComponents() override;
	std::vector<InputComponent*>& getInputComponents() override;
	std::vector<LightComponent*>& getLightComponents() override;
	std::vector<VisibleComponent*>& getVisibleComponents() override;

private:
	objectStatus m_objectStatus = objectStatus::SHUTDOWN;

	void testMath();
	void testMemory();

	BaseEntity m_rootEntity;

	std::vector<TransformComponent*> m_transformComponents;
	std::vector<CameraComponent*> m_cameraComponents;
	std::vector<InputComponent*> m_inputComponents;
	std::vector<LightComponent*> m_lightComponents;
	std::vector<VisibleComponent*> m_visibleComponents;
};

InnocenceTest g_game;
IGame* g_pGame = &g_game;