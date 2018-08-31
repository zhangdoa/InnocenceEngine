#pragma once

#include "IObject.h"

using EntityID = unsigned long;

class IComponent : public IObject
{
public:
	virtual ~IComponent() {};

	virtual EntityID getParentEntity() const = 0;
	virtual void setParentEntity(EntityID parentEntity) = 0;
};