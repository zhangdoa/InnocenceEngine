#pragma once
#include "../Common/Array.h"
#include "../Common/EntityID.h"
#include "../Common/GPUDataStructure.h"

namespace Inno
{
    struct BVHNode
    {
        AABB m_AABB;

        Inno::Array<BVHNode>::iterator m_Parent;
        Inno::Array<BVHNode>::iterator m_LeftChild;
        Inno::Array<BVHNode>::iterator m_RightChild;
        size_t m_Depth = 0;

        EntityID m_Entity = INVALID_ENTITY;

        bool operator==(const BVHNode& other) const
        {
            return (
                m_Parent == other.m_Parent
                && m_LeftChild == other.m_LeftChild
                && m_RightChild == other.m_RightChild
                );
        }
    };

    struct BVHServiceImpl;
    class BVHService
    {
    public:
        BVHService();

        void Update();
        void AddNode(EntityID Entity);
        void ClearNodes();
        const Inno::Array<BVHNode>& GetNodes();

    private:
        BVHServiceImpl* m_Impl;
    };
}