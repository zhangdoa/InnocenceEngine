#pragma once
#include "common/stdafx.h"
#include "interface/IObject.hpp"
#include "interface/IEntity.h"

class IEntity;
class IComponent : public IObject
{
public:
	virtual ~IComponent() {};

	virtual IEntity* getParentEntity() const = 0;
	virtual void setParentEntity(IEntity* parentEntity) = 0;
};