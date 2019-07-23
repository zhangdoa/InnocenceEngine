#pragma once
#include "../Common/InnoType.h"
#include "../Common/InnoClassTemplate.h"
#include "../Common/InnoEntity.h"
#include "../Common/STL17.h"

class IEntityManager
{
public:
	INNO_CLASS_INTERFACE_NON_COPYABLE(IEntityManager);

	virtual bool Setup() = 0;
	virtual bool Initialize() = 0;
	virtual bool Simulate() = 0;
	virtual bool Terminate() = 0;
	virtual InnoEntity* Spawn(ObjectSource objectSource, ObjectUsage objectUsage, const char* entityName = "") = 0;
	virtual bool Destory(InnoEntity* entity) = 0;
	virtual std::optional<InnoEntity*> Find(const char* entityName) = 0;
	virtual const std::vector<InnoEntity*>& GetEntities() = 0;
	virtual uint32_t AcquireUUID() = 0;
};