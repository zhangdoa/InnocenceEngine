#pragma once
#include "FixedSizeString.h"
#include "Config.h"
#include "EntityID.h"
#include "Enum.h"

// INNO_ENUM declares the enum in Inno::Enum::, registers it with the traits
// registry so Log() prints the name, and publishes a `using EnumName = ...`
// alias into Inno:: so callers can write `Inno::ObjectStatus::Activated` or
// plain `ObjectStatus::Activated` inside `namespace Inno` — same ergonomics
// as the raw enum while being fully loggable.
INNO_ENUM(ObjectStatus,   Invalid, Created, Activated, Suspended, Terminated)
INNO_ENUM(ObjectLifespan, Invalid, Persistence, Scene, Frame)

namespace Inno
{

	using ObjectName = FixedSizeString<128>;

	class Object
	{
	public:
		Object() = default;
		~Object() = default;

		uint64_t m_UUID = 0;
		bool m_Serializable = false;
		ObjectLifespan m_ObjectLifespan = ObjectLifespan::Invalid;
		ObjectStatus m_ObjectStatus = ObjectStatus::Invalid;

		ObjectName m_InstanceName;
	};

	const uint32_t MaxComponentType = 512;

	class Entity : public Object
	{
	public:
		Entity() = default;
		~Entity() = default;

		static uint32_t GetTypeID() { return 0; }
        static const char* GetTypeName() { return "Entity"; }
	};

	class Component : public Object
	{
	public:
		Component() = default;
		~Component() = default;

		EntityID m_Owner = INVALID_ENTITY;
	};
}
