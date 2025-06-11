#pragma once
#include "../Interface/ISystem.h"
#include "../Component/ModelComponent.h"
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

		bool CreateCollisionComponent(const MeshComponent& component);
		bool CreateCollisionComponent(const ModelComponent& component);

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
