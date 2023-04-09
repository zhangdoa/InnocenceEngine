#pragma once
#include "../Interface/ISystem.h"
#include "../Component/TransformComponent.h"

namespace Inno
{
	class InnoTransformSystem : public ITransformSystem
	{
	public:
		INNO_CLASS_CONCRETE_NON_COPYABLE(InnoTransformSystem);

		bool Setup(ISystemConfig* systemConfig = nullptr) override;
		bool Initialize() override;
		bool Update() override;
		bool OnFrameEnd() override;
		bool Terminate() override;

		ObjectStatus GetStatus() override;

		const TransformComponent* GetRootTransformComponent() override;
	};
}