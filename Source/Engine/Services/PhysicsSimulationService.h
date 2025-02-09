#pragma once
#include "../Component/CollisionComponent.h"
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

		bool CreateCollisionPrimitive(MeshComponent* meshComponent);
		bool CreateCollisionComponents(ModelComponent* modelComponent);

		void RunCulling();
		const std::vector<CullingResult>& GetCullingResult();
		AABB GetVisibleSceneAABB();
		AABB GetStaticSceneAABB();
		AABB GetTotalSceneAABB();

		bool AddForce(ModelComponent* modelComponent, Vec4 force);

	private:
		PhysicsSimulationServiceImpl* m_Impl;
	};
}
