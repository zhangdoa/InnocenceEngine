#pragma once
#include "../Common/GraphicsPrimitive.h"
#include "../Component/ShaderProgramComponent.h"

namespace Inno
{
	// POD description structs deserialized from a render-graph JSON file.
	// These mirror the imperative ::Setup values a pass would otherwise hard-code;
	// the RenderGraphService builds engine components (TextureComponent /
	// RenderPassComponent / CommandListComponent) from them at startup.

	enum class RenderGraphResourceType { Texture, Buffer };
	enum class RenderGraphResourceLifetime { Persistent };

	struct ResourceDesc
	{
		std::string m_Name;
		RenderGraphResourceType m_Type = RenderGraphResourceType::Texture;
		RenderGraphResourceLifetime m_Lifetime = RenderGraphResourceLifetime::Persistent;
		TextureDesc m_TextureDesc = {};
	};

	struct BindingDesc
	{
		std::string m_Resource;
		GPUResourceType m_GPUResourceType = GPUResourceType::Image;
		uint32_t m_DescriptorSetIndex = 0;
		uint32_t m_DescriptorIndex = 0;
		Accessibility m_BindingAccessibility = Accessibility::ReadOnly;
		Accessibility m_ResourceAccessibility = Accessibility::ReadOnly;
		TextureUsage m_TextureUsage = TextureUsage::Invalid;
		ShaderStage m_ShaderStage = ShaderStage::Invalid;
	};

	struct DispatchDesc
	{
		uint32_t m_X = 1;
		uint32_t m_Y = 1;
		uint32_t m_Z = 1;
	};

	struct PassNodeDesc
	{
		std::string m_Name;
		std::string m_Kernel = "Default";
		GPUEngineType m_Queue = GPUEngineType::Compute;
		ShaderFilePaths m_ShaderFilePaths = {};
		Inno::Array<std::string> m_Reads;
		Inno::Array<std::string> m_Writes;
		Inno::Array<BindingDesc> m_Bindings;
		DispatchDesc m_Dispatch = {};
		bool m_OneShot = false;
		bool m_BypassEnabled = false;
		bool m_ClearOnBypass = false;
	};

	struct RenderGraphDesc
	{
		std::string m_Name;
		Inno::Array<ResourceDesc> m_Resources;
		Inno::Array<PassNodeDesc> m_Passes;
	};
}
