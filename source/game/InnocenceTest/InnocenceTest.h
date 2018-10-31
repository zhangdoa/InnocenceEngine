#pragma once
#include "system/GameSystem.h"
#include "component/InnoAllocator.h"

#include "interface/ILogSystem.h"
#include "interface/IMemorySystem.h"

extern ILogSystem* g_pLogSystem;
extern IMemorySystem* g_pMemorySystem;

class InnocenceTest : public GameSystem
{
public:
	InnocenceTest() {};
	~InnocenceTest() {};

	void setup() override;
	void initialize() override;
	void update() override;
	void terminate() override;
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
	void testConcurrency();
	void testContainer();

	std::vector<TransformComponent*> m_transformComponents;
	std::vector<CameraComponent*> m_cameraComponents;
	std::vector<InputComponent*> m_inputComponents;
	std::vector<LightComponent*> m_lightComponents;
	std::vector<VisibleComponent*> m_visibleComponents;
};