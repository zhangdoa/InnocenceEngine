#pragma once
#include "../Common/EntityID.h"
#include "../Common/STL14.h"
#include "../Interface/ISystem.h"

namespace Inno
{
	struct HierarchyNode
	{
		EntityID m_Parent      = INVALID_ENTITY;
		EntityID m_FirstChild  = INVALID_ENTITY;
		EntityID m_NextSibling = INVALID_ENTITY;
		uint32_t m_Depth       = 0;
	};

	class TransformService : public ISystem
	{
	public:
		INNO_CLASS_CONCRETE_NON_COPYABLE(TransformService);

		bool Setup(ISystemConfig*) override;
		bool Initialize() override;
		bool Update() override;
		bool Terminate() override;
		ObjectStatus GetStatus() override;

		void     SetParent(EntityID Child, EntityID Parent);
		void     ClearParent(EntityID Child);
		EntityID GetParent(EntityID Child) const;
		EntityID GetFirstChild(EntityID Parent) const;
		EntityID GetNextSibling(EntityID Entity) const;

	private:
		// Sparse array indexed by EntityID — O(1) parent/child lookup.
		// Allocated once at Setup; 65536 * 16 bytes = 1 MB.
		std::vector<HierarchyNode> m_Nodes;
		std::vector<EntityID>      m_TraversalOrder;
		bool                       m_HierarchyDirty = true;
		ObjectStatus               m_ObjectStatus   = ObjectStatus::Invalid;

		std::function<void()> m_SceneLoadingCallback;

		void RebuildTraversalOrder(const std::vector<EntityID>& AllTransformOwners);
	};
}
