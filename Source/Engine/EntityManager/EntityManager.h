#pragma once
#include "IEntityManager.h"

class InnoEntityManager : public IEntityManager
{
public:
	INNO_CLASS_CONCRETE_NON_COPYABLE(InnoEntityManager);

	bool Setup(ISystemConfig* systemConfig) override;
	bool Initialize() override;
	bool Update() override;
	bool Terminate() override;

	ObjectStatus GetStatus() override;

	InnoEntity* Spawn(bool serializable, ObjectLifespan objectLifespan, const char* entityName) override;
	bool Destroy(InnoEntity* entity) override;
	const std::vector<InnoEntity*>& GetEntities() override;
	std::optional<InnoEntity*> Find(const char* entityName) override;
};