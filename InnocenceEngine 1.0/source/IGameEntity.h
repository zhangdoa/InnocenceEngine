#pragma once

#include "IBaseObject.h"
#include "LogManager.h"

#ifndef _I_GAME_ENTITY_H_
#define _I_GAME_ENTITY_H_

class Transform
{
public:
	Transform();
	~Transform();

	void update();
	void rotate(glm::vec3 axis, float angle);

	const glm::vec3& getPos();
	const glm::quat& getRot();
	const glm::vec3& getScale();

	void setPos(const glm::vec3& pos);
	void setRot(const glm::quat& rot);
	void setScale(const glm::vec3& scale);

	const glm::vec3& getOldPos();
	const glm::quat& getOldRot();
	const glm::vec3& getOldScale();

	glm::vec3 getForward() const;
	glm::vec3 getBackward() const;
	glm::vec3 getUp() const;
	glm::vec3 getDown() const;
	glm::vec3 getRight() const;
	glm::vec3 getLeft() const;

	glm::mat4 QuatToRotationMatrix(const glm::quat& quat) const;

private:

	glm::vec3 _pos;
	glm::quat _rot;
	glm::vec3 _scale;

	glm::vec3 _oldPos;
	glm::quat _oldRot;
	glm::vec3 _oldScale;
};

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
	glm::mat4 caclTransformation();
	glm::vec3 caclTransformedPos();
	glm::quat caclTransformedRot();

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


