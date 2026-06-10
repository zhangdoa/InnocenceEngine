#include "RenderGraphService.h"

#include "../Engine.h"
#include "../Services/GraphicsHardwareService.h"

using namespace Inno;

namespace
{
	// The scheduled node (before `consumer`) that writes `name` — the graph
	// producer of a resource the consumer reads. nullptr for imported/orphan
	// inputs (no producing node), which need no cross-node wait.
	RenderGraphPassNode* FindProducer(const Inno::Array<RenderGraphPassNode*>& recorded,
		const std::string& name, const RenderGraphPassNode* consumer)
	{
		RenderGraphPassNode* l_producer = nullptr;
		for (auto* l_node : recorded)
		{
			if (l_node == consumer)
				break;
			for (const auto& l_write : l_node->m_Desc.m_Writes)
				if (l_write == name)
					l_producer = l_node;
		}
		return l_producer;
	}
}

bool RenderGraphService::Render()
{
	// First, record every ready node (one-shot nodes only on the first frame).
	Inno::Array<RenderGraphPassNode*> l_recorded;
	for (auto* l_node : m_Schedule)
	{
		if (l_node->m_Desc.m_OneShot && m_OneShotDone)
			continue;
		if (!l_node->m_RenderPass || l_node->m_RenderPass->m_ObjectStatus != ObjectStatus::Activated)
			continue;
		if (RecordNode(l_node))
			l_recorded.push_back(l_node);
	}

	// Then submit + fence. The sync topology is derived from node data, not
	// hand-authored per pass.
	auto l_hw = g_Engine->Get<GraphicsHardwareService>();
	bool l_oneShotRan = false;
	for (auto* l_node : l_recorded)
	{
		const auto& l_desc = l_node->m_Desc;
		const auto l_queue = l_desc.m_Queue;
		const bool l_hasPrepass = !l_desc.m_Transitions.empty();
		const auto l_firstQueue = l_hasPrepass ? GPUEngineType::Graphics : l_queue;

		// Wait on the producer of each read so the consumer's first queue sees the
		// producer's writes (cross-queue/cross-pass dependency).
		for (const auto& l_read : l_desc.m_Reads)
		{
			auto* l_producer = FindProducer(l_recorded, l_read, l_node);
			if (l_producer)
				l_hw->WaitOnGPU(l_producer->m_RenderPass, l_firstQueue, l_producer->m_Desc.m_Queue);
		}

		// Graphics-queue transition prepass (compute can't transition a render
		// target): execute it, then make the node's queue wait on it.
		if (l_hasPrepass)
		{
			l_hw->Execute(l_node->m_CommandList_Graphics, GPUEngineType::Graphics);
			l_hw->SignalOnGPU(l_node->m_RenderPass, GPUEngineType::Graphics);
			l_hw->WaitOnGPU(l_node->m_RenderPass, l_queue, GPUEngineType::Graphics);
		}

		l_hw->Execute(l_node->m_CommandList_Compute, l_queue);
		l_hw->SignalOnGPU(l_node->m_RenderPass, l_queue);

		if (l_desc.m_OneShot)
			l_oneShotRan = true;
	}

	// One-shot bakes (e.g. BRDF LUT) run once; block the CPU until they finish so
	// their results are ready before first use, then never resubmit them.
	if (l_oneShotRan && !m_OneShotDone)
	{
		l_hw->WaitOnCPU(l_hw->GetSemaphoreValue(GPUEngineType::Graphics), GPUEngineType::Graphics);
		l_hw->WaitOnCPU(l_hw->GetSemaphoreValue(GPUEngineType::Compute), GPUEngineType::Compute);
		m_OneShotDone = true;
	}

	return true;
}
