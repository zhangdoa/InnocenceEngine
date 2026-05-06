#include "Engine_ParseInitConfig_Helpers.h"
#include "Common/LogService.h"
#include <cstring>

namespace Inno
{
	namespace EngineParseInitConfigHelpers
	{
		void ParseSerializeTestArg(const std::string& in_Arg, InitConfig& out_Result)
		{
			auto l_Pos = in_Arg.find("-serialize_test");
			if (l_Pos == std::string::npos)
				return;

			std::string l_Remainder = in_Arg.substr(l_Pos + 15);
			auto l_Start = l_Remainder.find_first_not_of(' ');
			if (l_Start == std::string::npos)
				return;

			auto l_End = l_Remainder.find(' ', l_Start);
			std::string l_Path = l_Remainder.substr(l_Start,
				l_End == std::string::npos ? std::string::npos : l_End - l_Start);
			if (l_Path.size() < sizeof(out_Result.serializeTest))
			{
				std::memcpy(out_Result.serializeTest, l_Path.c_str(), l_Path.size() + 1);
				Log(Success, "Serialize-determinism test on scene: ", out_Result.serializeTest);
				out_Result.isOffscreen = true; // render pipeline not required
				out_Result.totalFrames = 1;    // exit immediately after save
			}
		}

		// Scene override for the auto-test path. `-scene <relative-path>` makes
		// the example logic client load this scene as the initial scene and
		// suppresses the default "switch to GISponza at frame 5" transition,
		// so each of UnitTest / GITestBox / GISponza can be driven headlessly
		// for the three-scene visual gate. Whitespace-terminated single token,
		// matching the other flag-arg conventions in this parser.
		void ParseSceneArg(const std::string& in_Arg, InitConfig& out_Result)
		{
			auto l_Pos = in_Arg.find("-scene ");
			if (l_Pos == std::string::npos)
				return;

			std::string l_Remainder = in_Arg.substr(l_Pos + 7);
			auto l_Start = l_Remainder.find_first_not_of(' ');
			if (l_Start == std::string::npos)
			{
				Log(Warning, "'-scene' flag found but no scene path provided. Ignoring.");
				return;
			}

			auto l_End = l_Remainder.find(' ', l_Start);
			std::string l_Path = l_Remainder.substr(l_Start,
				l_End == std::string::npos ? std::string::npos : l_End - l_Start);
			if (l_Path.size() < sizeof(out_Result.initialScene))
			{
				std::memcpy(out_Result.initialScene, l_Path.c_str(), l_Path.size() + 1);
				Log(Success, "Initial scene override: ", out_Result.initialScene);
			}
			else
			{
				Log(Warning, "'-scene' path too long (max ", sizeof(out_Result.initialScene) - 1, " chars); ignoring.");
			}
		}

		void ParseDumpFramesArg(const std::string& in_Arg, InitConfig& out_Result)
		{
			auto l_Pos = in_Arg.find("-dump_frames");
			if (l_Pos == std::string::npos)
				return;

			// Expect `-dump_frames START-END` (inclusive). Single token, no spaces
			// inside the range — matches the other flag-arg conventions in this
			// parser.
			std::string l_Remainder = in_Arg.substr(l_Pos + 12);
			auto l_Start = l_Remainder.find_first_not_of(' ');
			if (l_Start == std::string::npos)
				return;

			std::string l_Range = l_Remainder.substr(l_Start);
			auto l_Space = l_Range.find(' ');
			if (l_Space != std::string::npos) l_Range = l_Range.substr(0, l_Space);
			auto l_Dash = l_Range.find('-');
			if (l_Dash == std::string::npos || l_Dash == 0 || l_Dash + 1 >= l_Range.size())
			{
				Log(Warning, "'-dump_frames' expects START-END with a dash.");
				return;
			}

			try
			{
				out_Result.dumpFramesStart = std::stoi(l_Range.substr(0, l_Dash));
				out_Result.dumpFramesEnd   = std::stoi(l_Range.substr(l_Dash + 1));
				if (out_Result.dumpFramesEnd < out_Result.dumpFramesStart)
				{
					Log(Warning, "'-dump_frames' end < start; ignoring.");
					out_Result.dumpFramesStart = -1;
					out_Result.dumpFramesEnd   = -1;
				}
				else
				{
					Log(Success, "Dumping gpu_output_NNNN.png for frames [",
						out_Result.dumpFramesStart, ", ", out_Result.dumpFramesEnd, "].");
				}
			}
			catch (...)
			{
				Log(Warning, "'-dump_frames' range parse failed; expected START-END.");
			}
		}

		void ParseCameraOrbitArg(const std::string& in_Arg, InitConfig& out_Result)
		{
			auto l_Pos = in_Arg.find("-camera_orbit");
			if (l_Pos == std::string::npos)
				return;

			// Expect `-camera_orbit PITCH_DEG,RADIUS,DURATION_FRAMES`.
			// Comma-separated triple, one whitespace-terminated token.
			std::string l_Remainder = in_Arg.substr(l_Pos + 13);
			auto l_Start = l_Remainder.find_first_not_of(' ');
			if (l_Start == std::string::npos)
				return;

			std::string l_Triple = l_Remainder.substr(l_Start);
			auto l_Space = l_Triple.find(' ');
			if (l_Space != std::string::npos) l_Triple = l_Triple.substr(0, l_Space);
			auto l_C1 = l_Triple.find(',');
			auto l_C2 = (l_C1 != std::string::npos) ? l_Triple.find(',', l_C1 + 1) : std::string::npos;
			if (l_C1 == std::string::npos || l_C2 == std::string::npos)
			{
				Log(Warning, "'-camera_orbit' expects PITCH_DEG,RADIUS,DURATION (three comma-separated values).");
				return;
			}

			try
			{
				out_Result.cameraOrbitPitchDeg = std::stof(l_Triple.substr(0, l_C1));
				out_Result.cameraOrbitRadius   = std::stof(l_Triple.substr(l_C1 + 1, l_C2 - l_C1 - 1));
				out_Result.cameraOrbitDuration = std::stoi(l_Triple.substr(l_C2 + 1));
				if (out_Result.cameraOrbitDuration > 0 && out_Result.cameraOrbitRadius > 0.0f)
				{
					out_Result.cameraOrbitActive = true;
					Log(Success, "Camera orbit: pitch=", out_Result.cameraOrbitPitchDeg,
						"deg radius=", out_Result.cameraOrbitRadius,
						" duration=", out_Result.cameraOrbitDuration, " frames.");
				}
				else
				{
					Log(Warning, "'-camera_orbit' requires DURATION > 0 and RADIUS > 0; ignoring.");
				}
			}
			catch (...)
			{
				Log(Warning, "'-camera_orbit' parse failed; expected PITCH_DEG,RADIUS,DURATION.");
			}
		}

		// Bake mode (TASK-68): `-bake "path1.gltf;path2.fbx;..."` runs a one-shot
		// asset-import-then-exit pass with no window or rendering services. The
		// quoted argument is a `;`-separated list of paths relative to the
		// working directory. Implies -headless.
		void ParseBakeArg(const std::string& in_Arg, InitConfig& out_Result)
		{
			auto l_Pos = in_Arg.find("-bake");
			if (l_Pos == std::string::npos)
				return;

			auto l_Remainder = in_Arg.substr(l_Pos + 5);
			// Accept either `-bake "a;b"` (quoted) or `-bake a;b` (unquoted, ends at next arg).
			auto l_Start = l_Remainder.find_first_not_of(" \t");
			if (l_Start == std::string::npos)
				return;

			size_t l_End = std::string::npos;
			if (l_Remainder[l_Start] == '"')
			{
				++l_Start;
				l_End = l_Remainder.find('"', l_Start);
			}
			else
			{
				l_End = l_Remainder.find_first_of(" \t", l_Start);
			}
			const std::string l_List = l_Remainder.substr(
				l_Start, l_End == std::string::npos ? std::string::npos : l_End - l_Start);
			strncpy(out_Result.bakeInputs, l_List.c_str(), sizeof(out_Result.bakeInputs) - 1);
			out_Result.isBakeMode = true;
			out_Result.isHeadless = true;
			Log(Success, "Bake mode: will import '", out_Result.bakeInputs, "' then exit.");
		}
	}
}
