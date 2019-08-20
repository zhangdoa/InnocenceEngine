#pragma once
#include "IEntityManager.h"

class InnoEntityManager : public IEntityManager
{
public:
	INNO_CLASS_CONCRETE_NON_COPYABLE(InnoEntityManager);

	bool Setup() override;
	bool Initialize() override;
	bool Simulate() override;
	bool Terminate() override;
	InnoEntity* Spawn(ObjectSource objectSource, ObjectUsage objectUsage, const char* entityName) override;
	bool Destroy(InnoEntity* entity) override;
	const std::vector<InnoEntity*>& GetEntities() override;
	std::optional<InnoEntity*> Find(const char* entityName) override;
	uint32_t AcquireUUID() override;
};