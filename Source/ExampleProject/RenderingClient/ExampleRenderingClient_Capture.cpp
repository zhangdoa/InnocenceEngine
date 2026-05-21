#include "ExampleRenderingClient_Internal.h"
#include "FinalBlendPass.h"

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

		auto l_srcTextureComp = static_cast<TextureComponent*>(FinalBlendPass::Get().GetResult());
		auto* l_editorService = g_Engine->Get<EditorService>();

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

		// Millisecond precision avoids collisions when the user clicks twice in the same second.
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

		// Extension matches STBWrapper::Save dispatch: UByte->png, Float16/Float32->hdr.
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
		// Serialize-test mode sets totalFrames=1 but never activates FinalBlendPass; skipping the
		// trigger avoids ReadTextureBackToCPU on an unactivated texture (fatal log path).
		const bool l_isSerializeTest = g_Engine->getInitConfig().serializeTest[0] != '\0';
		const uint32_t l_triggerAtFrame = l_isSerializeTest ? 0u
			: (l_totalFrames > 0
				? static_cast<uint32_t>(l_totalFrames)
				: (l_isPathTracerTestMode ? 30u : 0u));

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
		auto* l_fmService = g_Engine->Get<FrameManagementService>();
		auto* l_srcTex = static_cast<TextureComponent*>(FinalBlendPass::Get().GetResult());
		auto l_texFrameIndex = l_srcTex->m_TextureDesc.IsMultiBuffer ? l_fmService->GetCurrentFrame() : 0u;
		l_srcTex->SetCurrentState(l_texFrameIndex, l_srcTex->m_WriteState);
	}

	bool ExampleRenderingClientImpl::WriteCaptureToFile(const char* filename)
	{
		auto l_fmService = g_Engine->Get<FrameManagementService>();
		auto l_hwService = g_Engine->Get<GraphicsHardwareService>();
		l_hwService->SignalOnGPU(l_fmService->GetGlobalSemaphore(), GPUEngineType::Graphics);
		auto l_semVal = l_hwService->GetSemaphoreValue(GPUEngineType::Graphics);
		l_hwService->WaitOnCPU(l_semVal, GPUEngineType::Graphics);

		auto l_srcTex = static_cast<TextureComponent*>(FinalBlendPass::Get().GetResult());
		auto l_floatPixels = g_Engine->Get<TextureResourceService>()->ReadTextureBackToCPU(
			FinalBlendPass::Get().GetRenderPassComp(), l_srcTex);

		if (l_floatPixels.empty())
		{
			Log(Warning, "Capture: ReadTextureBackToCPU returned empty for ", filename);
			return false;
		}

		std::vector<uint8_t> l_uint8Pixels;
		l_uint8Pixels.reserve(l_floatPixels.size() * 4);
		// FinalBlendPass already writes gamma-encoded sRGB (AGX tonemap); no extra encode here.
		for (const auto& px : l_floatPixels)
		{
			l_uint8Pixels.push_back(uint8_t(255.99f * std::min(std::max(px.x, 0.0f), 1.0f)));
			l_uint8Pixels.push_back(uint8_t(255.99f * std::min(std::max(px.y, 0.0f), 1.0f)));
			l_uint8Pixels.push_back(uint8_t(255.99f * std::min(std::max(px.z, 0.0f), 1.0f)));
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

		// One-shot stats only — per-frame dump path would spam these on long `-dump_frames` runs.
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
