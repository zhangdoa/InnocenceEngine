#pragma once
#include "../Interface/ISystem.h"

namespace Inno
{
	struct LightSystemImpl;
	class LightSystem : public ISystem
	{
	public:
		INNO_CLASS_CONCRETE_NON_COPYABLE(LightSystem);

		bool Setup(ISystemConfig* systemConfig = nullptr) override;
		bool Initialize() override;
		bool Update() override;
		bool Terminate() override;

		ObjectStatus GetStatus() override;

	private:
		LightSystemImpl* m_Impl;
	};
}