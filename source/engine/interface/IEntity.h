#pragma once
#include "common/stdafx.h"
#include "interface/IObject.h"
#include "interface/IComponent.h"

typedef unsigned long int EntityID;

class IComponent;
class IEntity : public IObject
{
public:
	virtual ~IEntity() {};

	virtual const EntityID& getEntityID() const = 0;

	virtual void addChildComponent(IComponent* childComponent) = 0;
	virtual const std::vector<IComponent*>& getChildrenComponents() const = 0;
};

