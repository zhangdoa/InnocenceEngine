#pragma once
#include "../Component/PhysicsComponent.h"
#include "../Common/GPUDataStructure.h"

namespace Inno
{
    struct BVHNode
    {
        AABB m_AABB;

        std::vector<BVHNode>::iterator m_Parent;
        std::vector<BVHNode>::iterator m_LeftChild;
        std::vector<BVHNode>::iterator m_RightChild;
        size_t m_Depth = 0;

        PhysicsComponent* PDC = 0;

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
        void AddNode(PhysicsComponent* PDC);
        void ClearNodes();
        const std::vector<BVHNode>& GetNodes();

    private:
        BVHServiceImpl* m_Impl;
    };
}