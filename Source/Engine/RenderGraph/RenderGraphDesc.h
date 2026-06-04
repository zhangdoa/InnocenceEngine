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

	// Fields a migrated buffer-owning pass sets on its GPUBufferComponent in
	// Setup/Initialize (e.g. LuminanceAveragePass, ComputeCullingPass). Mirrors
	// the imperative path's m_ElementCount/m_ElementSize/m_Usage/accessibility;
	// no speculative fields are serialized.
	struct BufferDesc
	{
		size_t m_ElementCount = 0;
		size_t m_ElementSize = 0;
		GPUBufferUsage m_Usage = GPUBufferUsage::Generic;
		Accessibility m_CPUAccessibility = Accessibility(false, false);
		Accessibility m_GPUAccessibility = Accessibility(true, true);
	};

	struct ResourceDesc
	{
		std::string m_Name;
		RenderGraphResourceType m_Type = RenderGraphResourceType::Texture;
		RenderGraphResourceLifetime m_Lifetime = RenderGraphResourceLifetime::Persistent;
		// Screen-relative size as data: "screen" -> current screen resolution at
		// creation, recreated on resize via the owning pass's RT init-func. Empty =
		// fixed Width/Height. Only the literal "screen" is recognized today; finer
		// expressions ("screen/2") slot in here when a pass needs one.
		std::string m_SizeExpr;
		// A resource produced by a still-imperative pass: not created/owned by the
		// graph; resolved at bind time to the live engine resource by name via the
		// matching *ResourceService. Reads naming a resource absent from the graph's
		// Resources list are also imported.
		bool m_Imported = false;
		TextureDesc m_TextureDesc = {};
		BufferDesc m_BufferDesc = {};
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

	// An ordered render-target state transition recorded on the graphics queue
	// before the pass body — the compute queue cannot transition a render target
	// between ReadOnly and WriteOnly. Applied in array order.
	struct TransitionDesc
	{
		std::string m_Resource;
		Accessibility m_From = Accessibility::WriteOnly;
		Accessibility m_To = Accessibility::ReadOnly;
	};

	// How the dispatch thread-group count is derived. Static uses the literal
	// X/Y/Z; ScreenTile floors viewport/m_TileSize; TiledTwoLevel applies the
	// light-culling floor-then-ceil reduction; DrawModelGroups packs the live
	// draw-model count into groups of m_TileSize. A dispatch variant is data,
	// not a code path.
	enum class DispatchMode { Static, ScreenTile, TiledTwoLevel, DrawModelGroups };

	struct DispatchDesc
	{
		uint32_t m_X = 1;
		uint32_t m_Y = 1;
		uint32_t m_Z = 1;
		DispatchMode m_Mode = DispatchMode::Static;
		uint32_t m_TileSize = 0;
	};

	struct PassNodeDesc
	{
		std::string m_Name;
		GPUEngineType m_Queue = GPUEngineType::Compute;
		ShaderFilePaths m_ShaderFilePaths = {};
		Inno::Array<std::string> m_Reads;
		Inno::Array<std::string> m_Writes;
		Inno::Array<BindingDesc> m_Bindings;
		Inno::Array<TransitionDesc> m_Transitions;
		DispatchDesc m_Dispatch = {};
		bool m_OneShot = false;
		bool m_BypassEnabled = false;
		bool m_ClearOnBypass = false;
		// Publish the first Writes resource's post-write state after recording so a
		// downstream consumer emits the correct barrier (e.g. culling -> ExecuteIndirect).
		bool m_TrackWriteState = false;
	};

	struct RenderGraphDesc
	{
		std::string m_Name;
		Inno::Array<ResourceDesc> m_Resources;
		Inno::Array<PassNodeDesc> m_Passes;
	};
}
