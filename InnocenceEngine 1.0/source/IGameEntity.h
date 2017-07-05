#pragma once
#include "IBaseObject.h"
#include "LogManager.h"
#include "Math.h"

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

