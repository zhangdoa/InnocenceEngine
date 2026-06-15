#include "ScreenCaptureService.h"

#include "AssetService.h"
#include "ConfigurationService.h"
#include "DevToggleRegistry.h"
#include "EditorService.h"
#include "FrameManagementService.h"
#include "GraphicsHardwareService.h"
#include "../RenderGraph/RenderGraphService.h"
#include "TextureResourceService.h"
#include "../Component/TextureComponent.h"

#include "../Engine.h"
#include "../Common/LogService.h"
#include "../Common/Math.h"

#include <chrono>
#include <filesystem>
#include <iomanip>
#include <sstream>

using namespace Inno;

namespace
{
    using Float4 = Math::Vec4;

    std::filesystem::path OutputDirectory()
    {
        return std::filesystem::path(
            g_Engine->Get<ConfigurationService>()->GetScreenshotOutputDir());
    }

    bool EnsureDirectory(const std::filesystem::path& dir, std::string& outError)
    {
        std::error_code l_dirEc;
        std::filesystem::create_directories(dir, l_dirEc);
        if (!l_dirEc)
            return true;
        outError = std::string("Screenshot: failed to create directory '")
            + dir.string() + "': " + l_dirEc.message();
        return false;
    }

    std::string TimestampFileStem()
    {
        const auto* l_cfg = g_Engine->Get<ConfigurationService>();
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
        l_nameStream << l_cfg->GetScreenshotFilePrefix()
            << std::put_time(&l_tm, l_cfg->GetScreenshotTimestampFormat().c_str())
            << "-" << std::setw(3) << std::setfill('0') << l_nowMs.count();
        return l_nameStream.str();
    }

    const char* ExtensionFor(TexturePixelDataType pixelType)
    {
        return (pixelType == TexturePixelDataType::Float16
            || pixelType == TexturePixelDataType::Float32) ? ".hdr" : ".png";
    }

    std::string ToAbsolutePath(const std::filesystem::path& relative)
    {
        std::error_code l_absEc;
        const std::filesystem::path l_absolutePath =
            std::filesystem::absolute(relative, l_absEc).make_preferred();
        if (!l_absEc)
            return l_absolutePath.string();
        return std::filesystem::path(relative).make_preferred().string();
    }

}

bool ScreenCaptureService::Setup(IServiceConfig* /*systemConfig*/)
{
    m_ObjectStatus = ObjectStatus::Activated;
    return true;
}

bool ScreenCaptureService::Initialize()
{
    DevToggleRegistry::RegisterAction("Screenshot", [this]() { RequestCapture(); });
    return true;
}

bool ScreenCaptureService::Terminate()
{
    m_ObjectStatus = ObjectStatus::Terminated;
    return true;
}

ObjectStatus ScreenCaptureService::GetStatus()
{
    return m_ObjectStatus;
}

void ScreenCaptureService::ResolveCanvas()
{
    const auto* l_cfg = g_Engine->Get<ConfigurationService>();
    auto* l_graph = g_Engine->Get<RenderGraphService>();
    auto* l_node = l_graph->FindNode(l_cfg->GetCanvasNodeName().c_str());
    m_Canvas = l_graph->GetResource(l_cfg->GetCanvasResourceName());
    m_CanvasOwner = l_node ? l_node->m_RenderPass : nullptr;
}

void ScreenCaptureService::RequestCapture()
{
    m_saveScreenCapture = true;
}

bool ScreenCaptureService::Update()
{
    ResolveCanvas();
    HandleScreenCapture();
    HandleAutoCaptureTriggers();
    return true;
}

void ScreenCaptureService::HandleScreenCapture()
{
    if (!m_saveScreenCapture)
        return;

    auto* l_srcTextureComp = static_cast<TextureComponent*>(m_Canvas);
    if (!l_srcTextureComp || !m_CanvasOwner)
    {
        Log(Warning, "Screenshot failed: present canvas unresolved.");
        m_saveScreenCapture = false;
        return;
    }

    auto* l_editorService = g_Engine->Get<EditorService>();
    const auto l_outputDir = OutputDirectory();

    std::string l_dirError;
    if (!EnsureDirectory(l_outputDir, l_dirError))
    {
        Log(Warning, "Screenshot failed: ", l_dirError.c_str());
        (void)l_editorService->BroadcastScreenshotSaved(false, std::string(), l_dirError);
        m_saveScreenCapture = false;
        return;
    }

    const std::string l_stem = TimestampFileStem();
    const char* l_extension = ExtensionFor(l_srcTextureComp->m_TextureDesc.PixelDataType);
    const std::filesystem::path l_relativePath = l_outputDir / (l_stem + l_extension);
    const std::string l_absolutePathStr = ToAbsolutePath(l_relativePath);

    auto l_textureData = g_Engine->Get<TextureResourceService>()->ReadTextureBackToCPU(
        m_CanvasOwner, l_srcTextureComp);
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
