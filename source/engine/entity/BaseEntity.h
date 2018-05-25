#pragma once
#include "interface/IEntity.h"
#include "entity/InnoMath.h"

class BaseEntity : public IEntity
{
public:
	BaseEntity();
	virtual ~BaseEntity();

	void setup() override;
	void initialize() override;
	void shutdown() override;

	const EntityID& getEntityID() const override;
	const objectStatus& getStatus() const override;

	void addChildComponent(IComponent* childComponent) override;
	const std::vector<IComponent*>& getChildrenComponents() const override;

private:
	objectStatus m_objectStatus = objectStatus::SHUTDOWN;
	EntityID m_entityID = 0;

	std::vector<IComponent*> m_childComponents;
};