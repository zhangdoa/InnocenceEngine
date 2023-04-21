#pragma once
#include "IEntityManager.h"

namespace Inno
{
	class EntityManager : public IEntityManager
	{
	public:
		INNO_CLASS_CONCRETE_NON_COPYABLE(EntityManager);

		bool Setup(ISystemConfig* systemConfig) override;
		bool Initialize() override;
		bool Update() override;
		bool Terminate() override;

		ObjectStatus GetStatus() override;

		Entity* Spawn(bool serializable, ObjectLifespan objectLifespan, const char* entityName) override;
		bool Destroy(Entity* entity) override;
		const std::vector<Entity*>& GetEntities() override;
		std::optional<Entity*> Find(const char* entityName) override;
	};
}