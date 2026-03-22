#include "TransformService.h"
#include "../Common/MathHelper.h"
#include "../Common/LogService.h"
#include "../Component/TransformComponent.h"
#include "../Component/WorldTransformComponent.h"
#include "EntityRegistry.h"
#include "SceneService.h"
#include "../Engine.h"

using namespace Inno;

bool TransformService::Setup(ISystemConfig*)
{
	m_Nodes.resize(MAX_ENTITIES);
	m_ObjectStatus = ObjectStatus::Created;
	return true;
}

bool TransformService::Initialize()
{
	m_SceneLoadingCallback = [this]()
	{
		std::fill(m_Nodes.begin(), m_Nodes.end(), HierarchyNode{});
		m_TraversalOrder.clear();
		m_HierarchyDirty = true;
	};
	g_Engine->Get<SceneService>()->AddSceneLoadingStartedCallback(&m_SceneLoadingCallback, 0);

	m_ObjectStatus = ObjectStatus::Activated;
	return true;
}

bool TransformService::Update()
{
	if (m_ObjectStatus != ObjectStatus::Activated)
		return true;

	auto* l_Registry = g_Engine->Get<EntityRegistry>();
	auto& l_TransformStorage = l_Registry->Storage<TransformComponent>();
	const auto& l_Owners = l_TransformStorage.AllOwners();

	for (EntityID l_Entity : l_Owners)
	{
		if (!l_Registry->Has<WorldTransformComponent>(l_Entity))
			l_Registry->Emplace<WorldTransformComponent>(l_Entity);
	}

	if (m_HierarchyDirty || l_Owners.size() != m_TraversalOrder.size())
		RebuildTraversalOrder(l_Owners);

	for (EntityID l_Entity : m_TraversalOrder)
	{
		auto* l_Local = l_Registry->Get<TransformComponent>(l_Entity);
		auto* l_World = l_Registry->Get<WorldTransformComponent>(l_Entity);
		if (!l_Local || !l_World)
			continue;

		Mat4 l_T = Math::toTranslationMatrix(Vec4(l_Local->m_LocalPos, 1.0f));
		Mat4 l_R = Math::toRotationMatrix(l_Local->m_LocalRot);
		Mat4 l_S = Math::toScaleMatrix(Vec4(l_Local->m_LocalScale, 1.0f));
		Mat4 l_LocalTRS = l_T * l_R * l_S;

		EntityID l_Parent = m_Nodes[l_Entity].m_Parent;
		if (l_Parent == INVALID_ENTITY)
		{
			l_World->m_WorldMatrix         = l_LocalTRS;
			l_World->m_WorldRotationMatrix = l_R;
		}
		else
		{
			auto* l_ParentWorld = l_Registry->Get<WorldTransformComponent>(l_Parent);
			if (l_ParentWorld)
			{
				l_World->m_WorldMatrix         = l_ParentWorld->m_WorldMatrix * l_LocalTRS;
				l_World->m_WorldRotationMatrix = l_ParentWorld->m_WorldRotationMatrix * l_R;
			}
			else
			{
				l_World->m_WorldMatrix         = l_LocalTRS;
				l_World->m_WorldRotationMatrix = l_R;
			}
		}
	}

	return true;
}

bool TransformService::Terminate()
{
	m_ObjectStatus = ObjectStatus::Terminated;
	return true;
}

ObjectStatus TransformService::GetStatus()
{
	return m_ObjectStatus;
}

void TransformService::SetParent(EntityID l_Child, EntityID l_Parent)
{
	if (l_Child == INVALID_ENTITY || l_Child >= MAX_ENTITIES || l_Child == l_Parent)
		return;
	if (m_Nodes[l_Child].m_Parent == l_Parent)
		return;

	// Detach from current parent.
	EntityID l_OldParent = m_Nodes[l_Child].m_Parent;
	if (l_OldParent != INVALID_ENTITY)
	{
		EntityID& l_Head = m_Nodes[l_OldParent].m_FirstChild;
		if (l_Head == l_Child)
		{
			l_Head = m_Nodes[l_Child].m_NextSibling;
		}
		else
		{
			EntityID l_Prev = l_Head;
			while (l_Prev != INVALID_ENTITY && m_Nodes[l_Prev].m_NextSibling != l_Child)
				l_Prev = m_Nodes[l_Prev].m_NextSibling;
			if (l_Prev != INVALID_ENTITY)
				m_Nodes[l_Prev].m_NextSibling = m_Nodes[l_Child].m_NextSibling;
		}
	}

	// Attach to new parent (prepend to sibling list).
	m_Nodes[l_Child].m_Parent      = l_Parent;
	m_Nodes[l_Child].m_NextSibling = INVALID_ENTITY;

	if (l_Parent != INVALID_ENTITY && l_Parent < MAX_ENTITIES)
	{
		m_Nodes[l_Child].m_NextSibling = m_Nodes[l_Parent].m_FirstChild;
		m_Nodes[l_Parent].m_FirstChild = l_Child;
		m_Nodes[l_Child].m_Depth       = m_Nodes[l_Parent].m_Depth + 1;
	}
	else
	{
		m_Nodes[l_Child].m_Depth = 0;
	}

	m_HierarchyDirty = true;
}

void TransformService::ClearParent(EntityID l_Child)
{
	SetParent(l_Child, INVALID_ENTITY);
}

EntityID TransformService::GetParent(EntityID l_Child) const
{
	if (l_Child == INVALID_ENTITY || l_Child >= MAX_ENTITIES)
		return INVALID_ENTITY;
	return m_Nodes[l_Child].m_Parent;
}

EntityID TransformService::GetFirstChild(EntityID l_Parent) const
{
	if (l_Parent == INVALID_ENTITY || l_Parent >= MAX_ENTITIES)
		return INVALID_ENTITY;
	return m_Nodes[l_Parent].m_FirstChild;
}

EntityID TransformService::GetNextSibling(EntityID l_Entity) const
{
	if (l_Entity == INVALID_ENTITY || l_Entity >= MAX_ENTITIES)
		return INVALID_ENTITY;
	return m_Nodes[l_Entity].m_NextSibling;
}

void TransformService::RebuildTraversalOrder(const std::vector<EntityID>& l_AllOwners)
{
	m_TraversalOrder.clear();
	m_TraversalOrder.reserve(l_AllOwners.size());

	std::vector<EntityID> l_Queue;
	l_Queue.reserve(l_AllOwners.size());

	for (EntityID l_Entity : l_AllOwners)
	{
		if (m_Nodes[l_Entity].m_Parent == INVALID_ENTITY)
			l_Queue.push_back(l_Entity);
	}

	for (size_t l_Idx = 0; l_Idx < l_Queue.size(); ++l_Idx)
	{
		EntityID l_Current = l_Queue[l_Idx];
		m_TraversalOrder.push_back(l_Current);

		EntityID l_Child = m_Nodes[l_Current].m_FirstChild;
		while (l_Child != INVALID_ENTITY)
		{
			l_Queue.push_back(l_Child);
			l_Child = m_Nodes[l_Child].m_NextSibling;
		}
	}

	m_HierarchyDirty = false;
}
