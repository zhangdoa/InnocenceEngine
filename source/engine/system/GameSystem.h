#pragma once
#include "../common/ComponentHeaders.h"

namespace InnoGameSystem
{
	void setup();
	void addComponentsToMap();
	void initialize();
	void update();
	void shutdown();

	std::vector<TransformComponent*>& getTransformComponents();
	std::vector<VisibleComponent*>& getVisibleComponents();
	std::vector<LightComponent*>& getLightComponents();
	std::vector<CameraComponent*>& getCameraComponents();
	std::vector<InputComponent*>& getInputComponents();
	std::string getGameName();

	template<class TComponent>
	auto getComponent(EntityID parentEntity) -> TComponent*;

	void addMeshData(VisibleComponent* visibleComponentconst, meshID & meshID);
	void addTextureData(VisibleComponent* visibleComponentconst, const texturePair & texturePair);
	void overwriteTextureData(VisibleComponent* visibleComponentconst, const texturePair & texturePair);
	mat4 getProjectionMatrix(LightComponent* lightComponent, unsigned int cascadedLevel);
	void registerButtonStatusCallback(InputComponent* inputComponent, button boundButton, std::function<void()>* function);
	void registerMouseMovementCallback(InputComponent* inputComponent, int mouseCode, std::function<void(double)>* function);
	bool needRender();
	EntityID createEntityID();

	objectStatus m_GameSystemStatus = objectStatus::SHUTDOWN;
};
