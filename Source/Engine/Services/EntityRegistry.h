#pragma once

#include "../Common/STL14.h"
#include "../Common/STL17.h"
#include "../Common/EntityID.h"
#include "../Common/ComponentStorage.h"
#include "../Common/Object.h"
#include "../Common/LogService.h"
#include "../Engine.h"
#include "../Common/ClassTemplate.h"
#include "../Interface/IService.h"

namespace Inno
{
    class EntityRegistry : public IService
    {
    public:
        INNO_CLASS_CONCRETE_NON_COPYABLE(EntityRegistry);

        bool Setup(IServiceConfig* Config) override;
        bool Initialize() override;
        bool Update() override;
        bool Terminate() override;
        ObjectStatus GetStatus() override;

        // Entity lifecycle
        EntityID    Spawn(ObjectLifespan Lifespan, const char* Name = nullptr);
        void        Destroy(EntityID Entity);
        bool        IsValid(EntityID Entity) const;
        const char* GetName(EntityID Entity) const;
        EntityID    FindByName(const char* Name) const;  // linear scan; editor/load only
        std::vector<EntityID> GetAllEntityIDs(ObjectLifespan Lifespan) const;
        ObjectLifespan GetLifespan(EntityID Entity) const;

        // Component operations (inline templates — no engine API calls here)
        template<typename T>
        T& Emplace(EntityID Entity, T Data = {})
        {
            assert(IsValid(Entity));
            auto& l_Storage = Storage<T>();
            l_Storage.Add(Entity, m_Lifespans[Entity], std::move(Data));
            return *l_Storage.Get(Entity);
        }

        template<typename T>
        void Remove(EntityID Entity)
        {
            Storage<T>().Remove(Entity);
        }

        template<typename T>
        T* Get(EntityID Entity)
        {
            return Storage<T>().Get(Entity);
        }

        template<typename T>
        const T* Get(EntityID Entity) const
        {
            auto* l_Storage = FindStorage<T>();
            if (!l_Storage)
                return nullptr;
            return l_Storage->Get(Entity);
        }

        template<typename T>
        bool Has(EntityID Entity) const
        {
            auto* l_Storage = FindStorage<T>();
            if (!l_Storage)
                return false;
            return l_Storage->Has(Entity);
        }

        template<typename T>
        TComponentStorage<T>& Storage()
        {
            const auto l_Key = typeid(T).hash_code();
            const auto l_It  = m_Storages.find(l_Key);
            if (l_It == m_Storages.end())
            {
                auto l_Storage = std::make_unique<TStorageWrapper<T>>();
                auto* l_Raw    = &l_Storage->m_Storage;
                m_Storages.emplace(l_Key, std::move(l_Storage));
                return *l_Raw;
            }
            return static_cast<TStorageWrapper<T>*>(l_It->second.get())->m_Storage;
        }

        void CleanUp(ObjectLifespan Lifespan);

    private:
        struct IStorageWrapper
        {
            virtual ~IStorageWrapper() = default;
            virtual void CleanUp(ObjectLifespan Lifespan) = 0;
            virtual void Remove(EntityID Entity) = 0;
        };

        template<typename T>
        struct TStorageWrapper : IStorageWrapper
        {
            TComponentStorage<T> m_Storage;
            void CleanUp(ObjectLifespan Lifespan) override
            {
                // TASK-52: log per-storage before/after so a type-specific lifespan mismatch
                // (component registered with wrong lifespan) is visible by name.
                auto l_before = m_Storage.Size();
                m_Storage.CleanUp(Lifespan);
                auto l_after = m_Storage.Size();
                Log(Verbose, "  storage<", typeid(T).name(), ">: ", l_before, " -> ", l_after);
            }
            void Remove(EntityID Entity) override          { m_Storage.Remove(Entity); }
        };

        template<typename T>
        TComponentStorage<T>* FindStorage() const
        {
            const auto l_Key = typeid(T).hash_code();
            const auto l_It  = m_Storages.find(l_Key);
            if (l_It == m_Storages.end())
                return nullptr;
            return &static_cast<TStorageWrapper<T>*>(l_It->second.get())->m_Storage;
        }

        // Entity metadata arrays, indexed by EntityID
        std::vector<bool>            m_Valid;
        std::vector<ObjectLifespan>  m_Lifespans;
        std::vector<std::string>     m_Names;

        // Free list for entity slot recycling
        std::vector<EntityID>        m_FreeList;
        EntityID                     m_NextID = 1;   // 0 = INVALID_ENTITY

        // Type-erased component storages keyed by type hash
        std::unordered_map<size_t, std::unique_ptr<IStorageWrapper>> m_Storages;

        ObjectStatus m_ObjectStatus = ObjectStatus::Invalid;
    };
}
