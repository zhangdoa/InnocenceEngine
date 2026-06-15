#pragma once
#include "../Interface/IService.h"
#include "../Engine.h"

#include <cstdint>
#include <string>
#include <vector>

namespace Inno
{
    struct AuditPassEntry
    {
        std::string nodeName;
        std::string outputName;
        std::string fileName;
    };

    struct DebugViewEntry
    {
        std::string name;
        uint32_t modeId;
    };

    struct AmbientCGSetEntry
    {
        std::string setName;
        std::string assetPath;
    };

    class ConfigurationService : public IService
    {
    public:
        INNO_CLASS_CONCRETE_NON_COPYABLE(ConfigurationService);

        bool Setup(IServiceConfig* systemConfig) override;
        bool Initialize() override;
        bool Update() override { return true; }
        bool Terminate() override;
        ObjectStatus GetStatus() override;

        bool LoadFromCommandLine(const char* cmdline);
        bool LoadFromFile(const char* path);
        void ApplyDefaults();

        bool IsOffscreen() const;
        bool IsHeadless() const;
        bool IsAudit() const;
        bool IsBakeMode() const;
        bool IsAutoExit() const;
        EngineMode GetEngineMode() const;
        GraphicsService GetGraphicsService() const;
        int GetLogLevel() const;
        const std::string& GetApplicationName() const;
        const std::string& GetDataSubdir() const;

        int GetTotalFrames() const;
        int GetMaxFrames() const;
        int GetReloadAtFrame() const;
        int GetCaptureFrame() const;
        int GetDumpFramesStart() const;
        int GetDumpFramesEnd() const;
        bool IsCameraOrbitActive() const;
        float GetCameraOrbitPitchDeg() const;
        float GetCameraOrbitRadius() const;
        int GetCameraOrbitDuration() const;
        bool IsEnableGPUValidation() const;
        bool IsEnableGpuTimerLog() const;
        uint32_t GetParentPID() const;
        uint32_t GetAutoCaptureTriggerFrame() const;
        const std::string& GetTestCase() const;
        const std::string& GetInitialScene() const;
        const std::string& GetSerializeTest() const;
        const std::string& GetBakeInputs() const;
        const std::string& GetRenderGraphFile() const;
        void SetSerializeTestResult(int result);
        int GetSerializeTestResult() const;

        const std::string& GetScreenshotOutputDir() const;
        const std::string& GetScreenshotFilePrefix() const;
        const std::string& GetScreenshotTimestampFormat() const;
        const std::string& GetDumpFrameFilePattern() const;
        const std::string& GetAutoCaptureFileName() const;
        const std::string& GetCanvasResourceName() const;
        const std::string& GetCanvasNodeName() const;

        const std::vector<AuditPassEntry>& GetAuditPasses() const;
        const std::vector<DebugViewEntry>& GetDebugViewModes() const;
        const std::vector<AmbientCGSetEntry>& GetAmbientCGSets() const;
        uint32_t GetAuditTriggerAtFrame() const;

        // Setters used by the in-TU parse dispatcher. Public so the
        // loader (private to ConfigurationService.cpp) can call them
        // without the friend machinery. External callers shouldn't use
        // these — go through the loaded values via the Get* accessors.
        void SetEngineModeFromString(const std::string& s);
        void SetGraphicsServiceFromString(const std::string& s);
        void SetHeadless(bool v);
        void SetOffscreen(bool v);
        void SetAudit(bool v);
        void SetBakeMode(bool v);
        void SetAutoExit(bool v);
        void SetApplicationName(const std::string& s);
        void SetDataSubdir(const std::string& s);
        void SetLogLevel(int v);
        void SetTestCase(const std::string& s);
        void SetMaxFrames(int v);
        void SetTotalFrames(int v);
        void SetReloadAtFrame(int v);
        void SetCaptureFrame(int v);
        void SetDumpFramesStart(int v);
        void SetDumpFramesEnd(int v);
        void SetCameraOrbitActive(bool v);
        void SetCameraOrbitPitchDeg(float v);
        void SetCameraOrbitRadius(float v);
        void SetCameraOrbitDuration(int v);
        void SetEnableGPUValidation(bool v);
        void SetEnableGpuTimerLog(bool v);
        void SetParentPID(uint32_t v);
        void SetAutoCaptureTriggerFrame(uint32_t v);
        void SetInitialScene(const std::string& s);
        void SetSerializeTest(const std::string& s);
        void SetBakeInputs(const std::string& s);
        void SetRenderGraphFile(const std::string& s);
        void SetScreenshotOutputDir(const std::string& s);
        void SetScreenshotFilePrefix(const std::string& s);
        void SetScreenshotTimestampFormat(const std::string& s);
        void SetDumpFrameFilePattern(const std::string& s);
        void SetAutoCaptureFileName(const std::string& s);
        void SetCanvasResourceName(const std::string& s);
        void SetCanvasNodeName(const std::string& s);
        void SetAuditTriggerAtFrame(uint32_t v);
        void AddAuditPass(AuditPassEntry p);
        void AddDebugViewMode(DebugViewEntry d);
        void AddAmbientCGSet(AmbientCGSetEntry a);



    private:
        bool m_Offscreen = false;
        bool m_Headless = false;
        bool m_Audit = false;
        bool m_BakeMode = false;
        bool m_AutoExit = false;
        EngineMode m_EngineMode = EngineMode::Host;
        GraphicsService m_GraphicsService = GraphicsService::DX12;
        int m_LogLevel = 1;
        std::string m_ApplicationName = "InnocenceEngine";
        std::string m_DataSubdir = "Data";

        int m_TotalFrames = 0;
        int m_MaxFrames = 0;
        int m_ReloadAtFrame = 0;
        int m_CaptureFrame = -1;
        int m_DumpFramesStart = -1;
        int m_DumpFramesEnd = -1;
        bool m_CameraOrbitActive = false;
        float m_CameraOrbitPitchDeg = 0.0f;
        float m_CameraOrbitRadius = 0.0f;
        int m_CameraOrbitDuration = 0;
        bool m_EnableGPUValidation = false;
        bool m_EnableGpuTimerLog = false;
        uint32_t m_ParentPID = 0;
        uint32_t m_AutoCaptureTriggerFrame = 0;
        std::string m_TestCase;
        std::string m_InitialScene;
        std::string m_SerializeTest;
        std::string m_BakeInputs;
        std::string m_RenderGraphFile;
        int m_SerializeTestResult = 0;

        std::string m_ScreenshotOutputDir = "Captures/Screenshots";
        std::string m_ScreenshotFilePrefix = "screenshot_";
        std::string m_ScreenshotTimestampFormat = "%Y-%m-%d_%H-%M-%S";
        std::string m_DumpFrameFilePattern = "gpu_output_%04u.png";
        std::string m_AutoCaptureFileName = "gpu_output.png";
        std::string m_CanvasResourceName = "Final Blend Pass Result";
        std::string m_CanvasNodeName = "FinalBlendPass";

        std::vector<AuditPassEntry> m_AuditPasses;
        uint32_t m_AuditTriggerAtFrame = 0;

        std::vector<DebugViewEntry> m_DebugViewModes;
        std::vector<AmbientCGSetEntry> m_AmbientCGSets;

        std::string m_LoadedPath;
        ObjectStatus m_ObjectStatus = ObjectStatus::Invalid;
    };
}
