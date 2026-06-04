#include "ExampleRenderingClient_Internal.h"
#include "BRDFLUTPass.h"
#include "BRDFLUTMSPass.h"
#include "OpaqueCullingPass.h"
#include "SSAOPass.h"
#include "TiledFrustumGenerationPass.h"
#include "SkyPass.h"
#include "PreTAAPass.h"
#include "LuminanceAveragePass.h"

#include "../../Engine/Services/GraphicsHardwareService.h"

#include "../../Engine/Engine.h"

using namespace Inno;

namespace Inno
{
	// .inl included inside `namespace Inno` so its anonymous-namespace helpers stay per-TU local.
	#include "ExampleRenderingClient_Bypass.inl"

	bool ExampleRenderingClientImpl::PrepareCommands()
	{
		if (m_ExecuteOneShotCommands)
		{
			DispatchOrBypass(BRDFLUTPass::Get());
			DispatchOrBypass(BRDFLUTMSPass::Get());
		}

		DispatchOrBypass(OpaqueCullingPass::Get());

		DispatchOrBypass(SSAOPass::Get());

		DispatchOrBypass(TiledFrustumGenerationPass::Get());

		DispatchOrBypass(SkyPass::Get());

		DispatchOrBypass(PreTAAPass::Get());

		DispatchOrBypass(LuminanceAveragePass::Get());

		// FinalBlendPass/TAAPass archived; present the last keep-set color output.
		m_Canvas = PreTAAPass::Get().GetResult();
		m_CanvasOwner = PreTAAPass::Get().GetRenderPassComp();

		return true;
	}
}
