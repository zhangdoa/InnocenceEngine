#pragma once
#include "../Interface/ISystem.h"

namespace Inno
{
	class LightSystem : public IComponentSystem
	{
	public:
		INNO_CLASS_CONCRETE_NON_COPYABLE(LightSystem);

		bool Setup(ISystemConfig* systemConfig = nullptr) override;
		bool Initialize() override;
		bool Update() override;
		bool OnFrameEnd() override;
		bool Terminate() override;

		ObjectStatus GetStatus() override;
	};
}