#pragma once

#include "IBaseObject.h"

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

	glm::vec3& getPos();
	glm::quat& getRot();
	glm::vec3& getScale();

	void setPos(const glm::vec3& pos);
	void setRot(const glm::quat& rot);
	void setScale(const glm::vec3& scale);

	glm::vec3& getOldPos();
	glm::quat& getOldRot();
	glm::vec3& getOldScale();

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

	void setup() override;
	void initialize() override;
	void update() override;
	void shutdown() override;

	void addChildActor(BaseActor* childActor);
	const std::vector<BaseActor*>& getChildrenActors() const;
	BaseActor* getParentActor() const;
	void setParentActor(BaseActor* parentActor);

	void addChildComponent(BaseComponent* childComponent);
	const std::vector<BaseComponent*>& getChildrenComponents() const;

	Transform* getTransform();
	bool hasTransformChanged();

	glm::mat4 caclLocalPosMatrix();
	glm::mat4 caclLocalRotMatrix();
	glm::mat4 caclLocalScaleMatrix();

	glm::vec3 caclWorldPos();
	glm::quat caclWorldRot();
	glm::vec3 caclWorldScale();

	glm::mat4 caclWorldPosMatrix();
	glm::mat4 caclWorldRotMatrix();
	glm::mat4 caclWorldScaleMatrix();

	glm::mat4 caclTransformationMatrix();

private:
	std::vector<BaseActor*> m_childActors;
	BaseActor* m_parentActor;
	std::vector<BaseComponent*> m_childComponents;
	Transform m_transform;
};

class BaseComponent : public IBaseObject
{
public:
	BaseComponent();
	virtual ~BaseComponent();

	BaseActor* getParentActor() const;
	void setParentActor(BaseActor* parentActor);
	Transform* getTransform();
private:
	BaseActor* m_parentActor;
};
#endif // !_I_GAME_ENTITY_H_