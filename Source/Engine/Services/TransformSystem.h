#pragma once
#include "../Interface/ISystem.h"
#include "../Component/TransformComponent.h"

namespace Inno
{
	class TransformSystem : public ITransformSystem
	{
	public:
		INNO_CLASS_CONCRETE_NON_COPYABLE(TransformSystem);

		bool Setup(ISystemConfig* systemConfig = nullptr) override;
		bool Initialize() override;
		bool Update() override;
		bool Terminate() override;

		ObjectStatus GetStatus() override;

		const TransformComponent* GetRootTransformComponent() override;
	};
}