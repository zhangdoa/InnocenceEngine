#pragma once
#include "../Common/STL17.h"
#include "../Interface/ISystem.h"

namespace Inno
{
	class EntityManager : public ISystem
	{
	public:
		INNO_CLASS_CONCRETE_NON_COPYABLE(EntityManager);

		bool Setup(ISystemConfig* systemConfig) override;
		bool Initialize() override;
		bool Update() override;
		bool Terminate() override;

		ObjectStatus GetStatus() override;

		Entity* Spawn(bool serializable, ObjectLifespan objectLifespan, const char* entityName);
		bool Destroy(Entity* entity);
		const std::vector<Entity*>& GetEntities();
		std::optional<Entity*> Find(const char* entityName);
	};
}