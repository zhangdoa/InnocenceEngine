#include "ScreenCaptureService.h"

#include "AssetService.h"
#include "ConfigurationService.h"
#include "FrameManagementService.h"
#include "GraphicsHardwareService.h"
#include "TextureResourceService.h"
#include "../Component/TextureComponent.h"

#include "../Engine.h"
#include "../Common/LogService.h"
#include "../Common/Math.h"

#include <algorithm>

using namespace Inno;

namespace
{
    using Float4 = Math::Vec4;

    void WaitForGraphicsQueue()
    {
        auto* l_fmService = g_Engine->Get<FrameManagementService>();
        auto* l_hwService = g_Engine->Get<GraphicsHardwareService>();
        l_hwService->SignalOnGPU(l_fmService->GetGlobalSemaphore(), GPUEngineType::Graphics);
        const auto l_semVal = l_hwService->GetSemaphoreValue(GPUEngineType::Graphics);
        l_hwService->WaitOnCPU(l_semVal, GPUEngineType::Graphics);
    }

    Inno::Array<uint8_t> ToUint8SRGBA(const Inno::Array<Float4>& floatPixels)
    {
        Inno::Array<uint8_t> l_uint8Pixels;
        l_uint8Pixels.reserve(floatPixels.size() * 4);
        for (const auto& px : floatPixels)
        {
            l_uint8Pixels.push_back(uint8_t(255.99f * std::min(std::max(px.x, 0.0f), 1.0f)));
            l_uint8Pixels.push_back(uint8_t(255.99f * std::min(std::max(px.y, 0.0f), 1.0f)));
            l_uint8Pixels.push_back(uint8_t(255.99f * std::min(std::max(px.z, 0.0f), 1.0f)));
            l_uint8Pixels.push_back(uint8_t(255));
        }
        return l_uint8Pixels;
    }

    TextureDesc CaptureWriteDesc(const TextureComponent& srcTex)
    {
        TextureDesc l_desc = srcTex.m_TextureDesc;
        l_desc.PixelDataType = TexturePixelDataType::UByte;
        l_desc.PixelDataFormat = TexturePixelDataFormat::RGBA;
        l_desc.Sampler = TextureSampler::Sampler2D;
        return l_desc;
    }

    void LogReadbackStats(const Inno::Array<Float4>& pixels)
    {
        if (pixels.empty())
            return;
        size_t l_zeroCount = 0;
        size_t l_nonZeroCount = 0;
        float l_sumR = 0, l_sumG = 0, l_sumB = 0;
        float l_maxR = 0, l_maxG = 0, l_maxB = 0;
        for (const auto& px : pixels)
        {
            const bool l_isZero = (px.x == 0.0f && px.y == 0.0f && px.z == 0.0f);
            if (l_isZero)
            {
                ++l_zeroCount;
                continue;
            }
            ++l_nonZeroCount;
            l_sumR += px.x; l_sumG += px.y; l_sumB += px.z;
            if (px.x > l_maxR) l_maxR = px.x;
            if (px.y > l_maxG) l_maxG = px.y;
            if (px.z > l_maxB) l_maxB = px.z;
        }
        const float l_total = static_cast<float>(pixels.size());
        Log(Success, "ReadbackStats: total=", pixels.size(),
            " zero=", l_zeroCount, " nonZero=", l_nonZeroCount,
            " mean=(", l_sumR / l_total, ",", l_sumG / l_total, ",", l_sumB / l_total, ")",
            " max=(", l_maxR, ",", l_maxG, ",", l_maxB, ")");
    }
}

bool ScreenCaptureService::IsSerializeTestMode() const
{
    return !g_Engine->Get<ConfigurationService>()->GetSerializeTest().empty();
}

uint32_t ScreenCaptureService::ResolveTriggerFrame() const
{
    const auto* l_cfg = g_Engine->Get<ConfigurationService>();
    const int l_total = l_cfg->GetTotalFrames();
    if (l_total > 0)
        return static_cast<uint32_t>(l_total);
    return l_cfg->GetAutoCaptureTriggerFrame();
}

bool ScreenCaptureService::IsInsideDumpWindow(uint32_t frameCount) const
{
    if (IsSerializeTestMode())
        return false;
    const auto* l_cfg = g_Engine->Get<ConfigurationService>();
    const int l_start = l_cfg->GetDumpFramesStart();
    const int l_end = l_cfg->GetDumpFramesEnd();
    if (l_start < 0 || l_end < l_start)
        return false;
    return frameCount >= static_cast<uint32_t>(l_start)
        && frameCount <= static_cast<uint32_t>(l_end);
}

void ScreenCaptureService::RunDumpFrame(uint32_t frameCount)
{
    AlignTrackerForMidFrameReadback();
    char l_buf[256];
    const std::string l_pattern = g_Engine->Get<ConfigurationService>()->GetDumpFrameFilePattern();
    snprintf(l_buf, sizeof(l_buf), l_pattern.c_str(), frameCount);
    WriteCaptureToFile(l_buf);
}

void ScreenCaptureService::RunOneShotIfReady(uint32_t triggerAtFrame)
{
    if (triggerAtFrame == 0u)
        return;
    if (m_autoCaptureWritten)
        return;
    if (g_Engine->Get<FrameManagementService>()->GetSteadyStateRelativeFrameCount() < triggerAtFrame)
        return;
    AlignTrackerForMidFrameReadback();
    TryWriteAutoCapture();
}

void ScreenCaptureService::HandleAutoCaptureTriggers()
{
    // Single canonical session clock — no private per-service frame counter.
    const uint32_t l_sessionFrame =
        g_Engine->Get<FrameManagementService>()->GetSteadyStateRelativeFrameCount();

    if (IsInsideDumpWindow(l_sessionFrame))
        RunDumpFrame(l_sessionFrame);

    RunOneShotIfReady(ResolveTriggerFrame());
}

void ScreenCaptureService::AlignTrackerForMidFrameReadback()
{
    auto* l_srcTex = static_cast<TextureComponent*>(m_Canvas);
    if (!l_srcTex)
        return;
    auto* l_fmService = g_Engine->Get<FrameManagementService>();
    const auto l_texFrameIndex =
        l_srcTex->m_TextureDesc.IsMultiBuffer ? l_fmService->GetCurrentFrame() : 0u;
    l_srcTex->SetCurrentState(l_texFrameIndex, l_srcTex->m_WriteState);
}

bool ScreenCaptureService::WriteCaptureToFile(const char* filename)
{
    auto* l_srcTex = static_cast<TextureComponent*>(m_Canvas);
    if (!l_srcTex || !m_CanvasOwner)
    {
        Log(Warning, "Capture: present canvas unresolved for ", filename);
        return false;
    }

    WaitForGraphicsQueue();

    auto l_floatPixels = g_Engine->Get<TextureResourceService>()->ReadTextureBackToCPU(
        m_CanvasOwner, l_srcTex);
    if (l_floatPixels.empty())
    {
        Log(Warning, "Capture: ReadTextureBackToCPU returned empty for ", filename);
        return false;
    }

    auto l_uint8Pixels = ToUint8SRGBA(l_floatPixels);
    const auto l_desc = CaptureWriteDesc(*l_srcTex);

    if (!g_Engine->Get<AssetService>()->Save(filename, l_desc, l_uint8Pixels.data()))
    {
        Log(Warning, "Capture: failed to write ", filename);
        return false;
    }
    Log(Success, "Capture: ", filename, " written.");
    return true;
}

void ScreenCaptureService::TryWriteAutoCapture()
{
    if (m_autoCaptureWritten)
        return;
    m_autoCaptureWritten = true;

    ResolveCanvas();
    auto* l_srcTex = static_cast<TextureComponent*>(m_Canvas);
    if (l_srcTex && m_CanvasOwner)
    {
        auto l_floatPixels = g_Engine->Get<TextureResourceService>()->ReadTextureBackToCPU(
            m_CanvasOwner, l_srcTex);
        LogReadbackStats(l_floatPixels);
    }

    WriteCaptureToFile(g_Engine->Get<ConfigurationService>()->GetAutoCaptureFileName().c_str());
}
