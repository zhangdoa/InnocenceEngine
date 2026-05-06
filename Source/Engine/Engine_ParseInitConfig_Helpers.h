#pragma once
#include "Engine.h"
#include <string>

namespace Inno
{
	// Per-flag parsers consumed only by `Engine::ParseInitConfig`. Split out of
	// `Engine_ParseInitConfig.cpp` to keep the dispatcher TU under the file-size
	// ratchet. Each helper mutates `out_Result` for one `-flag` argument shape
	// and is otherwise pure. The flag-search prefix is part of each helper's
	// contract — callers pass the full command-line string.
	namespace EngineParseInitConfigHelpers
	{
		void ParseSerializeTestArg(const std::string& in_Arg, InitConfig& out_Result);
		void ParseSceneArg(const std::string& in_Arg, InitConfig& out_Result);
		void ParseDumpFramesArg(const std::string& in_Arg, InitConfig& out_Result);
		void ParseCameraOrbitArg(const std::string& in_Arg, InitConfig& out_Result);
		void ParseBakeArg(const std::string& in_Arg, InitConfig& out_Result);
	}
}
