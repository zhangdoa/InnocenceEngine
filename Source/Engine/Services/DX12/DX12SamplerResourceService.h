#pragma once
#include "../SamplerResourceService.h"

namespace Inno
{
	struct DX12Context;

	class DX12SamplerResourceService : public SamplerResourceService
	{
	public:
		INNO_CLASS_CONCRETE_NON_COPYABLE(DX12SamplerResourceService);

		void SetDX12Context(DX12Context* ctx) { m_ctx = ctx; }

	protected:
		bool InitializeImpl(SamplerComponent* sampler) override;

	private:
		DX12Context* m_ctx = nullptr;
	};
}
