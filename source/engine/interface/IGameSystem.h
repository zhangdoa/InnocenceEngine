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

	virtual std::string getGameName() const = 0;
	virtual bool needRender() = 0;
	virtual EntityID createEntityID() = 0;
};