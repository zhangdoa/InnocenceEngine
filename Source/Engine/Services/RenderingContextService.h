#pragma once
#include "../Interface/ISystem.h"

namespace Inno
{
	struct RenderingContextServiceImpl;
	class RenderingContextService : public ISystem
	{
	public:
		INNO_CLASS_CONCRETE_NON_COPYABLE(RenderingContextService);

		bool Setup(ISystemConfig* systemConfig) override;
		bool Initialize() override;
		bool Update() override;
		bool Terminate() override;

		ObjectStatus GetStatus() override;

	private:
		RenderingContextServiceImpl* m_Impl;
	};
}
