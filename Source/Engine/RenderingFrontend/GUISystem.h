#pragma once
#include "IGUISystem.h"

namespace Inno
{
	class InnoGUISystem : public IGUISystem
	{
	public:
		INNO_CLASS_CONCRETE_NON_COPYABLE(InnoGUISystem);

		bool Setup(ISystemConfig* systemConfig) override;
		bool Initialize() override;
		bool Update() override;
		bool Render() override;
		bool Terminate() override;

		ObjectStatus GetStatus() override;
	};
}