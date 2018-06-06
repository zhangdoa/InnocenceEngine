#pragma once
#include "interface/IComponent.h"
#include "entity/InnoMath.h"

class BaseComponent : public IComponent
{
public:
	BaseComponent() :
		m_parentEntity(nullptr) {};
	virtual ~BaseComponent() {};

	IEntity* getParentEntity() const override;
	void setParentEntity(IEntity* parentEntity) override;

	const objectStatus& getStatus() const override;

protected:
	objectStatus m_objectStatus = objectStatus::SHUTDOWN;

private:
	IEntity* m_parentEntity;
};