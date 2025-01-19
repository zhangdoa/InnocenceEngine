#pragma once
#include "../Component/PhysicsComponent.h"
#include "../Common/GPUDataStructure.h"
#include "CullingResult.h"

namespace Inno
{
	struct PhysicsSystemImpl;
	class PhysicsSystem : public ISystem
	{
	public:
		INNO_CLASS_CONCRETE_NON_COPYABLE(PhysicsSystem);

		bool Setup(ISystemConfig* systemConfig) override;
		bool Initialize() override;
		bool Update() override;
		bool Terminate() override;

		ObjectStatus GetStatus();

		bool GeneratePhysicsProxy(ModelComponent* modelComponent);

		void RunCulling();
		const std::vector<CullingResult>& GetCullingResult();
		AABB GetVisibleSceneAABB();
		AABB GetStaticSceneAABB();
		AABB GetTotalSceneAABB();

		bool AddForce(ModelComponent* modelComponent, Vec4 force);

	private:
		PhysicsSystemImpl* m_Impl;
	};
}
