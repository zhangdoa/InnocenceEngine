#include "AuditDumpService.h"

#include "AssetService.h"
#include "FrameManagementService.h"
#include "GraphicsHardwareService.h"
#include "SceneService.h"
#include "TextureResourceService.h"
#include "../RenderGraph/RenderGraphService.h"

#include "../Component/TextureComponent.h"
#include "../Component/RenderPassComponent.h"

#include "../Engine.h"
#include "../Common/LogService.h"

#include <cstdlib>
#include <functional>

using namespace Inno;

namespace
{
    void DumpTexture(const char* filename, RenderPassComponent* rp, TextureComponent* tc)
    {
        if (!tc)
        {
            Log(Warning, "AuditDump: null TextureComponent for ", filename);
            return;
        }
        auto l_pixels = g_Engine->Get<TextureResourceService>()->ReadTextureBackToCPU(rp, tc);
        if (l_pixels.empty())
        {
            Log(Error, "AuditDump: empty readback for ", filename);
            return;
        }
        for (size_t pi = 0; pi < l_pixels.size(); pi++)
        {
            if (l_pixels[pi].x != 0.0f || l_pixels[pi].y != 0.0f || l_pixels[pi].z != 0.0f)
            {
                const uint32_t l_y = static_cast<uint32_t>(pi / tc->m_TextureDesc.Width);
                const uint32_t l_x = static_cast<uint32_t>(pi % tc->m_TextureDesc.Width);
                Log(Verbose, "AuditDump: ", filename, " first-nonzero y=", l_y, " x=", l_x,
                    " rgba=(", l_pixels[pi].x, ",", l_pixels[pi].y, ",", l_pixels[pi].z, ",", l_pixels[pi].w, ")");
                break;
            }
        }
        TextureDesc l_desc = tc->m_TextureDesc;
        l_desc.PixelDataType = TexturePixelDataType::Float32;
        g_Engine->Get<AssetService>()->Save(filename, l_desc, l_pixels.data());
        Log(Success, "AuditDump: saved ", filename);
    }
}

bool AuditDumpService::Setup(IServiceConfig* /*systemConfig*/)
{
    auto* l_cfg = g_Engine->Get<ConfigurationService>();
    m_Enabled = l_cfg->IsAudit();
    m_TriggerAtFrame = l_cfg->GetAuditTriggerAtFrame();
    m_Passes = l_cfg->GetAuditPasses();
    m_ObjectStatus = ObjectStatus::Created;
    return true;
}

bool AuditDumpService::Initialize()
{
    if (!m_Enabled)
    {
        m_ObjectStatus = ObjectStatus::Activated;
        return true;
    }
    m_ObjectStatus = ObjectStatus::Activated;

    m_SceneLoadedCallback = [this]() { m_SceneLoaded.store(true); };
    g_Engine->Get<SceneService>()->AddSceneLoadedCallback(&m_SceneLoadedCallback);
    return true;
}

bool AuditDumpService::Update()
{
    if (!m_Enabled)
        return true;
    if (!m_SceneLoaded.load())
        return true;
    if (m_FrameCount < m_TriggerAtFrame)
    {
        ++m_FrameCount;
        return true;
    }
    ++m_FrameCount;
    RunDump();
    return true;
}

bool AuditDumpService::Terminate()
{
    m_ObjectStatus = ObjectStatus::Terminated;
    return true;
}

ObjectStatus AuditDumpService::GetStatus()
{
    return m_ObjectStatus;
}

void AuditDumpService::RunDump()
{
    auto* l_hwService = g_Engine->Get<GraphicsHardwareService>();
    l_hwService->WaitOnCPU(l_hwService->GetSemaphoreValue(GPUEngineType::Graphics), GPUEngineType::Graphics);
    l_hwService->WaitOnCPU(l_hwService->GetSemaphoreValue(GPUEngineType::Compute), GPUEngineType::Compute);

    auto* l_graph = g_Engine->Get<RenderGraphService>();
    for (const auto& l_pass : m_Passes)
    {
        auto* l_node = l_graph->FindNode(l_pass.nodeName.c_str());
        auto* l_rp = l_node ? l_node->m_RenderPass : nullptr;
        auto* l_tex = static_cast<TextureComponent*>(l_graph->GetResource(l_pass.outputName.c_str()));
        DumpTexture(l_pass.fileName.c_str(), l_rp, l_tex);
    }

    Log(Success, "AuditDump complete.");
    std::exit(0);
}
