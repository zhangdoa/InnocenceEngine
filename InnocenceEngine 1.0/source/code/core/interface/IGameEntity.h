#pragma once

#include "IBaseObject.h"
#include "../manager/LogManager.h"

#ifndef _I_GAME_ENTITY_H_
#define _I_GAME_ENTITY_H_

class Transform
{
public:
	Transform();
	~Transform();

	enum direction { FORWARD, BACKWARD, UP, DOWN, RIGHT, LEFT };
	void update();
	void rotate(const glm::vec3& axis, float angle);

	const glm::vec3& getPos() const;
	const glm::quat& getRot() const;
	const glm::vec3& getScale() const;

	void setPos(const glm::vec3& pos);
	void setRot(const glm::quat& rot);
	void setScale(const glm::vec3& scale);

	const glm::vec3& getOldPos() const;
	const glm::quat& getOldRot() const;
	const glm::vec3& getOldScale() const;

	glm::vec3 getDirection(direction direction) const;

private:

	glm::vec3 m_pos;
	glm::quat m_rot;
	glm::vec3 m_scale;

	glm::vec3 m_oldPos;
	glm::quat m_oldRot;
	glm::vec3 m_oldScale;
};

class BaseComponent;
class BaseActor : public IBaseObject
{
public:
	BaseActor();
	virtual ~BaseActor();

	void addChildActor(BaseActor* childActor);
	const std::vector<BaseActor*>& getChildrenActors() const;
	const BaseActor& getParentActor() const;
	void setParentActor(BaseActor* parentActor);

	void addChildComponent(BaseComponent* childComponent);
	const std::vector<BaseComponent*>& getChildrenComponents() const;

	Transform* getTransform();
	bool hasTransformChanged() const;
	glm::mat4 caclTransformation() const;
	glm::vec3 caclTransformedPos() const;
	glm::quat caclTransformedRot() const;

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

	const BaseActor& getParentActor() const;
	void setParentActor(BaseActor* parentActor);
	Transform*getTransform();
private:
	BaseActor* m_parentActor;
};
#endif // !_I_GAME_ENTITY_H_