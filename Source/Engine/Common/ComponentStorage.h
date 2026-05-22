#pragma once
#include "Array.h"

#include "STL14.h"
#include "EntityID.h"
#include "Object.h"

namespace Inno
{
    template<typename T>
    class TComponentStorage
    {
    public:
        TComponentStorage()
        {
            m_Sparse.fill(InvalidIndex);
        }

        void Add(EntityID entity, ObjectLifespan lifespan, const T& data = {})
        {
            if (entity == INVALID_ENTITY || entity >= MAX_ENTITIES)
                return;
            if (m_Sparse[entity] != InvalidIndex)
            {
                assert(false && "TComponentStorage::Add - entity already has a component of this type");
                return;
            }

            m_Sparse[entity] = static_cast<uint32_t>(m_Dense.size());
            m_Dense.push_back(data);
            m_Owners.push_back(entity);
            m_Lifespans.push_back(lifespan);
        }

        void Add(EntityID entity, const T& data = {})
        {
            Add(entity, ObjectLifespan::Invalid, data);
        }

        void Remove(EntityID entity)
        {
            if (entity == INVALID_ENTITY || entity >= MAX_ENTITIES)
                return;
            if (m_Sparse[entity] == InvalidIndex)
                return;

            assert(!m_Dense.empty() && "TComponentStorage::Remove - dense array is empty but entity has a sparse entry");

            const uint32_t l_DenseIdx = m_Sparse[entity];
            const uint32_t l_LastIdx  = static_cast<uint32_t>(m_Dense.size()) - 1;

            if (l_DenseIdx != l_LastIdx)
            {
                m_Dense[l_DenseIdx]     = std::move(m_Dense[l_LastIdx]);
                m_Owners[l_DenseIdx]    = m_Owners[l_LastIdx];
                m_Lifespans[l_DenseIdx] = m_Lifespans[l_LastIdx];
                m_Sparse[m_Owners[l_DenseIdx]] = l_DenseIdx;
            }

            m_Dense.pop_back();
            m_Owners.pop_back();
            m_Lifespans.pop_back();
            m_Sparse[entity] = InvalidIndex;
        }

        T* Get(EntityID entity)
        {
            if (entity == INVALID_ENTITY || entity >= MAX_ENTITIES)
                return nullptr;
            if (m_Sparse[entity] == InvalidIndex)
                return nullptr;
            return &m_Dense[m_Sparse[entity]];
        }

        const T* Get(EntityID entity) const
        {
            if (entity == INVALID_ENTITY || entity >= MAX_ENTITIES)
                return nullptr;
            if (m_Sparse[entity] == InvalidIndex)
                return nullptr;
            return &m_Dense[m_Sparse[entity]];
        }

        bool Has(EntityID entity) const
        {
            if (entity == INVALID_ENTITY || entity >= MAX_ENTITIES)
                return false;
            return m_Sparse[entity] != InvalidIndex;
        }

        T* GetOrAdd(EntityID entity)
        {
            if (entity == INVALID_ENTITY || entity >= MAX_ENTITIES)
                return nullptr;
            if (!Has(entity))
                Add(entity);
            return &m_Dense[m_Sparse[entity]];
        }

        // Returns a reference to the packed dense array for cache-friendly sequential iteration.
        // C++17: returns Inno::Array<T>& instead of std::span<T> (std::span is C++20).
        const Inno::Array<T>& All() const             { return m_Dense; }
        Inno::Array<T>&       All()                   { return m_Dense; }
        const Inno::Array<EntityID>& AllOwners() const { return m_Owners; }

        void CleanUp(ObjectLifespan lifespan)
        {
            for (int32_t i = static_cast<int32_t>(m_Dense.size()) - 1; i >= 0; --i)
            {
                if (m_Lifespans[static_cast<size_t>(i)] == lifespan)
                    Remove(m_Owners[static_cast<size_t>(i)]);
            }
        }

        size_t Size() const { return m_Dense.size(); }

    private:
        static constexpr uint32_t InvalidIndex = UINT32_MAX;

        Inno::Array<T>              m_Dense;
        Inno::Array<EntityID>       m_Owners;
        Inno::Array<ObjectLifespan> m_Lifespans;
        std::array<uint32_t, MAX_ENTITIES> m_Sparse;
    };
}
