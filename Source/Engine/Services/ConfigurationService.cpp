#include "ConfigurationService.h"
#include "../Common/LogService.h"
#include "../ThirdParty/JSONWrapper/JSONWrapper.h"

using json = nlohmann::ordered_json;
using namespace Inno;
// Apply* per section live in ConfigurationService_Parse.cpp. The .h
// declares no dispatcher; the .cpp calls them directly through the Inno namespace.
namespace Inno
{
    void ApplyEngine(json& j, ConfigurationService& svc);
    void ApplySession(json& j, ConfigurationService& svc);
    void ApplyScene(json& j, ConfigurationService& svc);
    void ApplyCamera(json& j, ConfigurationService& svc);
    void ApplyBake(json& j, ConfigurationService& svc);
    void ApplyScreenshot(json& j, ConfigurationService& svc);
    void ApplyAutoCapture(json& j, ConfigurationService& svc);
    void ApplyAudit(json& j, ConfigurationService& svc);
    void ApplyDebugView(json& j, ConfigurationService& svc);
    void ApplyBootstrap(json& j, ConfigurationService& svc);
}

namespace
{
    const char* ExtractPathArg(const char* cmdline)
    {
        if (!cmdline)
            return nullptr;
        const std::string l_cmd(cmdline);
        const auto l_pos = l_cmd.find("-c ");
        if (l_pos == std::string::npos)
            return nullptr;
        const std::string l_rem = l_cmd.substr(l_pos + 3);
        const auto l_start = l_rem.find_first_not_of(' ');
        if (l_start == std::string::npos)
            return nullptr;
        const auto l_end = l_rem.find(' ', l_start);
        std::string l_path = l_rem.substr(l_start,
            l_end == std::string::npos ? std::string::npos : l_end - l_start);
        if (l_path.empty())
            return nullptr;
        static thread_local std::string l_out;
        l_out = l_path;
        return l_out.c_str();
    }
}

void ConfigurationService::ApplyDefaults()
{
    m_Offscreen = false;
    m_Headless = false;
    m_Audit = false;
    m_BakeMode = false;
    m_EngineMode = EngineMode::Host;
    m_GraphicsService = GraphicsService::DX12;
    m_LogLevel = 1;
    m_ApplicationName = "InnocenceEngine";
    m_DataSubdir = "Data";
    m_TotalFrames = 0;
    m_CaptureFrame = -1;
    m_DumpFramesStart = -1;
    m_DumpFramesEnd = -1;
    m_CameraOrbitActive = false;
    m_CameraOrbitPitchDeg = 0.0f;
    m_CameraOrbitRadius = 0.0f;
    m_CameraOrbitDuration = 0;
    m_EnableGPUValidation = false;
    m_EnableGpuTimerLog = false;
    m_ParentPID = 0;
    m_AutoCaptureTriggerFrame = 0;
    m_TestCase.clear();
    m_InitialScene.clear();
    m_SerializeTest.clear();
    m_BakeInputs.clear();
    m_RenderGraphFile.clear();
    m_SerializeTestResult = 0;
    m_ScreenshotOutputDir = "Captures/Screenshots";
    m_ScreenshotFilePrefix = "screenshot_";
    m_ScreenshotTimestampFormat = "%Y-%m-%d_%H-%M-%S";
    m_DumpFrameFilePattern = "gpu_output_%04u.png";
    m_AutoCaptureFileName = "gpu_output.png";
    m_CanvasResourceName = "Final Blend Pass Result";
    m_CanvasNodeName = "FinalBlendPass";
    m_AuditPasses.clear();
    m_AuditTriggerAtFrame = 0;
    m_DebugViewModes.clear();
    m_AmbientCGSets.clear();
}

bool ConfigurationService::Setup(IServiceConfig* /*systemConfig*/)
{
    ApplyDefaults();
    m_ObjectStatus = ObjectStatus::Created;
    return true;
}

bool ConfigurationService::Initialize()
{
    g_Engine->Get<LogService>()->SetDefaultLogLevel(static_cast<LogLevel>(m_LogLevel));
    m_ObjectStatus = ObjectStatus::Activated;
    return true;
}

bool ConfigurationService::Terminate()
{
    m_ObjectStatus = ObjectStatus::Terminated;
    return true;
}

ObjectStatus ConfigurationService::GetStatus()
{
    return m_ObjectStatus;
}

bool ConfigurationService::LoadFromCommandLine(const char* cmdline)
{
    const char* l_path = ExtractPathArg(cmdline);
    if (!l_path)
    {
        Log(Warning, "ConfigurationService: no '-c <path>' argument; using built-in defaults.");
        return false;
    }
    return LoadFromFile(l_path);
}

bool ConfigurationService::LoadFromFile(const char* path)
{
    ApplyDefaults();
    if (!path || !*path)
        return false;
    m_LoadedPath = path;

    json j;
    if (!JSONWrapper::Load(path, j))
    {
        Log(Error, "ConfigurationService: failed to load '", path, "'; using defaults.");
        return false;
    }

    ApplyEngine(j, *this);
    ApplySession(j, *this);
    ApplyScene(j, *this);
    ApplyCamera(j, *this);
    ApplyBake(j, *this);
    ApplyScreenshot(j, *this);
    ApplyAutoCapture(j, *this);
    ApplyAudit(j, *this);
    ApplyDebugView(j, *this);
    ApplyBootstrap(j, *this);

    Log(Success, "ConfigurationService: loaded '", path, "'.");
    return true;
}

bool ConfigurationService::IsOffscreen() const          { return m_Offscreen; }
bool ConfigurationService::IsHeadless() const           { return m_Headless; }
bool ConfigurationService::IsAudit() const              { return m_Audit; }
bool ConfigurationService::IsBakeMode() const           { return m_BakeMode; }
EngineMode ConfigurationService::GetEngineMode() const  { return m_EngineMode; }
GraphicsService ConfigurationService::GetGraphicsService() const { return m_GraphicsService; }
int ConfigurationService::GetLogLevel() const           { return m_LogLevel; }
const std::string& ConfigurationService::GetApplicationName() const { return m_ApplicationName; }
const std::string& ConfigurationService::GetDataSubdir() const { return m_DataSubdir; }

int ConfigurationService::GetTotalFrames() const        { return m_TotalFrames; }
int ConfigurationService::GetCaptureFrame() const       { return m_CaptureFrame; }
int ConfigurationService::GetDumpFramesStart() const    { return m_DumpFramesStart; }
int ConfigurationService::GetDumpFramesEnd() const      { return m_DumpFramesEnd; }
bool ConfigurationService::IsCameraOrbitActive() const  { return m_CameraOrbitActive; }
float ConfigurationService::GetCameraOrbitPitchDeg() const { return m_CameraOrbitPitchDeg; }
float ConfigurationService::GetCameraOrbitRadius() const { return m_CameraOrbitRadius; }
int ConfigurationService::GetCameraOrbitDuration() const { return m_CameraOrbitDuration; }
bool ConfigurationService::IsEnableGPUValidation() const { return m_EnableGPUValidation; }
bool ConfigurationService::IsEnableGpuTimerLog() const { return m_EnableGpuTimerLog; }
uint32_t ConfigurationService::GetParentPID() const    { return m_ParentPID; }
uint32_t ConfigurationService::GetAutoCaptureTriggerFrame() const { return m_AutoCaptureTriggerFrame; }
const std::string& ConfigurationService::GetTestCase() const    { return m_TestCase; }
const std::string& ConfigurationService::GetInitialScene() const { return m_InitialScene; }
const std::string& ConfigurationService::GetSerializeTest() const { return m_SerializeTest; }
const std::string& ConfigurationService::GetBakeInputs() const   { return m_BakeInputs; }
const std::string& ConfigurationService::GetRenderGraphFile() const { return m_RenderGraphFile; }
void ConfigurationService::SetSerializeTestResult(int result) { m_SerializeTestResult = result; }
int ConfigurationService::GetSerializeTestResult() const       { return m_SerializeTestResult; }

const std::string& ConfigurationService::GetScreenshotOutputDir() const { return m_ScreenshotOutputDir; }
const std::string& ConfigurationService::GetScreenshotFilePrefix() const { return m_ScreenshotFilePrefix; }
const std::string& ConfigurationService::GetScreenshotTimestampFormat() const { return m_ScreenshotTimestampFormat; }
const std::string& ConfigurationService::GetDumpFrameFilePattern() const { return m_DumpFrameFilePattern; }
const std::string& ConfigurationService::GetAutoCaptureFileName() const { return m_AutoCaptureFileName; }
const std::string& ConfigurationService::GetCanvasResourceName() const { return m_CanvasResourceName; }
const std::string& ConfigurationService::GetCanvasNodeName() const { return m_CanvasNodeName; }

const std::vector<AuditPassEntry>& ConfigurationService::GetAuditPasses() const { return m_AuditPasses; }
uint32_t ConfigurationService::GetAuditTriggerAtFrame() const { return m_AuditTriggerAtFrame; }
const std::vector<DebugViewEntry>& ConfigurationService::GetDebugViewModes() const { return m_DebugViewModes; }
const std::vector<AmbientCGSetEntry>& ConfigurationService::GetAmbientCGSets() const { return m_AmbientCGSets; }

void ConfigurationService::SetEngineModeFromString(const std::string& s)         { if (s == "Host") m_EngineMode = EngineMode::Host; else if (s == "Slave") m_EngineMode = EngineMode::Slave; else if (s == "Sidecar") m_EngineMode = EngineMode::Sidecar; }
void ConfigurationService::SetGraphicsServiceFromString(const std::string& s)     { if (s == "DX12") m_GraphicsService = GraphicsService::DX12; else if (s == "VK") m_GraphicsService = GraphicsService::VK; else if (s == "MT") m_GraphicsService = GraphicsService::MT; }
void ConfigurationService::SetHeadless(bool v)                                    { m_Headless = v; }
void ConfigurationService::SetOffscreen(bool v)                                   { m_Offscreen = v; }
void ConfigurationService::SetAudit(bool v)                                       { m_Audit = v; }
void ConfigurationService::SetBakeMode(bool v)                                    { m_BakeMode = v; }
void ConfigurationService::SetApplicationName(const std::string& s)                 { m_ApplicationName = s; }
void ConfigurationService::SetDataSubdir(const std::string& s)                     { m_DataSubdir = s; }
void ConfigurationService::SetLogLevel(int v)                                     { m_LogLevel = v; }
void ConfigurationService::SetTestCase(const std::string& s)                       { m_TestCase = s; }
void ConfigurationService::SetTotalFrames(int v)                                  { m_TotalFrames = v; }
void ConfigurationService::SetCaptureFrame(int v)                                 { m_CaptureFrame = v; }
void ConfigurationService::SetDumpFramesStart(int v)                              { m_DumpFramesStart = v; }
void ConfigurationService::SetDumpFramesEnd(int v)                                { m_DumpFramesEnd = v; }
void ConfigurationService::SetCameraOrbitActive(bool v)                            { m_CameraOrbitActive = v; }
void ConfigurationService::SetCameraOrbitPitchDeg(float v)                        { m_CameraOrbitPitchDeg = v; }
void ConfigurationService::SetCameraOrbitRadius(float v)                          { m_CameraOrbitRadius = v; }
void ConfigurationService::SetCameraOrbitDuration(int v)                          { m_CameraOrbitDuration = v; }
void ConfigurationService::SetEnableGPUValidation(bool v)                        { m_EnableGPUValidation = v; }
void ConfigurationService::SetEnableGpuTimerLog(bool v)                           { m_EnableGpuTimerLog = v; }
void ConfigurationService::SetParentPID(uint32_t v)                               { m_ParentPID = v; }
void ConfigurationService::SetAutoCaptureTriggerFrame(uint32_t v)                 { m_AutoCaptureTriggerFrame = v; }
void ConfigurationService::SetInitialScene(const std::string& s)                  { m_InitialScene = s; }
void ConfigurationService::SetSerializeTest(const std::string& s)                  { m_SerializeTest = s; }
void ConfigurationService::SetBakeInputs(const std::string& s)                     { m_BakeInputs = s; }
void ConfigurationService::SetRenderGraphFile(const std::string& s)                { m_RenderGraphFile = s; }
void ConfigurationService::SetScreenshotOutputDir(const std::string& s)            { m_ScreenshotOutputDir = s; }
void ConfigurationService::SetScreenshotFilePrefix(const std::string& s)            { m_ScreenshotFilePrefix = s; }
void ConfigurationService::SetScreenshotTimestampFormat(const std::string& s)      { m_ScreenshotTimestampFormat = s; }
void ConfigurationService::SetDumpFrameFilePattern(const std::string& s)          { m_DumpFrameFilePattern = s; }
void ConfigurationService::SetAutoCaptureFileName(const std::string& s)             { m_AutoCaptureFileName = s; }
void ConfigurationService::SetCanvasResourceName(const std::string& s)             { m_CanvasResourceName = s; }
void ConfigurationService::SetCanvasNodeName(const std::string& s)                 { m_CanvasNodeName = s; }
void ConfigurationService::SetAuditTriggerAtFrame(uint32_t v)                     { m_AuditTriggerAtFrame = v; }
void ConfigurationService::AddAuditPass(AuditPassEntry p)                          { m_AuditPasses.push_back(std::move(p)); }
void ConfigurationService::AddDebugViewMode(DebugViewEntry d)                      { m_DebugViewModes.push_back(std::move(d)); }
void ConfigurationService::AddAmbientCGSet(AmbientCGSetEntry a)                    { m_AmbientCGSets.push_back(std::move(a)); }
