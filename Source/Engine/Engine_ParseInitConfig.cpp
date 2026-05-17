#include "Engine_Internal.h"
#include "Engine_ParseInitConfig_Helpers.h"
#include "Common/LogService.h"

using namespace Inno;

InitConfig Engine::ParseInitConfig(const std::string& arg)
{
	InitConfig l_result;

	if (arg == "")
	{
		Log(Warning, "No arguments found, use default settings.");
		return l_result;
	}

	auto l_engineModeArgPos = arg.find("mode");
	auto l_sidecarArgPos = arg.find("sidecar");

	if (l_engineModeArgPos == std::string::npos)
	{
		if (l_sidecarArgPos == std::string::npos)
		{
			Log(Warning, "No engine mode argument found, use default game mode.");
		}
	}
	else
	{
		std::string l_engineModeArguments = arg.substr(l_engineModeArgPos + 5);
		l_engineModeArguments = l_engineModeArguments.substr(0, 1);

		if (l_engineModeArguments == "0")
		{
			l_result.engineMode = EngineMode::Host;
			Log(Success, "Launch in host mode, engine will handle OS event.");
		}
		else if (l_engineModeArguments == "1")
		{
			l_result.engineMode = EngineMode::Slave;
			Log(Success, "Launch in slave mode, engine requires client handle OS event.");
		}
		else if (l_engineModeArguments == "2")
		{
			l_result.engineMode = EngineMode::Sidecar;
			Log(Success, "Launch in sidecar mode, engine will be controlled by external process.");
		}
		else
		{
			Log(Warning, "Unsupported engine mode.");
		}
	}

	auto l_graphicsServiceArgPos = arg.find("renderer");

	if (l_graphicsServiceArgPos == std::string::npos)
	{
		Log(Error, "No rendering backend argument found.");
	}
	else
	{
		std::string l_rendererArguments = arg.substr(l_graphicsServiceArgPos + 9);
		l_rendererArguments = l_rendererArguments.substr(0, 1);

		if (l_rendererArguments == "0")
		{
#if defined INNO_RENDERER_DIRECTX
			l_result.graphicsService = GraphicsService::DX12;
#else
			Log(Warning, "DirectX 12 is not supported on current platform.");
#endif
		}
		else if (l_rendererArguments == "1")
		{
#if defined INNO_RENDERER_VULKAN
			l_result.graphicsService = GraphicsService::VK;
#else
			Log(Warning, "Vulkan is not supported on current platform.");
#endif
		}
		else if (l_rendererArguments == "2")
		{
#if defined INNO_RENDERER_METAL
			l_result.graphicsService = GraphicsService::MT;
#else
			Log(Warning, "Metal is not supported on current platform.");
#endif
		}
	}

	auto l_logLevelArgPos = arg.find("loglevel");
	if (l_logLevelArgPos == std::string::npos)
	{
		Get<LogService>()->SetDefaultLogLevel(LogLevel::Success);
	}
	else
	{
		std::string l_logLevelArguments = arg.substr(l_logLevelArgPos + 9);
		l_logLevelArguments = l_logLevelArguments.substr(0, 1);

		if (l_logLevelArguments == "0")
		{
			Get<LogService>()->SetDefaultLogLevel(LogLevel::Verbose);
		}
		else if (l_logLevelArguments == "1")
		{
			Get<LogService>()->SetDefaultLogLevel(LogLevel::Success);
		}
		else if (l_logLevelArguments == "2")
		{
			Get<LogService>()->SetDefaultLogLevel(LogLevel::Warning);
		}
		else if (l_logLevelArguments == "3")
		{
			Get<LogService>()->SetDefaultLogLevel(LogLevel::Error);
		}
		else
		{
			Log(Warning, "Unsupported log level.");
		}
	}

	auto l_headlessArgPos = arg.find("headless");
	if (l_headlessArgPos != std::string::npos)
	{
		l_result.isHeadless = true;
		Log(Success, "Launch in headless mode, no windowing or rendering systems.");
	}

	auto l_offscreenArgPos = arg.find("offscreen");
	if (l_offscreenArgPos != std::string::npos)
	{
		l_result.isOffscreen = true;
		Log(Success, "Launch in offscreen mode, no windowing but real rendering server for testing.");
	}

	if (l_sidecarArgPos != std::string::npos)
	{
		l_result.engineMode = EngineMode::Sidecar;
		Log(Success, "Launch in sidecar mode, engine will be controlled by external process.");
	}

	if (arg.find("audit") != std::string::npos)
	{
		l_result.isAudit = true;
		Log(Success, "Audit mode: will dump all pass outputs after scene-load completes + settle frames.");
	}

	auto l_testArgPos = arg.find("-test");
	if (l_testArgPos != std::string::npos)
	{
		std::string l_remainder = arg.substr(l_testArgPos + 5);
		auto l_start = l_remainder.find_first_not_of(' ');
		if (l_start != std::string::npos)
		{
			auto l_end = l_remainder.find(' ', l_start);
			std::string l_caseName = l_remainder.substr(l_start,
				l_end == std::string::npos ? std::string::npos : l_end - l_start);
			strncpy(l_result.testCase, l_caseName.c_str(), sizeof(l_result.testCase) - 1);
			Log(Success, "Test case: ", l_result.testCase);
		}
		else
		{
			Log(Warning, "'-test' flag found but no test case name provided. Ignoring.");
		}
	}

	auto l_framesArgPos = arg.find("-total_frames");
	if (l_framesArgPos != std::string::npos)
	{
		std::string l_remainder = arg.substr(l_framesArgPos + 13);
		auto l_start = l_remainder.find_first_not_of(' ');
		if (l_start != std::string::npos)
		{
			l_result.totalFrames = std::stoi(l_remainder.substr(l_start));
			Log(Success, "Auto-terminate after ", l_result.totalFrames, " frames.");
		}
	}

	auto l_reloadArgPos = arg.find("-reload_at_frame");
	if (l_reloadArgPos != std::string::npos)
	{
		std::string l_remainder = arg.substr(l_reloadArgPos + 16);
		auto l_start = l_remainder.find_first_not_of(' ');
		if (l_start != std::string::npos)
		{
			l_result.reloadAtFrame = std::stoi(l_remainder.substr(l_start));
			Log(Success, "Scene reload at frame ", l_result.reloadAtFrame, ".");
		}
	}

	if (arg.find("-gpu_validation") != std::string::npos)
	{
		l_result.enableGPUValidation = true;
		Log(Success, "D3D12 GPU-based validation enabled.");
	}

	if (arg.find("-gpu_timer_log") != std::string::npos)
	{
		l_result.enableGpuTimerLog = true;
		Log(Success, "Per-pass GPU timer Verbose dump enabled.");
	}

	EngineParseInitConfigHelpers::ParseSerializeTestArg(arg, l_result);
	EngineParseInitConfigHelpers::ParseSceneArg(arg, l_result);
	EngineParseInitConfigHelpers::ParseDumpFramesArg(arg, l_result);
	EngineParseInitConfigHelpers::ParseCameraOrbitArg(arg, l_result);

	auto l_captureArgPos = arg.find("-capture_frame");
	if (l_captureArgPos != std::string::npos)
	{
		std::string l_remainder = arg.substr(l_captureArgPos + 14);
		auto l_start = l_remainder.find_first_not_of(' ');
		if (l_start != std::string::npos)
		{
			l_result.captureFrame = std::stoi(l_remainder.substr(l_start));
			Log(Success, "RenderDoc capture at frame ", l_result.captureFrame, ".");
		}
	}

	auto l_parentPidArgPos = arg.find("-parent_pid");
	if (l_parentPidArgPos != std::string::npos)
	{
		auto l_remainder = arg.substr(l_parentPidArgPos + 12);
		auto l_start = l_remainder.find_first_not_of(' ');
		if (l_start != std::string::npos)
		{
			l_result.parentPID = std::stoul(l_remainder.substr(l_start));
			Log(Success, "Parent PID set to: ", l_result.parentPID);
		}
	}

	EngineParseInitConfigHelpers::ParseBakeArg(arg, l_result);

	return l_result;
}
