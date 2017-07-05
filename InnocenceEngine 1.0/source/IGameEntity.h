#pragma once
#include "IBaseObject.h"
#include "Math.h"

class IGameEntity : public IBaseObject
{
public:
	IGameEntity();
	virtual ~IGameEntity();
	
	void addChildEntity(IGameEntity* childEntity);

	IGameEntity* getParentEntity();
	void setParentEntity(IGameEntity* parentEntity);

	Transform* getTransform();
	bool hasTransformChanged();
	Mat4f caclTransformation();
	Vec3f caclTransformedPos();
	Quaternion caclTransformedRot();

private:
	std::vector<std::auto_ptr<IGameEntity>> m_childGameEntity;
	IGameEntity* m_parentEntity;

	Transform m_transform;

};

class Actor : public IGameEntity
{
public:
	Actor();
	~Actor();
};

