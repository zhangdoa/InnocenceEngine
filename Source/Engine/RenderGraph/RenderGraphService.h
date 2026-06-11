#pragma once
#include <memory>
#include <functional>
#include <unordered_map>
#include "RenderGraphDesc.h"
#include "RenderGraphPassRecorder.h"
#include "../Component/TextureComponent.h"
#include "../Component/ShaderProgramComponent.h"

namespace Inno
{
	// A compiled pass node: the engine components the graph built from a PassNodeDesc.
	struct RenderGraphPassNode
	{
		PassNodeDesc m_Desc;
		RenderPassComponent* m_RenderPass = nullptr;
		ShaderProgramComponent* m_ShaderProgram = nullptr;
		CommandListComponent* m_CommandList_Compute = nullptr;
		CommandListComponent* m_CommandList_Graphics = nullptr;

		// First Writes resource — the node's primary output (parity with GetResult()).
		GPUResourceComponent* m_PrimaryOutput = nullptr;
	};

	// Loads a render-graph JSON at startup, creates resources + pass nodes via the
	// existing resource services, and records each node through its kernel.
	// ScheduledNodes() is where a topo-sort over reads/writes will slot in.
	class RenderGraphService
	{
	public:
		RenderGraphService();
		~RenderGraphService();

		bool LoadGraph(const char* fileName);

		RenderGraphPassNode* FindNode(const char* name);
		const Inno::Array<RenderGraphPassNode*>& ScheduledNodes() const { return m_Schedule; }

	// Record a node's command list from its data (per-frame / one-shot).
	bool RecordNode(RenderGraphPassNode* node);

	// Record every scheduled node, then submit + fence them (graph owns the
	// queue/sync topology, derived from each node's queue, transition prepass,
	// reads->producer edges, and one-shot flag). Replaces per-pass client submission.
	bool Render();

		// Live resolved resource by name — for a migrated pass adopting a
		// graph-owned resource (e.g. a deferred screen-sized RT created after
		// Initialize runs the writer node's RT-init-func).
		GPUResourceComponent* GetResource(const std::string& name) { return FindResource(name); }

		// The OTHER-parity (history) texture of a ping-pong resource — the previous
		// frame's output a node reads back. A plain GetResource of the same name
		// hands back the current-parity texture (this frame's output).
		GPUResourceComponent* GetHistoryResource(const std::string& name) { return PingPongTexture(name, true); }

		// Named hooks for the residual CPU work a pure-data node can't express:
		// an init hook creates+fills a node's imported resources (run once after
		// load); an update hook refreshes per-frame data before the node records.
		// Keyed by node name; the client registers them in Setup.
		void RegisterInitHook(const std::string& nodeName, std::function<void()> fn) { m_InitHooks[nodeName] = std::move(fn); }
		void RegisterUpdateHook(const std::string& nodeName, std::function<void()> fn) { m_UpdateHooks[nodeName] = std::move(fn); }

	private:
		GPUResourceComponent* FindResource(const std::string& name);
		GPUResourceComponent* ResolveImportedResource(const std::string& name);
		bool CreateResource(const ResourceDesc& desc);
		bool CreatePassNode(const PassNodeDesc& desc);
		// (Re)creates a screen-sized texture at the current resolution. Installed as
		// the writer node's RenderPass init-func so the engine's PostResize loop
		// drives resize through the same path the imperative passes used.
		bool CreateScreenSizedTexture(const ResourceDesc& desc);
		// Initializes graph-owned resources that no pass writes (their producer
		// isn't a graph node yet) so consumers bind valid zeroed inputs.
		void CreateOrphanResources();
		// Resolves a ping-pong pair to one physical texture by frame parity:
		// history=false -> current-frame output (odd frame -> Odd), history=true ->
		// the other parity (previous frame's output). Null if name isn't ping-pong.
		TextureComponent* PingPongTexture(const std::string& name, bool history);

		RenderGraphDesc m_Desc;
		std::unordered_map<std::string, GPUResourceComponent*> m_Resources;
		// Texture resources whose size is "screen" — created lazily by the writer
		// node's RT-init-func, not eagerly in CreateResource.
		std::unordered_map<std::string, ResourceDesc> m_DeferredScreenTextures;
		// Ping-pong pairs keyed by logical name: { Even, Odd } physical textures.
		// The writer node's RT-init-func (re)creates both via CreateScreenSizedTexture.
		std::unordered_map<std::string, std::pair<TextureComponent*, TextureComponent*>> m_PingPong;
		std::unordered_map<std::string, std::unique_ptr<RenderGraphPassNode>> m_Nodes;
		Inno::Array<RenderGraphPassNode*> m_Schedule;
		bool m_Loaded = false;
		bool m_OneShotDone = false;
		std::unordered_map<std::string, std::function<void()>> m_InitHooks;
		std::unordered_map<std::string, std::function<void()>> m_UpdateHooks;
	};
}
