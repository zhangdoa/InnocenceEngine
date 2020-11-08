#pragma once
#include "InnoType.h"

namespace Inno
{
	class InnoObject
	{
	public:
		InnoObject() = default;
		~InnoObject() = default;

		uint64_t m_UUID = 0;
		bool m_Serializable = false;
		ObjectLifespan m_ObjectLifespan = ObjectLifespan::Invalid;
		ObjectStatus m_ObjectStatus = ObjectStatus::Invalid;

#ifdef INNO_DEBUG
		ObjectName m_InstanceName;
#endif
	};

	class InnoEntity : public InnoObject
	{
	public:
		InnoEntity() = default;
		~InnoEntity() = default;

		static uint32_t GetTypeID() { return 0; }
        static const char* GetTypeName() { return "InnoEntity"; }

		std::bitset<MaxComponentType> m_ComponentsMask;
	};

	class InnoComponent : public InnoObject
	{
	public:
		InnoComponent() = default;
		~InnoComponent() = default;

		InnoEntity* m_Owner = 0;
	};
}
