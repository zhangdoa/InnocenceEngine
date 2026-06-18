#include "ConfigurationService.h"
#include "../ThirdParty/JSONWrapper/JSONWrapper.h"

using json = nlohmann::ordered_json;
using namespace Inno;

namespace Inno
{
    void ApplyEngine(json& j, ConfigurationService& svc)
    {
        if (!j.contains("engine") || !j["engine"].is_object())
            return;
        const auto& e = j["engine"];
        if (e.contains("engineMode") && e["engineMode"].is_string())
            svc.SetEngineModeFromString(e["engineMode"].get<std::string>());
        if (e.contains("graphicsService") && e["graphicsService"].is_string())
            svc.SetGraphicsServiceFromString(e["graphicsService"].get<std::string>());
        if (e.contains("isHeadless") && e["isHeadless"].is_boolean())
            svc.SetHeadless(e["isHeadless"].get<bool>());
        if (e.contains("isOffscreen") && e["isOffscreen"].is_boolean())
            svc.SetOffscreen(e["isOffscreen"].get<bool>());
        if (e.contains("isAudit") && e["isAudit"].is_boolean())
            svc.SetAudit(e["isAudit"].get<bool>());
        if (e.contains("isBakeMode") && e["isBakeMode"].is_boolean())
            svc.SetBakeMode(e["isBakeMode"].get<bool>());
        if (e.contains("applicationName") && e["applicationName"].is_string())
            svc.SetApplicationName(e["applicationName"].get<std::string>());
        if (e.contains("dataSubdir") && e["dataSubdir"].is_string())
            svc.SetDataSubdir(e["dataSubdir"].get<std::string>());
        if (e.contains("logLevel") && e["logLevel"].is_number_integer())
            svc.SetLogLevel(e["logLevel"].get<int>());
    }

    void ApplySession(json& j, ConfigurationService& svc)
    {
        if (!j.contains("session") || !j["session"].is_object())
            return;
        const auto& s = j["session"];
        if (s.contains("testCase") && s["testCase"].is_string())
            svc.SetTestCase(s["testCase"].get<std::string>());
        if (s.contains("totalFrames") && s["totalFrames"].is_number_integer())
            svc.SetTotalFrames(s["totalFrames"].get<int>());
        if (s.contains("captureFrame") && s["captureFrame"].is_number_integer())
            svc.SetCaptureFrame(s["captureFrame"].get<int>());
        if (s.contains("dumpFramesStart") && s["dumpFramesStart"].is_number_integer())
            svc.SetDumpFramesStart(s["dumpFramesStart"].get<int>());
        if (s.contains("dumpFramesEnd") && s["dumpFramesEnd"].is_number_integer())
            svc.SetDumpFramesEnd(s["dumpFramesEnd"].get<int>());
        if (s.contains("parentPID") && s["parentPID"].is_number_unsigned())
            svc.SetParentPID(s["parentPID"].get<uint32_t>());
        if (s.contains("enableGPUValidation") && s["enableGPUValidation"].is_boolean())
            svc.SetEnableGPUValidation(s["enableGPUValidation"].get<bool>());
        if (s.contains("enableGpuTimerLog") && s["enableGpuTimerLog"].is_boolean())
            svc.SetEnableGpuTimerLog(s["enableGpuTimerLog"].get<bool>());
        if (s.contains("renderGraph") && s["renderGraph"].is_string())
            svc.SetRenderGraphFile(s["renderGraph"].get<std::string>());
    }

    void ApplyScene(json& j, ConfigurationService& svc)
    {
        if (!j.contains("scene") || !j["scene"].is_object())
            return;
        const auto& s = j["scene"];
        if (s.contains("initialScene") && s["initialScene"].is_string())
            svc.SetInitialScene(s["initialScene"].get<std::string>());
        if (s.contains("serializeTest") && s["serializeTest"].is_string())
            svc.SetSerializeTest(s["serializeTest"].get<std::string>());
    }

    void ApplyCamera(json& j, ConfigurationService& svc)
    {
        if (!j.contains("camera") || !j["camera"].is_object())
            return;
        const auto& c = j["camera"];
        if (c.contains("orbitActive") && c["orbitActive"].is_boolean())
            svc.SetCameraOrbitActive(c["orbitActive"].get<bool>());
        if (c.contains("orbitPitchDeg") && c["orbitPitchDeg"].is_number())
            svc.SetCameraOrbitPitchDeg(c["orbitPitchDeg"].get<float>());
        if (c.contains("orbitRadius") && c["orbitRadius"].is_number())
            svc.SetCameraOrbitRadius(c["orbitRadius"].get<float>());
        if (c.contains("orbitDuration") && c["orbitDuration"].is_number_integer())
            svc.SetCameraOrbitDuration(c["orbitDuration"].get<int>());
    }

    void ApplyBake(json& j, ConfigurationService& svc)
    {
        if (!j.contains("bake") || !j["bake"].is_object())
            return;
        const auto& b = j["bake"];
        if (b.contains("inputs") && b["inputs"].is_string())
            svc.SetBakeInputs(b["inputs"].get<std::string>());
    }

    void ApplyScreenshot(json& j, ConfigurationService& svc)
    {
        if (!j.contains("screenshot") || !j["screenshot"].is_object())
            return;
        const auto& s = j["screenshot"];
        if (s.contains("outputDir") && s["outputDir"].is_string())
            svc.SetScreenshotOutputDir(s["outputDir"].get<std::string>());
        if (s.contains("filePrefix") && s["filePrefix"].is_string())
            svc.SetScreenshotFilePrefix(s["filePrefix"].get<std::string>());
        if (s.contains("timestampFormat") && s["timestampFormat"].is_string())
            svc.SetScreenshotTimestampFormat(s["timestampFormat"].get<std::string>());
        if (s.contains("canvasResource") && s["canvasResource"].is_string())
            svc.SetCanvasResourceName(s["canvasResource"].get<std::string>());
        if (s.contains("canvasNode") && s["canvasNode"].is_string())
            svc.SetCanvasNodeName(s["canvasNode"].get<std::string>());
        if (s.contains("dumpFramePattern") && s["dumpFramePattern"].is_string())
            svc.SetDumpFrameFilePattern(s["dumpFramePattern"].get<std::string>());
        if (s.contains("autoCaptureFileName") && s["autoCaptureFileName"].is_string())
            svc.SetAutoCaptureFileName(s["autoCaptureFileName"].get<std::string>());
    }

    void ApplyAutoCapture(json& j, ConfigurationService& svc)
    {
        if (!j.contains("autoCapture") || !j["autoCapture"].is_object())
            return;
        const auto& a = j["autoCapture"];
        if (a.contains("triggerFrame") && a["triggerFrame"].is_number_unsigned())
            svc.SetAutoCaptureTriggerFrame(a["triggerFrame"].get<uint32_t>());
    }

    void ApplyAudit(json& j, ConfigurationService& svc)
    {
        if (!j.contains("audit") || !j["audit"].is_object())
            return;
        const auto& a = j["audit"];
        if (a.contains("triggerAtFrame") && a["triggerAtFrame"].is_number_unsigned())
            svc.SetAuditTriggerAtFrame(a["triggerAtFrame"].get<uint32_t>());
        if (a.contains("passes") && a["passes"].is_array())
        {
            for (const auto& entry : a["passes"])
            {
                if (!entry.is_object())
                    continue;
                AuditPassEntry l_p;
                if (entry.contains("node") && entry["node"].is_string())
                    l_p.nodeName = entry["node"].get<std::string>();
                if (entry.contains("output") && entry["output"].is_string())
                    l_p.outputName = entry["output"].get<std::string>();
                if (entry.contains("file") && entry["file"].is_string())
                    l_p.fileName = entry["file"].get<std::string>();
                if (!l_p.nodeName.empty() && !l_p.fileName.empty())
                    svc.AddAuditPass(std::move(l_p));
            }
        }
    }

    void ApplyDebugView(json& j, ConfigurationService& svc)
    {
        if (!j.contains("debugView") || !j["debugView"].is_object())
            return;
        const auto& dv = j["debugView"];
        if (dv.contains("modes") && dv["modes"].is_array())
        {
            for (const auto& entry : dv["modes"])
            {
                if (!entry.is_object())
                    continue;
                DebugViewEntry l_d;
                if (entry.contains("name") && entry["name"].is_string())
                    l_d.name = entry["name"].get<std::string>();
                if (entry.contains("mode") && entry["mode"].is_number_unsigned())
                    l_d.modeId = entry["mode"].get<uint32_t>();
                if (!l_d.name.empty())
                    svc.AddDebugViewMode(std::move(l_d));
            }
        }
    }

    void ApplyBootstrap(json& j, ConfigurationService& svc)
    {
        if (!j.contains("bootstrap") || !j["bootstrap"].is_object())
            return;
        const auto& b = j["bootstrap"];
        if (b.contains("ambientCGSets") && b["ambientCGSets"].is_array())
        {
            for (const auto& entry : b["ambientCGSets"])
            {
                if (!entry.is_object())
                    continue;
                AmbientCGSetEntry l_s;
                if (entry.contains("setName") && entry["setName"].is_string())
                    l_s.setName = entry["setName"].get<std::string>();
                if (entry.contains("assetPath") && entry["assetPath"].is_string())
                    l_s.assetPath = entry["assetPath"].get<std::string>();
                if (!l_s.setName.empty())
                    svc.AddAmbientCGSet(std::move(l_s));
            }
        }
    }
}
