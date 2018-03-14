#pragma once
#include "common/stdafx.h"
#include "interface/IObject.hpp"
#include "interface/IComponent.h"
#include "entity/InnoMath.h"

typedef unsigned long int EntityID;
class IComponent;
class IEntity : public IObject
{
public:
	virtual ~IEntity() {};

	virtual const EntityID& getEntityID() const = 0;

	virtual IEntity* getParentEntity() const = 0;
	virtual void setParentEntity(IEntity* parentEntity) = 0;

	virtual void addChildEntity(IEntity* childEntity) = 0;
	virtual const std::vector<IEntity*>& getChildrenEntitys() const = 0;

	virtual void addChildComponent(IComponent* childComponent) = 0;
	virtual const std::vector<IComponent*>& getChildrenComponents() const = 0;

	virtual Transform* getTransform() = 0;
	virtual bool hasTransformChanged() = 0;

	virtual mat4 caclLocalPosMatrix() = 0;
	virtual mat4 caclLocalRotMatrix() = 0;
	virtual mat4 caclLocalScaleMatrix() = 0;

	virtual vec3 caclWorldPos() = 0;
	virtual quat caclWorldRot() = 0;
	virtual vec3 caclWorldScale() = 0;

	virtual mat4 caclWorldPosMatrix() = 0;
	virtual mat4 caclWorldRotMatrix() = 0;
	virtual mat4 caclWorldScaleMatrix() = 0;

	virtual mat4 caclTransformationMatrix() = 0;
};

