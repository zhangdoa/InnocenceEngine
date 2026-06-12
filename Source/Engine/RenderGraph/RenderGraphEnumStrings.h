#pragma once
#include <string>
#include "../Common/GraphicsPrimitive.h"
#include "RenderGraphDesc.h"

namespace Inno
{
	// String <-> enum for render-graph JSON: GraphicsPrimitive.h enums lack
	// INNO_ENUM ToString, so the readable-string schema needs these tables.
	// FromString falls back to the type's Invalid/default on an unknown token.
	namespace RenderGraphEnumStrings
	{
		std::string ToString(GPUEngineType v);
		GPUEngineType GPUEngineTypeFromString(const std::string& s);

		std::string ToString(TextureSampler v);
		TextureSampler TextureSamplerFromString(const std::string& s);

		std::string ToString(TextureUsage v);
		TextureUsage TextureUsageFromString(const std::string& s);

		std::string ToString(TexturePixelDataFormat v);
		TexturePixelDataFormat TexturePixelDataFormatFromString(const std::string& s);

		std::string ToString(TexturePixelDataType v);
		TexturePixelDataType TexturePixelDataTypeFromString(const std::string& s);

		std::string ToString(GPUResourceType v);
		GPUResourceType GPUResourceTypeFromString(const std::string& s);

		std::string ToString(ShaderStage v);
		ShaderStage ShaderStageFromString(const std::string& s);

		std::string ToString(Accessibility v);
		Accessibility AccessibilityFromString(const std::string& s);

		std::string ToString(GPUBufferUsage v);
		GPUBufferUsage GPUBufferUsageFromString(const std::string& s);

		std::string ToString(RenderGraphResourceType v);
		RenderGraphResourceType ResourceTypeFromString(const std::string& s);

		std::string ToString(RenderGraphResourceLifetime v);
		RenderGraphResourceLifetime LifetimeFromString(const std::string& s);

		std::string ToString(DispatchMode v);
		DispatchMode DispatchModeFromString(const std::string& s);

		std::string ToString(ComparisionFunction v);
		ComparisionFunction ComparisionFunctionFromString(const std::string& s);
	}
}
