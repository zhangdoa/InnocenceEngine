#pragma once
#include "../Common/Array.h"
#include "../Interface/IService.h"
#include "../Common/EntityID.h"
#include "../Common/GPUDataStructure.h"
#include "CullingResult.h"

namespace Inno
{
	struct PhysicsSimulationServiceImpl;
	class PhysicsSimulationService : public IService
	{
	public:
		INNO_CLASS_CONCRETE_NON_COPYABLE(PhysicsSimulationService);

		bool Setup(IServiceConfig* systemConfig) override;
		bool Initialize() override;
		bool Update() override;
		bool Terminate() override;

		ObjectStatus GetStatus();

		bool CreateCollisionComponent(EntityID Entity);

		void RunCulling();
		const Inno::Array<CullingResult>& GetCullingResult();
		AABB GetVisibleSceneAABB();
		AABB GetStaticSceneAABB();
		AABB GetTotalSceneAABB();

		bool AddForce(EntityID Entity, Vec4 Force);

		void OnSceneUnloading();

	private:
		PhysicsSimulationServiceImpl* m_Impl;
	};
}
