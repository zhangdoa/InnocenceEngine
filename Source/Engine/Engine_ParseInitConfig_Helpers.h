#pragma once
#include "Engine.h"
#include <string>

namespace Inno
{
	// Each helper mutates out_Result for one -flag shape and is otherwise pure.
	// Callers pass the full command-line string; the flag prefix is part of the contract.
	namespace EngineParseInitConfigHelpers
	{
		void ParseSerializeTestArg(const std::string& in_Arg, InitConfig& out_Result);
		void ParseSceneArg(const std::string& in_Arg, InitConfig& out_Result);
		void ParseDumpFramesArg(const std::string& in_Arg, InitConfig& out_Result);
		void ParseCameraOrbitArg(const std::string& in_Arg, InitConfig& out_Result);
		void ParseBakeArg(const std::string& in_Arg, InitConfig& out_Result);
	}
}
