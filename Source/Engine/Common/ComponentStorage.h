#pragma once

#include "EntityID.h"
#include "Object.h"
#include "STL14.h"

namespace Inno
{
    template<typename T>
    class ComponentStorage
    {
    public:
        ComponentStorage()
        {
            m_sparse.assign(MAX_ENTITIES, k_invalid);
        }

        void Add(EntityID entity, ObjectLifespan lifespan, const T& data = {})
        {
            if (entity == INVALID_ENTITY || entity >= MAX_ENTITIES)
                return;
            if (m_sparse[entity] != k_invalid)
                return;

            m_sparse[entity] = static_cast<uint32_t>(m_dense.size());
            m_dense.push_back(data);
            m_owners.push_back(entity);
            m_lifespans.push_back(lifespan);
        }

        void Add(EntityID entity, const T& data = {})
        {
            Add(entity, ObjectLifespan::Invalid, data);
        }

        void Remove(EntityID entity)
        {
            if (entity == INVALID_ENTITY || entity >= MAX_ENTITIES)
                return;
            if (m_sparse[entity] == k_invalid)
                return;

            const uint32_t denseIdx = m_sparse[entity];
            const uint32_t lastIdx  = static_cast<uint32_t>(m_dense.size()) - 1;

            if (denseIdx != lastIdx)
            {
                m_dense[denseIdx]     = std::move(m_dense[lastIdx]);
                m_owners[denseIdx]    = m_owners[lastIdx];
                m_lifespans[denseIdx] = m_lifespans[lastIdx];
                m_sparse[m_owners[denseIdx]] = denseIdx;
            }

            m_dense.pop_back();
            m_owners.pop_back();
            m_lifespans.pop_back();
            m_sparse[entity] = k_invalid;
        }

        T* Get(EntityID entity)
        {
            if (entity == INVALID_ENTITY || entity >= MAX_ENTITIES)
                return nullptr;
            if (m_sparse[entity] == k_invalid)
                return nullptr;
            return &m_dense[m_sparse[entity]];
        }

        const T* Get(EntityID entity) const
        {
            if (entity == INVALID_ENTITY || entity >= MAX_ENTITIES)
                return nullptr;
            if (m_sparse[entity] == k_invalid)
                return nullptr;
            return &m_dense[m_sparse[entity]];
        }

        bool Has(EntityID entity) const
        {
            if (entity == INVALID_ENTITY || entity >= MAX_ENTITIES)
                return false;
            return m_sparse[entity] != k_invalid;
        }

        T& GetOrAdd(EntityID entity)
        {
            if (!Has(entity))
                Add(entity);
            return m_dense[m_sparse[entity]];
        }

        const std::vector<T>& All() const             { return m_dense; }
        std::vector<T>&       All()                   { return m_dense; }
        const std::vector<EntityID>& AllOwners() const { return m_owners; }

        void CleanUp(ObjectLifespan lifespan)
        {
            for (int32_t i = static_cast<int32_t>(m_dense.size()) - 1; i >= 0; --i)
            {
                if (m_lifespans[static_cast<size_t>(i)] == lifespan)
                    Remove(m_owners[static_cast<size_t>(i)]);
            }
        }

        size_t Size() const { return m_dense.size(); }

    private:
        static constexpr uint32_t k_invalid = UINT32_MAX;

        std::vector<T>              m_dense;
        std::vector<EntityID>       m_owners;
        std::vector<ObjectLifespan> m_lifespans;
        std::vector<uint32_t>       m_sparse;
    };
}
