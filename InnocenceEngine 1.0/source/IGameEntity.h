#pragma once

#include "IBaseObject.h"
#include "LogManager.h"
#include "Math.h"

#ifndef _I_GAME_ENTITY_H_
#define _I_GAME_ENTITY_H_

class BaseComponent;
class BaseActor : public IBaseObject
{
public:
	BaseActor();
	virtual ~BaseActor();

	void addChildActor(BaseActor* childActor);
	std::vector<BaseActor*>& getChildrenActors();
	BaseActor* getParentActor();
	void setParentActor(BaseActor* parentActor);

	void addChildComponent(BaseComponent* childComponent);
	std::vector<BaseComponent*>& getChildrenComponents();

	Transform* getTransform();
	bool hasTransformChanged();
	Mat4f caclTransformation();
	Vec3f caclTransformedPos();
	Vec4f caclTransformedRot();

private:
	std::vector<BaseActor*> m_childActor;
	BaseActor* m_parentActor;
	std::vector<BaseComponent*> m_childComponent;
	Transform m_transform;

	void init() override;
	void update() override;
	void shutdown() override;

};

class BaseComponent : public IBaseObject
{
public:
	BaseComponent();
	virtual ~BaseComponent();

	BaseActor* getParentActor();
	void setParentActor(BaseActor* parentActor);
	Transform* getTransform();
private:
	BaseActor* m_parentActor;
};
#endif // !_I_GAME_ENTITY_H_


