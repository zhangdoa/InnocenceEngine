#pragma once
#include "../Component/ModelComponent.h"
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

        ModelComponent* ModelComponent = 0;

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
        void AddNode(ModelComponent* ModelComponent);
        void ClearNodes();
        const std::vector<BVHNode>& GetNodes();

    private:
        BVHServiceImpl* m_Impl;
    };
}