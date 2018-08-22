#pragma once
#include "common/stdafx.h"
#include "ISystem.h"
#include "ILogSystem.h"
#include "common/ComponentHeaders.h"

extern ILogSystem* g_pLogSystem;

class IGameSystem : public ISystem
{
public:
	virtual ~IGameSystem() {};

	virtual std::vector<TransformComponent*>& getTransformComponents() = 0;
	virtual std::vector<VisibleComponent*>& getVisibleComponents() = 0;
	virtual std::vector<LightComponent*>& getLightComponents() = 0;
	virtual std::vector<CameraComponent*>& getCameraComponents() = 0;
	virtual std::vector<InputComponent*>& getInputComponents() = 0;

	virtual TransformComponent* getTransformComponent(EntityID parentEntity) = 0;
	virtual void addMeshData(VisibleComponent* visibleComponentconst, meshID & meshID) = 0;
	virtual void addTextureData(VisibleComponent* visibleComponentconst, const texturePair & texturePair) = 0;
	virtual void overwriteTextureData(VisibleComponent* visibleComponentconst, const texturePair & texturePair) = 0;
	virtual mat4 getProjectionMatrix(LightComponent* lightComponent, unsigned int cascadedLevel) = 0;
	virtual void registerButtonStatusCallback(InputComponent* inputComponent, button boundButton, std::function<void()>* function) = 0;
	virtual void registerMouseMovementCallback(InputComponent* inputComponent, int mouseCode, std::function<void(double)>* function) = 0;
	virtual std::string getGameName() const = 0;
	virtual bool needRender() = 0;
	virtual EntityID createEntityID() = 0;
};