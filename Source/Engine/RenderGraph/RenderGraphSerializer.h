#pragma once
#include "RenderGraphDesc.h"
#include "../ThirdParty/JSONWrapper/JSONWrapper.h"

namespace Inno
{
	// Hand-written JSON (de)serialization for the render-graph POD desc structs,
	// following the JSONSerializer_Components.cpp pattern. Enum fields are
	// written/read as readable strings via RenderGraphEnumStrings.
	namespace RenderGraphSerializer
	{
		void to_json(json& j, const RenderGraphDesc& p);
		void from_json(const json& j, RenderGraphDesc& p);

		// Load a graph file (path resolved through AssetService search) into a desc.
		bool LoadFromFile(const char* fileName, RenderGraphDesc& out);
	}
}
