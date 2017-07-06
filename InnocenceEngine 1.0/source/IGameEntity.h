#pragma once

#include "IBaseObject.h"
#include "LogManager.h"
#include "Math.h"

#ifndef _I_GAME_ENTITY_H_
#define _I_GAME_ENTITY_H_

class IGameEntity : public IBaseObject
{
public:
	IGameEntity();
	virtual ~IGameEntity();

	void addChildEntity(IGameEntity* childEntity);
	std::vector<IGameEntity*>& getChildrenEntity();
	IGameEntity* getParentEntity();
	void setParentEntity(IGameEntity* parentEntity);

	Transform* getTransform();
	bool hasTransformChanged();
	Mat4f caclTransformation();
	Vec3f caclTransformedPos();
	Quaternion caclTransformedRot();

private:
	std::vector<IGameEntity*> m_childGameEntity;
	IGameEntity* m_parentEntity;

	Transform m_transform;

};

#endif // !_I_GAME_ENTITY_H_

class Actor : public IGameEntity
{
public:
	Actor();
	~Actor();
private:
	void init() override;
	void update() override;
	void shutdown() override;
};

