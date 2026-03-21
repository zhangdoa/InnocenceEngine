#pragma once
#include "../Interface/ISystem.h"
#include "../Common/EntityID.h"
#include "../Common/GPUDataStructure.h"
#include "CullingResult.h"

namespace Inno
{
	struct PhysicsSimulationServiceImpl;
	class PhysicsSimulationService : public ISystem
	{
	public:
		INNO_CLASS_CONCRETE_NON_COPYABLE(PhysicsSimulationService);

		bool Setup(ISystemConfig* systemConfig) override;
		bool Initialize() override;
		bool Update() override;
		bool Terminate() override;

		ObjectStatus GetStatus();

		bool CreateCollisionComponent(EntityID Entity);

		void RunCulling();
		const std::vector<CullingResult>& GetCullingResult();
		AABB GetVisibleSceneAABB();
		AABB GetStaticSceneAABB();
		AABB GetTotalSceneAABB();

		bool AddForce(EntityID Entity, Vec4 Force);

	private:
		PhysicsSimulationServiceImpl* m_Impl;
	};
}
