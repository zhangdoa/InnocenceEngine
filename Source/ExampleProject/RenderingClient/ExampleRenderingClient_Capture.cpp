#include "ExampleRenderingClient_Internal.h"
#include "FinalBlendPass.h"

#include "../../Engine/Services/AssetService.h"
#include "../../Engine/Services/EditorService.h"
#include "../../Engine/Services/FrameManagementService.h"
#include "../../Engine/Services/GraphicsHardwareService.h"
#include "../../Engine/Services/PerFrameDataService.h"
#include "../../Engine/Services/TextureResourceService.h"

#include "../../Engine/Engine.h"

#include <chrono>
#include <cmath>
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

		// TASK-211: editor Screenshot action consumer. Writes a uniquely-
		// named capture under Bin/Captures/Screenshots/ (engine CWD-relative)
		// and broadcasts SCREENSHOT_SAVED via EditorService so the editor
		// can surface a toast naming the absolute path. Best-effort
		// broadcast — the engine-side log line is the durable record.
		auto l_srcTextureComp = static_cast<TextureComponent*>(FinalBlendPass::Get().GetResult());
		auto* l_editorService = g_Engine->Get<EditorService>();

		// Step 1: ensure output directory exists.
		const std::filesystem::path l_outputDir = std::filesystem::path("Captures") / "Screenshots";
		std::error_code l_dirEc;
		std::filesystem::create_directories(l_outputDir, l_dirEc);
		if (l_dirEc)
		{
			const std::string l_errorReason =
				std::string("Screenshot: failed to create directory '") + l_outputDir.string()
				+ "': " + l_dirEc.message();
			Log(Warning, "Screenshot failed: ", l_errorReason.c_str());
			(void)l_editorService->BroadcastScreenshotSaved(false, std::string(), l_errorReason);
			m_saveScreenCapture = false;
			return;
		}

		// Step 2: timestamped filename. Millisecond precision avoids
		// collisions when the user clicks twice within the same second.
		const auto l_now = std::chrono::system_clock::now();
		const auto l_nowTimeT = std::chrono::system_clock::to_time_t(l_now);
		const auto l_nowMs = std::chrono::duration_cast<std::chrono::milliseconds>(
			l_now.time_since_epoch()) % std::chrono::milliseconds(1000);
		std::tm l_tm{};
#if defined(_WIN32)
		localtime_s(&l_tm, &l_nowTimeT);
#else
		localtime_r(&l_nowTimeT, &l_tm);
#endif
		std::ostringstream l_nameStream;
		l_nameStream << "screenshot_"
			<< std::put_time(&l_tm, "%Y-%m-%d_%H-%M-%S")
			<< "-" << std::setw(3) << std::setfill('0') << l_nowMs.count();

		// Step 3: extension by pixel-format branch — matches
		// STBWrapper::Save (UByte -> stbi_write_png; Float16/Float32
		// -> stbi_write_hdr).
		const TexturePixelDataType l_pixelType = l_srcTextureComp->m_TextureDesc.PixelDataType;
		const char* l_extension = (l_pixelType == TexturePixelDataType::Float16
			|| l_pixelType == TexturePixelDataType::Float32) ? ".hdr" : ".png";
		l_nameStream << l_extension;

		const std::filesystem::path l_relativePath = l_outputDir / l_nameStream.str();
		std::error_code l_absEc;
		const std::filesystem::path l_absolutePath =
			std::filesystem::absolute(l_relativePath, l_absEc).make_preferred();
		const std::string l_absolutePathStr = l_absEc
			? std::filesystem::path(l_relativePath).make_preferred().string()
			: l_absolutePath.string();

		// Step 4: GPU readback.
		auto l_textureData = g_Engine->Get<TextureResourceService>()->ReadTextureBackToCPU(
			FinalBlendPass::Get().GetRenderPassComp(), l_srcTextureComp);
		if (l_textureData.empty())
		{
			const std::string l_errorReason =
				std::string("Screenshot: ReadTextureBackToCPU returned empty for '")
				+ l_absolutePathStr + "'.";
			Log(Warning, "Screenshot failed: ", l_errorReason.c_str());
			(void)l_editorService->BroadcastScreenshotSaved(false, l_absolutePathStr, l_errorReason);
		}
		else if (g_Engine->Get<AssetService>()->Save(l_absolutePathStr.c_str(),
			l_srcTextureComp->m_TextureDesc, l_textureData.data()))
		{
			Log(Success, "Screenshot: ", l_absolutePathStr.c_str());
			(void)l_editorService->BroadcastScreenshotSaved(true, l_absolutePathStr, std::string());
		}
		else
		{
			const std::string l_errorReason =
				std::string("Screenshot: AssetService::Save failed for '")
				+ l_absolutePathStr + "'.";
			Log(Warning, "Screenshot failed: ", l_errorReason.c_str());
			(void)l_editorService->BroadcastScreenshotSaved(false, l_absolutePathStr, l_errorReason);
		}
		m_saveScreenCapture = false;
	}

	void ExampleRenderingClientImpl::HandleAutoCaptureTriggers()
	{
		auto l_totalFrames = g_Engine->getInitConfig().totalFrames;
		const bool l_isPathTracerTestMode =
			strcmp(g_Engine->getInitConfig().testCase, "gpu_path_tracer") == 0 && m_GPUPathTracerActive;
		// Serialize-test mode sets totalFrames=1 for the parse's auto-terminate
		// path, but the render pipeline (FinalBlendPass, et al.) is intentionally
		// not activated in that mode — the test's whole work is scene load +
		// save + compare, no rendering. Skip the auto-capture trigger so
		// ReadTextureBackToCPU doesn't run against a texture with empty GPU
		// resources and hit the fatal-on-error log path.
		const bool l_isSerializeTest = g_Engine->getInitConfig().serializeTest[0] != '\0';
		const uint32_t l_triggerAtFrame = l_isSerializeTest ? 0u
			: (l_totalFrames > 0
				? static_cast<uint32_t>(l_totalFrames)
				: (l_isPathTracerTestMode ? 30u : 0u));

		// Unified per-frame counter. Previously lived inside the one-shot
		// trigger's conditional; moved out so the `-dump_frames` path can
		// share it. Runs that don't use either feature increment the
		// counter harmlessly — nothing else reads it.
		// TASK-213 CL B: gated on FrameManagementService's steady-state latch
		// so the counter is steady-state-relative, not absolute. Frozen at 0
		// until IsSteadyState() first goes true; from that frame on, advances
		// 1-per-rendered-frame. Cross-launch the load-frame count varies
		// (deferred-init drain timing) so the absolute counter at the dump
		// frame would differ; the latch gates that variability out. Flap-back
		// (TLAS rebuild after first-true, e.g. GISponza frame=16 / 30) does
		// NOT reset the counter — once accumulation has begun, resetting would
		// corrupt the running mean. See CL A's reviewer carry-forward.
		if (g_Engine->Get<FrameManagementService>()->HasReachedSteadyState())
			m_autoCaptureFrameCount++;

		// Frame-sequence dump for temporal validation. When
		// `-dump_frames START-END` is set, write `gpu_output_NNNN.png` for
		// every frame N in [START, END] inclusive. Lets a reviewer scrub /
		// diff consecutive frames to catch flickering, probe-spawn
		// oscillation, or denoiser instability that a single-frame capture
		// can't expose. Skipped when the serialize-test or an un-activated
		// FinalBlendPass would make the readback meaningless (same guard
		// shape as the one-shot trigger).
		const auto& l_initCfg = g_Engine->getInitConfig();
		if (!l_isSerializeTest
			&& l_initCfg.dumpFramesStart >= 0
			&& l_initCfg.dumpFramesEnd >= l_initCfg.dumpFramesStart
			&& m_autoCaptureFrameCount >= static_cast<uint32_t>(l_initCfg.dumpFramesStart)
			&& m_autoCaptureFrameCount <= static_cast<uint32_t>(l_initCfg.dumpFramesEnd))
		{
			char l_buf[64];
			snprintf(l_buf, sizeof(l_buf), "gpu_output_%04u.png", m_autoCaptureFrameCount);
			WriteCaptureToFile(l_buf);
		}

		// Per-frame trigger: mid-session snapshot (e.g. path tracer frame 30).
		// The structural fallback is FinalizeGPUResults, which runs at shutdown
		// after WaitForGPUIdle and catches any case the per-frame trigger missed
		// (e.g. the user exited before the trigger frame). TASK-42.
		if (l_triggerAtFrame > 0 && !m_autoCaptureWritten)
		{
			if (m_autoCaptureFrameCount >= l_triggerAtFrame)
				TryWriteAutoCapture();
		}
	}

	bool ExampleRenderingClientImpl::WriteCaptureToFile(const char* filename)
	{
		auto l_fmService = g_Engine->Get<FrameManagementService>();
		auto l_hwService = g_Engine->Get<GraphicsHardwareService>();
		l_hwService->SignalOnGPU(l_fmService->GetGlobalSemaphore(), GPUEngineType::Graphics);
		auto l_semVal = l_hwService->GetSemaphoreValue(GPUEngineType::Graphics);
		l_hwService->WaitOnCPU(l_semVal, GPUEngineType::Graphics);

		auto l_srcTex = static_cast<TextureComponent*>(FinalBlendPass::Get().GetResult());
		auto l_texFrameIndex = l_srcTex->m_TextureDesc.IsMultiBuffer ? l_fmService->GetCurrentFrame() : 0u;
		l_srcTex->SetCurrentState(l_texFrameIndex, l_srcTex->m_WriteState);
		auto l_floatPixels = g_Engine->Get<TextureResourceService>()->ReadTextureBackToCPU(
			FinalBlendPass::Get().GetRenderPassComp(), l_srcTex);

		if (l_floatPixels.empty())
		{
			Log(Warning, "Capture: ReadTextureBackToCPU returned empty for ", filename);
			return false;
		}

		std::vector<uint8_t> l_uint8Pixels;
		l_uint8Pixels.reserve(l_floatPixels.size() * 4);
		for (const auto& px : l_floatPixels)
		{
			l_uint8Pixels.push_back(uint8_t(255.99f * std::min(sqrtf(std::max(px.x, 0.0f)), 1.0f)));
			l_uint8Pixels.push_back(uint8_t(255.99f * std::min(sqrtf(std::max(px.y, 0.0f)), 1.0f)));
			l_uint8Pixels.push_back(uint8_t(255.99f * std::min(sqrtf(std::max(px.z, 0.0f)), 1.0f)));
			l_uint8Pixels.push_back(uint8_t(255));
		}

		TextureDesc l_desc = l_srcTex->m_TextureDesc;
		l_desc.PixelDataType = TexturePixelDataType::UByte;
		l_desc.PixelDataFormat = TexturePixelDataFormat::RGBA;
		l_desc.Sampler = TextureSampler::Sampler2D;

		if (g_Engine->Get<AssetService>()->Save(filename, l_desc, l_uint8Pixels.data()))
		{
			Log(Success, "Capture: ", filename, " written.");
			return true;
		}
		Log(Warning, "Capture: failed to write ", filename);
		return false;
	}

	void ExampleRenderingClientImpl::TryWriteAutoCapture()
	{
		if (m_autoCaptureWritten)
			return;
		m_autoCaptureWritten = true;

		// PathTracerReadback stats: zero/non-zero pixel split, mean, max.
		// Originally added for path-tracer convergence diagnosis; kept here
		// (one-shot path) rather than in the per-frame dump path so a
		// 100-frame `-dump_frames` run doesn't spam the log.
		auto l_srcTex = static_cast<TextureComponent*>(FinalBlendPass::Get().GetResult());
		auto l_floatPixels = g_Engine->Get<TextureResourceService>()->ReadTextureBackToCPU(
			FinalBlendPass::Get().GetRenderPassComp(), l_srcTex);
		if (!l_floatPixels.empty())
		{
			size_t l_zeroCount = 0, l_nonZeroCount = 0;
			float l_sumR = 0, l_sumG = 0, l_sumB = 0, l_maxR = 0, l_maxG = 0, l_maxB = 0;
			for (const auto& px : l_floatPixels)
			{
				const bool l_isZero = (px.x == 0.0f && px.y == 0.0f && px.z == 0.0f);
				if (l_isZero) { l_zeroCount++; }
				else
				{
					l_nonZeroCount++;
					l_sumR += px.x; l_sumG += px.y; l_sumB += px.z;
					if (px.x > l_maxR) l_maxR = px.x;
					if (px.y > l_maxG) l_maxG = px.y;
					if (px.z > l_maxB) l_maxB = px.z;
				}
			}
			const float l_total = static_cast<float>(l_floatPixels.size());
			Log(Success, "PathTracerReadback: total=", l_floatPixels.size(),
				" zero=", l_zeroCount, " nonZero=", l_nonZeroCount,
				" mean=(", l_sumR / l_total, ",", l_sumG / l_total, ",", l_sumB / l_total, ")",
				" max=(", l_maxR, ",", l_maxG, ",", l_maxB, ")");
		}

		WriteCaptureToFile("gpu_output.png");
	}
}
