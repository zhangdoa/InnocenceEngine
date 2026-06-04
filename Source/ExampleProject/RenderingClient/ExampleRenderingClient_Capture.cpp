#include "ExampleRenderingClient_Internal.h"
#include "../../Engine/Common/Array.h"

#include "../../Engine/Services/AssetService.h"
#include "../../Engine/Services/EditorService.h"
#include "../../Engine/Services/FrameManagementService.h"
#include "../../Engine/Services/GraphicsHardwareService.h"
#include "../../Engine/Services/TextureResourceService.h"

#include "../../Engine/Engine.h"

#include <chrono>
#include <cstring>
#include <filesystem>
#include <iomanip>
#include <sstream>
#include <vector>

using namespace Inno;

namespace Inno
{
	void ExampleRenderingClientImpl::HandleScreenCapture()
	{
		if (!m_saveScreenCapture)
			return;
		// FinalBlendPass archived; screen-capture source unavailable in the keep-set graph.
		m_saveScreenCapture = false;
	}

	void ExampleRenderingClientImpl::HandleAutoCaptureTriggers()
	{
		auto l_totalFrames = g_Engine->getInitConfig().totalFrames;
		const bool l_isPTTestMode =
			strcmp(g_Engine->getInitConfig().testCase, "gpu_path_tracer") == 0 && m_PTActive;
		// Serialize-test mode sets totalFrames=1 but never activates FinalBlendPass; skipping the
		// trigger avoids ReadTextureBackToCPU on an unactivated texture (fatal log path).
		const bool l_isSerializeTest = g_Engine->getInitConfig().serializeTest[0] != '\0';
		const uint32_t l_triggerAtFrame = l_isSerializeTest ? 0u
			: (l_totalFrames > 0
				? static_cast<uint32_t>(l_totalFrames)
				: (l_isPTTestMode ? 30u : 0u));

		// Steady-state gate makes the counter cross-launch reproducible (deferred-init drain timing
		// varies the absolute load frame). Once accumulation begins, flap-back (TLAS rebuild after
		// first-true) does NOT reset — resetting would corrupt the running mean.
		if (g_Engine->Get<FrameManagementService>()->HasReachedSteadyState())
			m_autoCaptureFrameCount++;

		const auto& l_initCfg = g_Engine->getInitConfig();
		if (!l_isSerializeTest
			&& l_initCfg.dumpFramesStart >= 0
			&& l_initCfg.dumpFramesEnd >= l_initCfg.dumpFramesStart
			&& m_autoCaptureFrameCount >= static_cast<uint32_t>(l_initCfg.dumpFramesStart)
			&& m_autoCaptureFrameCount <= static_cast<uint32_t>(l_initCfg.dumpFramesEnd))
		{
			AlignTrackerForMidFrameReadback();

			char l_buf[64];
			snprintf(l_buf, sizeof(l_buf), "gpu_output_%04u.png", m_autoCaptureFrameCount);
			WriteCaptureToFile(l_buf);
		}

		if (l_triggerAtFrame > 0 && !m_autoCaptureWritten)
		{
			if (m_autoCaptureFrameCount >= l_triggerAtFrame)
			{
				AlignTrackerForMidFrameReadback();
				TryWriteAutoCapture();
			}
		}
	}

	void ExampleRenderingClientImpl::AlignTrackerForMidFrameReadback()
	{
	}

	bool ExampleRenderingClientImpl::WriteCaptureToFile(const char* filename)
	{
		// FinalBlendPass archived; no readback source in the keep-set graph.
		(void)filename;
		return false;
	}

	void ExampleRenderingClientImpl::TryWriteAutoCapture()
	{
		// FinalBlendPass archived; auto-capture readback unavailable in the keep-set graph.
		m_autoCaptureWritten = true;
	}
}
