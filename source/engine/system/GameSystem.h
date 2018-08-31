#pragma once
#include "../interface/IGameSystem.h"
#include "../interface/ITimeSystem.h"
#include "../interface/IMemorySystem.h"

#include "../common/ComponentHeaders.h"

extern ITimeSystem* g_pTimeSystem;
extern IMemorySystem* g_pMemorySystem;

class GameSystem : public IGameSystem
{
public:
	GameSystem() {};
	~GameSystem() {};

	void setup() override;
	void addComponentsToMap();
	void initialize() override;
	void update() override;
	void shutdown() override;

	std::vector<TransformComponent*>& getTransformComponents() override;
	std::vector<VisibleComponent*>& getVisibleComponents() override;
	std::vector<LightComponent*>& getLightComponents() override;
	std::vector<CameraComponent*>& getCameraComponents() override;
	std::vector<InputComponent*>& getInputComponents() override;

	TransformComponent* getTransformComponent(EntityID parentEntity) override;
	void addMeshData(VisibleComponent* visibleComponentconst, meshID & meshID) override;
	void addTextureData(VisibleComponent* visibleComponentconst, const texturePair & texturePair) override;
	void overwriteTextureData(VisibleComponent* visibleComponentconst, const texturePair & texturePair) override;
	mat4 getProjectionMatrix(LightComponent* lightComponent, unsigned int cascadedLevel) override;
	void registerButtonStatusCallback(InputComponent* inputComponent, button boundButton, std::function<void()>* function) override;
	void registerMouseMovementCallback(InputComponent* inputComponent, int mouseCode, std::function<void(double)>* function) override;
	bool needRender() override;
	EntityID createEntityID() override;

	const objectStatus& getStatus() const override;

protected:
	// the SOA here
	std::vector<TransformComponent*> m_transformComponents;
	std::vector<VisibleComponent*> m_visibleComponents;
	std::vector<LightComponent*> m_lightComponents;
	std::vector<CameraComponent*> m_cameraComponents;
	std::vector<InputComponent*> m_inputComponents;

private:
	objectStatus m_objectStatus = objectStatus::SHUTDOWN;

	std::unordered_map<EntityID, TransformComponent*> m_TransformComponentsMap;
	std::unordered_multimap<EntityID, VisibleComponent*> m_VisibleComponentsMap;
	std::unordered_multimap<EntityID, LightComponent*> m_LightComponentsMap;
	std::unordered_multimap<EntityID, CameraComponent*> m_CameraComponentsMap;
	std::unordered_multimap<EntityID, InputComponent*> m_InputComponentsMap;

	bool m_needRender = true;
};
