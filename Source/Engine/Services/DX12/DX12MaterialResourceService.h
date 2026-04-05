#pragma once
#include "../MaterialResourceService.h"

namespace Inno
{
	struct DX12Context;

	class DX12MaterialResourceService : public MaterialResourceService
	{
	public:
		INNO_CLASS_CONCRETE_NON_COPYABLE(DX12MaterialResourceService);

		void SetDX12Context(DX12Context* ctx) { m_ctx = ctx; }

	private:
		DX12Context* m_ctx = nullptr;
	};
}
