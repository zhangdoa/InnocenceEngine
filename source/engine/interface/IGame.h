#pragma once
#include "common/stdafx.h"
#include "IObject.hpp"
#include "entity/ComponentHeaders.h"

class IGame : public IObject
{
public:
	virtual ~IGame() {};
	virtual std::string getGameName() const = 0;
	virtual std::vector<CameraComponent*>& getCameraComponents() = 0;
	virtual std::vector<InputComponent*>& getInputComponents() = 0;
	virtual std::vector<LightComponent*>& getLightComponents() = 0;
	virtual std::vector<VisibleComponent*>& getVisibleComponents() = 0;
};