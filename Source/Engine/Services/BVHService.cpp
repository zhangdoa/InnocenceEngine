#include "BVHService.h"
#include "PhysicsSimulationService.h"

#include "../Engine.h"
using namespace Inno;

namespace Inno
{
    struct BVHServiceImpl
    {
        AABB GenerateAABB(std::vector<BVHNode>::iterator begin, std::vector<BVHNode>::iterator end);
        bool GenerateNodes(std::vector<BVHNode>::iterator node, size_t begin, size_t end, std::vector<BVHNode>& nodes);
        void Update();

        const size_t m_maxDepth = 256;
        const size_t m_MaxComponentCount = 16384;

        BVHNode m_RootNode;

        std::vector<BVHNode> m_Nodes;
        std::atomic<size_t> m_WorkloadCount = 0;

        std::vector<BVHNode> m_TempNodes;
    };
}

AABB BVHServiceImpl::GenerateAABB(std::vector<BVHNode>::iterator begin, std::vector<BVHNode>::iterator end)
{
    auto l_BoundMax = Math::minVec4<float>;
    l_BoundMax.w = 1.0f;
    auto l_BoundMin = Math::maxVec4<float>;
    l_BoundMin.w = 1.0f;

    for (auto it = begin; it != end; it++)
    {
        l_BoundMax = Math::elementWiseMax(it->m_AABB.m_boundMax, l_BoundMax);
        l_BoundMin = Math::elementWiseMin(it->m_AABB.m_boundMin, l_BoundMin);
    }

    return Math::GenerateAABB(l_BoundMax, l_BoundMin);
}

bool BVHServiceImpl::GenerateNodes(std::vector<BVHNode>::iterator node, size_t begin, size_t end, std::vector<BVHNode>& nodes)
{
    if (end - begin < 3)
    {
        return true;
    }

    if (node->m_Depth >= m_maxDepth)
    {
        return true;
    }

    // Find max axis
    uint32_t l_maxAxis;
    if (node->m_AABB.m_extend.x > node->m_AABB.m_extend.y)
    {
        if (node->m_AABB.m_extend.x > node->m_AABB.m_extend.z)
        {
            l_maxAxis = 0;
        }
        else
        {
            l_maxAxis = 2;
        }
    }
    else
    {
        if (node->m_AABB.m_extend.y > node->m_AABB.m_extend.z)
        {
            l_maxAxis = 1;
        }
        else
        {
            l_maxAxis = 2;
        }
    }

    auto l_begin = nodes.begin() + begin;
    auto l_end = nodes.begin() + end;

    // Sort children nodes
    std::sort(l_begin, l_end, [&](BVHNode A, BVHNode B)
        {
            return A.m_AABB.m_boundMin[l_maxAxis] < B.m_AABB.m_boundMin[l_maxAxis];
        });

#define SPATIAL_DIVIDE 0
#if SPATIAL_DIVIDE
    auto l_maxAxisLength = node->m_AABB.m_extend[l_maxAxis];
    auto l_middleMaxAxis = node->m_AABB.m_boundMin[l_maxAxis] + l_maxAxisLength / 2.0f;
    auto l_middle = std::find_if(l_begin, l_end, [&](BVHNode A)
        {
            return A.m_AABB.m_boundMin[l_maxAxis] > l_middleMaxAxis;
        });
#else
    auto l_middle = l_begin + (end - begin) / 2;
#endif

    // Add intermediate nodes
    if (l_middle != l_end && l_middle != l_begin)
    {
        BVHNode l_leftChildNode;
        l_leftChildNode.m_Parent = node;
        l_leftChildNode.m_Depth = node->m_Depth + 1;
        l_leftChildNode.m_AABB = GenerateAABB(l_begin, l_middle);

        BVHNode l_rightChildNode;
        l_rightChildNode.m_Parent = node;
        l_rightChildNode.m_Depth = node->m_Depth + 1;
        l_rightChildNode.m_AABB = GenerateAABB(l_middle, l_end);

        nodes.emplace_back(l_leftChildNode);
        node->m_LeftChild = nodes.end() - 1;

        nodes.emplace_back(l_rightChildNode);
        node->m_RightChild = nodes.end() - 1;

        auto l_middleRelativeIndex = std::distance(l_begin, l_middle);
        GenerateNodes(node->m_LeftChild, begin, begin + l_middleRelativeIndex, nodes);
        GenerateNodes(node->m_RightChild, begin + l_middleRelativeIndex, end, nodes);
    }

    return true;
}

void BVHServiceImpl::Update()
{
    if (m_WorkloadCount)
    {
        m_WorkloadCount = 0;
    }

    m_TempNodes.clear();

    for (auto& i : m_Nodes)
    {
        // AABB now stored directly in ModelComponent
        i.m_AABB = i.ModelComponent->m_AABB;
    }

    m_RootNode.m_AABB = g_Engine->Get<PhysicsSimulationService>()->GetStaticSceneAABB();

    m_TempNodes.emplace_back(m_RootNode);
    m_TempNodes.insert(m_TempNodes.end(), m_Nodes.begin(), m_Nodes.end());

    GenerateNodes(m_TempNodes.begin(), 1, m_TempNodes.size(), m_TempNodes);
}


BVHService::BVHService()
{
    m_Impl = new BVHServiceImpl();

    m_Impl->m_Nodes.reserve(m_Impl->m_MaxComponentCount);
    m_Impl->m_TempNodes.reserve(m_Impl->m_MaxComponentCount * 2);
}

void BVHService::Update()
{
    m_Impl->Update();
}

void BVHService::AddNode(ModelComponent* ModelComponent)
{
    BVHNode l_BVHNode;
    l_BVHNode.ModelComponent = ModelComponent;
    l_BVHNode.m_AABB = ModelComponent->m_AABB;

    m_Impl->m_Nodes.emplace_back(l_BVHNode);

    m_Impl->m_WorkloadCount++;
}

void BVHService::ClearNodes()
{
    m_Impl->m_Nodes.clear();

    Log(Verbose, "All BVH nodes have been cleared.");
}

const std::vector<BVHNode>& BVHService::GetNodes()
{
    return m_Impl->m_Nodes;
}