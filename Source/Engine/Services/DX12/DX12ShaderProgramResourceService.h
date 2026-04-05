#pragma once
#include "../ShaderProgramResourceService.h"

namespace Inno
{
	struct DX12Context;

	class DX12ShaderProgramResourceService : public ShaderProgramResourceService
	{
	public:
		INNO_CLASS_CONCRETE_NON_COPYABLE(DX12ShaderProgramResourceService);

		void SetDX12Context(DX12Context* ctx) { m_ctx = ctx; }

	protected:
		bool InitializeImpl(ShaderProgramComponent* shaderProgram) override;

	private:
		DX12Context* m_ctx = nullptr;
	};
}
