#pragma once
#include "../Interface/ISystem.h"

namespace Inno
{
	class GUISystem : public ISystem
	{
	public:
		INNO_CLASS_CONCRETE_NON_COPYABLE(GUISystem);

		bool Setup(ISystemConfig* systemConfig) override;
		bool Initialize() override;
		bool Update() override;
		bool Terminate() override;
		bool Render();

		ObjectStatus GetStatus() override;
	};
}