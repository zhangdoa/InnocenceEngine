#pragma once
#include "../Interface/ISystem.h"
#include "../Common/STL17.h"

class IEntityManager : public ISystem
{
public:
	INNO_CLASS_INTERFACE_NON_COPYABLE(IEntityManager);

	virtual InnoEntity* Spawn(bool serializable, ObjectLifespan objectLifespan, const char* entityName = "") = 0;
	virtual bool Destroy(InnoEntity* entity) = 0;
	virtual std::optional<InnoEntity*> Find(const char* entityName) = 0;
	virtual const std::vector<InnoEntity*>& GetEntities() = 0;
};