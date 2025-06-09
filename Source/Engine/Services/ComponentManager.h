#pragma once
#include "../Common/IOService.h"
#include "../Common/Object.h"
#include "../Common/ObjectPool.h"
#include "../Common/ThreadSafeVector.h"
#include "../Common/ThreadSafeUnorderedMap.h"
#include "../Common/Randomizer.h"
#include "../Interface/ISystem.h"
#include "../Services/AssetService.h"
#include "../Engine.h"

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
			if (m_ComponentPointers.empty())
				return true;

			Log(Verbose, "Remove ", T::GetTypeName(), " by ObjectLifespan: ", std::to_string(static_cast<int>(objectLifespan)).c_str(), "!");
			m_ComponentPointers.for_each([this, objectLifespan](T*& component)
				{
					if (component->m_ObjectLifespan == objectLifespan)
					{
						DestroyComponent(component);
						component = nullptr;
					}
				});

			m_ComponentPointers.erase_if([](T* component) { return component == nullptr; });

			Log(Verbose, "Remove ", T::GetTypeName(), " by ObjectLifespan: ", std::to_string(static_cast<int>(objectLifespan)).c_str(), " has been done!");
			return true;
		}

		T* Spawn(const Entity* owner, bool serializable, ObjectLifespan objectLifespan)
		{
			if (!owner)
			{
				Log(Error, T::GetTypeName(), "ComponentFactory: Can't spawn ", T::GetTypeName(), " by Entity: nullptr!");
				return nullptr;
			}

			auto l_Component = static_cast<TObjectPool<T>*>(m_ComponentPool)->Spawn();
			if (!l_Component)
			{
				Log(Error, T::GetTypeName(), "ComponentFactory: Can't spawn ", T::GetTypeName(), "!");
				return nullptr;
			}

			l_Component->m_UUID = Randomizer::GenerateUUID();
			l_Component->m_ObjectStatus = ObjectStatus::Created;
			l_Component->m_Serializable = serializable;
			l_Component->m_ObjectLifespan = objectLifespan;
			auto l_owner = const_cast<Entity*>(owner);
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

		void Destroy(T* component)
		{
			if (!DestroyComponent(component))
			{
				return;
			}

			m_ComponentPointers.eraseByValue(component);
		}

		T* Find(const Entity* owner)
		{
			if (!owner)
			{
				Log(Error, T::GetTypeName(), "ComponentFactory: Can't find ", T::GetTypeName(), " by Entity: nullptr!");
				return nullptr;
			}

			auto l_owner = const_cast<Entity*>(owner);

			auto l_result = m_ComponentLUT.find(l_owner);
			if (l_result != m_ComponentLUT.end())
			{
				return l_result->second;
			}
			else
			{
				Log(Error, T::GetTypeName(), "ComponentFactory: Can't find ", T::GetTypeName(), " by Entity: ", l_owner->m_InstanceName.c_str(), "!");
				return nullptr;
			}
		}

		T* Get(std::size_t index)
		{
			if (index >= m_ComponentPointers.size())
			{
				Log(Error, T::GetTypeName(), "ComponentFactory: Can't get ", T::GetTypeName(), " by index: ", std::to_string(index).c_str(), "!");
				return nullptr;
			}

			return m_ComponentPointers[index];
		}

		const std::vector<T*>& GetAll()
		{
			return m_ComponentPointers.getRawData();
		}

		uint64_t Load(const char* componentName, const Entity* entity)
		{
			auto fileName = componentName + std::string(".") + std::string(T::GetTypeName()) + ".inno";			
			uint64_t l_result = 0;
			if (!FindLoaded(fileName.c_str(), l_result))
			{
				std::unique_lock<std::shared_mutex> lock{ m_Mutex };
				
				auto l_componentPtr = Spawn<T>(entity, true, ObjectLifespan::Scene);
				auto& component = *(l_componentPtr);
				if (AssetService::Load(fileName.c_str(), component))
				{
					l_result = component.m_UUID;
					RecordLoaded(fileName, l_result);
					component.m_ObjectStatus = ObjectStatus::Activated;
				}
			}

			return l_result;
		}

	private:
		bool DestroyComponent(T* component)
		{
			if (!component)
			{
				Log(Error, T::GetTypeName(), "ComponentFactory: Can't destroy ", T::GetTypeName(), " by nullptr!");
				return false;
			}

			component->m_ObjectStatus = ObjectStatus::Terminated;
			m_ComponentLUT.erase(component->m_Owner);
			static_cast<TObjectPool<T>*>(m_ComponentPool)->Destroy(component);

			return true;
		}

	private:
		bool RecordLoaded(const char* fileName, uint64_t value)
		{
			m_LoadedComponents.emplace(fileName, value);
			return true;
		}

		bool FindLoaded(const char* fileName, uint64_t& value)
		{
			auto l_loaded = m_LoadedComponents.find(fileName);
			if (l_loaded != m_LoadedComponents.end())
			{
				value = l_loaded->second;

				return true;
			}
			else
			{
				Log(Verbose, "", fileName, " is not loaded yet.");

				return false;
			}
		}

		std::shared_mutex m_Mutex;
		uint32_t m_MaxComponentCount = 0;
		uint32_t m_CurrentComponentIndex = 0;

		TObjectPool<T>* m_ComponentPool;
		ThreadSafeVector<T*> m_ComponentPointers;
		ThreadSafeUnorderedMap<Entity*, T*> m_ComponentLUT;
		std::unordered_map<std::string, uint64_t> m_LoadedComponents;
	};

	class ComponentManager
	{
	public:
		template<typename T>
		bool RegisterType(uint32_t maxComponentCount, ISystem* componentSystem)
		{
			auto& l_TypeInfo = typeid(T);
			auto l_typeHashCode = l_TypeInfo.hash_code();

			if (m_ComponentTypeIndexLUT.find(l_typeHashCode) != m_ComponentTypeIndexLUT.end())
			{
				Log(Error, "", l_TypeInfo.name(), " has been registered!");
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

			Log(Error, "", l_TypeInfo.name(), " is not registered!");
			return -1;
		}

		template<typename T>
		ISystem* GetComponentSystem()
		{
			const std::type_info& l_TypeInfo = typeid(T);
			auto l_typeHashCode = l_TypeInfo.hash_code();

			if (m_ComponentTypeIndexLUT.find(l_typeHashCode) != m_ComponentTypeIndexLUT.end())
			{
				return m_ComponentSystems[l_typeHashCode];
			}

			Log(Error, "", l_TypeInfo.name(), " is not registered!");

			return nullptr;
		}

		template<typename T>
		T* Spawn(const Entity* owner, bool serializable, ObjectLifespan objectLifespan)
		{
			return GetComponentFactory<T>()->Spawn(owner, serializable, objectLifespan);
		}

		template<typename T>
		void Destroy(T* component)
		{
			GetComponentFactory<T>()->Destroy(component);
		}

		template<typename T>
		T* Find(Entity* entity)
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
			Log(Verbose, "ComponentManager is cleaning up...");
			for (auto& i : m_ComponentFactories)
			{
				i.second->CleanUp(objectLifespan);
			}
			Log(Success, "ComponentManager has been cleaned up.");
			return true;
		}

		template<typename T>
		uint64_t Load(const char* componentName, const Entity* entity)
		{
			return GetComponentFactory<T>()->Load(componentName, entity);
		}

	private:
		std::unordered_map<size_t, uint32_t> m_ComponentTypeIndexLUT;
		std::unordered_map<size_t, std::shared_ptr<IComponentFactory>> m_ComponentFactories;
		std::unordered_map<size_t, ISystem*> m_ComponentSystems;

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

			Log(Error, l_TypeInfo.name(), " is not registered!");

			return nullptr;
		}
	};
}
