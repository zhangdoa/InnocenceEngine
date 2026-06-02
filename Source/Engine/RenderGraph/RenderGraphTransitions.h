#pragma once
#include "IRenderGraphKernel.h"

namespace Inno
{
	class FrameManagementService;

	// Records the node's m_Transitions as graphics-queue barriers in array order.
	// Routed onto graphicsCL because the compute queue cannot transition a render
	// target between ReadOnly and WriteOnly.
	bool RecordTransitionPrepass(RenderGraphPassContext& ctx, CommandListComponent* graphicsCL, FrameManagementService* fmService);
}
