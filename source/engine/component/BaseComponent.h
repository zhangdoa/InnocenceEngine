#pragma once
#include "interface/IComponent.h"
#include "entity/InnoMath.h"

class BaseComponent : public IComponent
{
public:
	BaseComponent() :
		m_parentEntity() {};
	virtual ~BaseComponent() {};

	EntityID getParentEntity() const override;
	void setParentEntity(EntityID parentEntity) override;

	const objectStatus& getStatus() const override;
	//void setStatus(objectStatus objectStatus) override;

protected:
	objectStatus m_objectStatus = objectStatus::SHUTDOWN;
	EntityID m_parentEntity;
};