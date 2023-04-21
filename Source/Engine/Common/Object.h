#pragma once
#include "Type.h"

namespace Inno
{
	class Object
	{
	public:
		Object() = default;
		~Object() = default;

		uint64_t m_UUID = 0;
		bool m_Serializable = false;
		ObjectLifespan m_ObjectLifespan = ObjectLifespan::Invalid;
		ObjectStatus m_ObjectStatus = ObjectStatus::Invalid;

#ifdef INNO_DEBUG
		ObjectName m_InstanceName;
#endif
	};

	class Entity : public Object
	{
	public:
		Entity() = default;
		~Entity() = default;

		static uint32_t GetTypeID() { return 0; }
        static const char* GetTypeName() { return "Entity"; }

		std::bitset<MaxComponentType> m_ComponentsMask;
	};

	class Component : public Object
	{
	public:
		Component() = default;
		~Component() = default;

		Entity* m_Owner = 0;
	};
}
