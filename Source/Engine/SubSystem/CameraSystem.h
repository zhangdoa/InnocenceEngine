#pragma once
#include "../Interface/ISystem.h"

namespace Inno
{
	class InnoCameraSystem : public ISystem
	{
	public:
		INNO_CLASS_CONCRETE_NON_COPYABLE(InnoCameraSystem);

		bool Setup(ISystemConfig* systemConfig = nullptr) override;
		bool Initialize() override;
		bool Update() override;
		bool OnFrameEnd() override;
		bool Terminate() override;

		ObjectStatus GetStatus() override;
	};
}