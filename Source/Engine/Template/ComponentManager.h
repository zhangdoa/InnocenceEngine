#pragma once
#include "../Common/InnoObject.h"
#include "ObjectPool.h"
#include "../Core/InnoRandomizer.h"

namespace Inno
{
	class IComponentFactory
	{
	public:
		virtual ~IComponentFactory() = default;
		virtual bool CleanUp(ObjectLifespan objectLifespan) = 0;
	};

	template<typename T>
	class TComponentFactory : public IComponentFactory
	{
	public:
		TComponentFactory() = delete;

		explicit TComponentFactory(uint32_t maxComponentCount)
		{
			m_MaxComponentCount = maxComponentCount;
			m_ComponentPool = TObjectPool<T>::Create(m_MaxComponentCount);
			m_ComponentPointers.reserve(m_MaxComponentCount);
			m_ComponentLUT.reserve(m_MaxComponentCount);
		}

		~TComponentFactory() = default;

		bool CleanUp(ObjectLifespan objectLifespan)
		{
			std::vector<T*> l_componentPtrs = m_ComponentPointers.getRawData();
			for (auto i : l_componentPtrs)
			{
				if (i->m_ObjectLifespan == objectLifespan)
				{
					Destroy(i);
				}
			}
			return true;
		}

		T* Spawn(const InnoEntity* owner, bool serializable, ObjectLifespan objectLifespan)
		{
			auto l_Component = static_cast<TObjectPool<T>*>(m_ComponentPool)->Spawn();
			if (l_Component)
			{
				l_Component->m_UUID = InnoRandomizer::GenerateUUID();
				l_Component->m_ObjectStatus = ObjectStatus::Created;
				l_Component->m_Serializable = serializable;
				l_Component->m_ObjectLifespan = objectLifespan;
				auto l_owner = const_cast<InnoEntity*>(owner);
				l_Component->m_Owner = l_owner;
				auto l_componentIndex = m_CurrentComponentIndex;
#ifdef INNO_DEBUG
				auto l_instanceName = ObjectName((std::string(owner->m_InstanceName.c_str()) + "." + std::string(T::GetTypeName()) + "_" + std::to_string(l_componentIndex) + "/").c_str());
				l_Component->m_InstanceName = l_instanceName;
#endif
				m_ComponentPointers.emplace_back(l_Component);
				m_ComponentLUT.emplace(l_owner, l_Component);
				l_Component->m_ObjectStatus = ObjectStatus::Activated;
				m_CurrentComponentIndex++;

				return l_Component;
			}
			else
			{
				return nullptr;
			}
		}

		void Destroy(T* component)
		{
			component->m_ObjectStatus = ObjectStatus::Terminated;
			m_ComponentPointers.eraseByValue(component);
			m_ComponentLUT.erase(component->m_Owner);
			static_cast<TObjectPool<T>*>(m_ComponentPool)->Destroy(component);
		}

		T* Find(const InnoEntity* owner)
		{
			auto l_owner = const_cast<InnoEntity*>(owner);

			auto l_result = m_ComponentLUT.find(l_owner);
			if (l_result != m_ComponentLUT.end())
			{
				return l_result->second;
			}
			else
			{
				InnoLogger::Log(LogLevel::Error, T::GetTypeName(), "ComponentFactory: Can't find ", T::GetTypeName(), " by Entity: ", l_owner->m_InstanceName.c_str(), "!");
				return nullptr;
			}
		}

		T* Get(std::size_t index)
		{
			if (index >= m_ComponentPointers.size())
			{
				return nullptr;
			}
			return m_ComponentPointers[index];
		}

		const std::vector<T*>& GetAll()
		{
			return m_ComponentPointers.getRawData();
		}

	private:
		uint32_t m_MaxComponentCount = 0;
		uint32_t m_CurrentComponentIndex = 0;
		IObjectPool* m_ComponentPool;
		ThreadSafeVector<T*> m_ComponentPointers;
		ThreadSafeUnorderedMap<InnoEntity*, T*> m_ComponentLUT;
	};

	class ComponentManager
	{
	public:
		template<typename T>
		bool RegisterType(uint32_t maxComponentCount, IComponentSystem* componentSystem)
		{
			auto& l_TypeInfo = typeid(T);
			auto l_typeHashCode = l_TypeInfo.hash_code();

			if (m_ComponentTypeIndexLUT.find(l_typeHashCode) != m_ComponentTypeIndexLUT.end())
			{
				InnoLogger::Log(LogLevel::Error, "Component type: ", l_TypeInfo.name(), " has been registered!");
				return false;
			}

			m_ComponentTypeIndexLUT.emplace(l_typeHashCode, m_ComponentTypeIndexTracker);
			m_ComponentFactories.emplace(l_typeHashCode, std::make_shared<TComponentFactory<T>>(maxComponentCount));
			m_ComponentSystems.emplace(l_typeHashCode, componentSystem);

			++m_ComponentTypeIndexTracker;

			return true;
		}

		template<typename T>
		uint32_t GetTypeIndex()
		{
			auto& l_TypeInfo = typeid(T);
			auto l_typeHashCode = l_TypeInfo.hash_code();

			if (m_ComponentTypeIndexLUT.find(l_typeHashCode) != m_ComponentTypeIndexLUT.end())
			{
				return m_ComponentTypeIndexLUT[l_typeHashCode];
			}

			InnoLogger::Log(LogLevel::Error, "Component type: ", l_TypeInfo.name(), " is not registered!");
			return -1;
		}

		template<typename T>
		IComponentSystem* GetComponentSystem()
		{
			const std::type_info& l_TypeInfo = typeid(T);
			auto l_typeHashCode = l_TypeInfo.hash_code();

			if (m_ComponentTypeIndexLUT.find(l_typeHashCode) != m_ComponentTypeIndexLUT.end())
			{
				return m_ComponentSystems[l_typeHashCode];
			}

			InnoLogger::Log(LogLevel::Error, "Component type: ", l_TypeInfo.name(), " is not registered!");

			return nullptr;
		}

		template<typename T>
		T* Spawn(const InnoEntity* owner, bool serializable, ObjectLifespan objectLifespan)
		{
			return GetComponentFactory<T>()->Spawn(owner, serializable, objectLifespan);
		}

		template<typename T>
		void Destroy(T* component)
		{
			GetComponentFactory<T>()->Destroy(component);
		}

		template<typename T>
		T* Find(InnoEntity* entity)
		{
			return GetComponentFactory<T>()->Find(entity);
		}

		template<typename T>
		T* Get(std::size_t index)
		{
			return GetComponentFactory<T>()->Get(index);
		}

		template<typename T>
		const std::vector<T*>& GetAll()
		{
			return GetComponentFactory<T>()->GetAll();
		}

		bool CleanUp(ObjectLifespan objectLifespan)
		{
			for (auto& i : m_ComponentFactories)
			{
				i.second->CleanUp(objectLifespan);
			}
			return true;
		}

	private:
		std::unordered_map<size_t, uint32_t> m_ComponentTypeIndexLUT;
		std::unordered_map<size_t, std::shared_ptr<IComponentFactory>> m_ComponentFactories;
		std::unordered_map<size_t, IComponentSystem*> m_ComponentSystems;

		std::atomic<uint32_t> m_ComponentTypeIndexTracker = 0;

		template<typename T>
		std::shared_ptr<TComponentFactory<T>> GetComponentFactory()
		{
			const std::type_info& l_TypeInfo = typeid(T);
			auto l_typeHashCode = l_TypeInfo.hash_code();

			if (m_ComponentTypeIndexLUT.find(l_typeHashCode) != m_ComponentTypeIndexLUT.end())
			{
				return std::static_pointer_cast<TComponentFactory<T>>(m_ComponentFactories[l_typeHashCode]);
			}

			InnoLogger::Log(LogLevel::Error, "Component type: ", l_TypeInfo.name(), " is not registered!");

			return nullptr;
		}
	};
}
