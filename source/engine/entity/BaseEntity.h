#pragma once
#include "interface/IEntity.h"

class BaseEntity : public IEntity
{
public:
	BaseEntity();
	virtual ~BaseEntity();

	void setup() override;
	void initialize() override;
	void update() override;
	void shutdown() override;

	const EntityID& getEntityID() const override;
	const objectStatus& getStatus() const override;

	IEntity* getParentEntity() const override;
	void setParentEntity(IEntity* parentEntity) override;

	void addChildEntity(IEntity* childEntity) override;
	const std::vector<IEntity*>& getChildrenEntitys() const override;

	void addChildComponent(IComponent* childComponent) override;
	const std::vector<IComponent*>& getChildrenComponents() const override;

	Transform* getTransform() override;
	bool hasTransformChanged() override;

	mat4 caclLocalPosMatrix() override;
	mat4 caclLocalRotMatrix() override;
	mat4 caclLocalScaleMatrix() override;

	vec3 caclWorldPos() override;
	quat caclWorldRot() override;
	vec3 caclWorldScale() override;

	mat4 caclWorldPosMatrix() override;
	mat4 caclWorldRotMatrix() override;
	mat4 caclWorldScaleMatrix() override;

	mat4 caclTransformationMatrix() override;

private:
	objectStatus m_objectStatus = objectStatus::SHUTDOWN;
	EntityID m_entityID = 0;

	IEntity* m_parentEntity;
	std::vector<IEntity*> m_childEntitys;
	std::vector<IComponent*> m_childComponents;
	Transform m_transform;
};