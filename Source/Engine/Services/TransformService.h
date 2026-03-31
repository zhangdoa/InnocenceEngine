#pragma once
#include "../Common/EntityID.h"
#include "../Common/STL14.h"
#include "../Interface/IService.h"

namespace Inno
{
	struct HierarchyNode
	{
		EntityID m_Parent      = INVALID_ENTITY;
		EntityID m_FirstChild  = INVALID_ENTITY;
		EntityID m_NextSibling = INVALID_ENTITY;
		uint32_t m_Depth       = 0;
	};

	class TransformService : public IService
	{
	public:
		INNO_CLASS_CONCRETE_NON_COPYABLE(TransformService);

		bool Setup(IServiceConfig*) override;
		bool Initialize() override;
		bool Update() override;
		bool Terminate() override;
		ObjectStatus GetStatus() override;

		void OnSceneUnloading();

		void     SetParent(EntityID Child, EntityID Parent);
		void     ClearParent(EntityID Child);
		EntityID GetParent(EntityID Child) const;
		EntityID GetFirstChild(EntityID Parent) const;
		EntityID GetNextSibling(EntityID Entity) const;

	private:
		std::vector<HierarchyNode> m_Nodes;
		std::vector<EntityID>      m_TraversalOrder;
		bool                       m_HierarchyDirty = true;
		ObjectStatus               m_ObjectStatus   = ObjectStatus::Invalid;

		void RebuildTraversalOrder(const std::vector<EntityID>& AllTransformOwners);
	};
}
