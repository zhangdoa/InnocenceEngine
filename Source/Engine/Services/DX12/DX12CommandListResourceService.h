#pragma once
#include "../CommandListResourceService.h"

namespace Inno
{
	struct DX12Context;

	class DX12CommandListResourceService : public CommandListResourceService
	{
	public:
		INNO_CLASS_CONCRETE_NON_COPYABLE(DX12CommandListResourceService);

		void SetDX12Context(DX12Context* ctx) { m_ctx = ctx; }

		bool Delete(CommandListComponent* ptr) override;

	protected:
		bool InitializeImpl(CommandListComponent* commandList) override;

	private:
		DX12Context* m_ctx = nullptr;
	};
}
