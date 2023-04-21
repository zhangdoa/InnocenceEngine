#pragma once
#include "IGUISystem.h"

namespace Inno
{
	class GUISystem : public IGUISystem
	{
	public:
		INNO_CLASS_CONCRETE_NON_COPYABLE(GUISystem);

		bool Setup(ISystemConfig* systemConfig) override;
		bool Initialize() override;
		bool Update() override;
		bool Render() override;
		bool Terminate() override;

		ObjectStatus GetStatus() override;
	};
}