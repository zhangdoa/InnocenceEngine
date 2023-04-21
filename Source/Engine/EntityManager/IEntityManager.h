#pragma once
#include "../Interface/ISystem.h"
#include "../Common/STL17.h"

namespace Inno
{
	class IEntityManager : public ISystem
	{
	public:
		INNO_CLASS_INTERFACE_NON_COPYABLE(IEntityManager);

		virtual Entity* Spawn(bool serializable, ObjectLifespan objectLifespan, const char* entityName = "") = 0;
		virtual bool Destroy(Entity* entity) = 0;
		virtual std::optional<Entity*> Find(const char* entityName) = 0;
		virtual const std::vector<Entity*>& GetEntities() = 0;
	};
}