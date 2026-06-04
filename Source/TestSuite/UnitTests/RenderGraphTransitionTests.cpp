#include "../Common/TestRunner.h"
#include "../../Engine/RenderGraph/RenderGraphSerializer.h"

using namespace Inno;

static RenderGraphDesc MakeTransitionGraph()
{
	// Mirrors PreTAAPass: a ScreenTile pass with a graphics-queue state-transition
	// prepass — two inputs WriteOnly->ReadOnly, own result ReadOnly->WriteOnly,
	// authored in the same order the prepass records them.
	RenderGraphDesc l_graph;
	l_graph.m_Name = "TransitionGraph";

	PassNodeDesc l_pass;
	l_pass.m_Name = "PreTAAPass";
	l_pass.m_Queue = GPUEngineType::Compute;
	l_pass.m_ShaderFilePaths.m_CSPath = "preTAAPass.comp";

	TransitionDesc l_t0;
	l_t0.m_Resource = "LightPass Luminance Result";
	l_t0.m_From = Accessibility::WriteOnly;
	l_t0.m_To = Accessibility::ReadOnly;
	l_pass.m_Transitions.push_back(l_t0);

	TransitionDesc l_t1;
	l_t1.m_Resource = "Sky Pass Result";
	l_t1.m_From = Accessibility::WriteOnly;
	l_t1.m_To = Accessibility::ReadOnly;
	l_pass.m_Transitions.push_back(l_t1);

	TransitionDesc l_t2;
	l_t2.m_Resource = "Pre-TAA Pass Result";
	l_t2.m_From = Accessibility::ReadOnly;
	l_t2.m_To = Accessibility::WriteOnly;
	l_pass.m_Transitions.push_back(l_t2);

	l_graph.m_Passes.push_back(l_pass);
	return l_graph;
}

void TestTransitionRoundTrip()
{
	TestRunner::StartTest("RenderGraph: transition prepass array round-trips (order + From/To); empty omits key");

	RenderGraphDesc l_original = MakeTransitionGraph();

	json j;
	RenderGraphSerializer::to_json(j, l_original);

	RenderGraphDesc l_loaded;
	RenderGraphSerializer::from_json(j, l_loaded);

	bool passed = l_loaded.m_Passes.size() == 1 && l_loaded.m_Passes[0].m_Transitions.size() == 3;

	if (passed)
	{
		const auto& t = l_loaded.m_Passes[0].m_Transitions;
		passed =
			t[0].m_Resource == "LightPass Luminance Result" &&
			t[0].m_From == Accessibility::WriteOnly && t[0].m_To == Accessibility::ReadOnly &&
			t[1].m_Resource == "Sky Pass Result" &&
			t[1].m_From == Accessibility::WriteOnly && t[1].m_To == Accessibility::ReadOnly &&
			t[2].m_Resource == "Pre-TAA Pass Result" &&
			t[2].m_From == Accessibility::ReadOnly && t[2].m_To == Accessibility::WriteOnly;
	}

	// Populated pass carries the key; a pass with no transitions omits it
	// entirely (sparse-key convention) and round-trips to an empty array.
	if (passed)
		passed = j["Passes"][0].contains("Transitions");

	if (passed)
	{
		RenderGraphDesc l_empty;
		PassNodeDesc l_emptyPass;
		l_emptyPass.m_Name = "NoTransitions";
		l_empty.m_Passes.push_back(l_emptyPass);

		json je;
		RenderGraphSerializer::to_json(je, l_empty);
		passed = !je["Passes"][0].contains("Transitions");

		if (passed)
		{
			RenderGraphDesc l_emptyLoaded;
			RenderGraphSerializer::from_json(je, l_emptyLoaded);
			passed = l_emptyLoaded.m_Passes.size() == 1 && l_emptyLoaded.m_Passes[0].m_Transitions.empty();
		}
	}

	TestRunner::EndTest(passed);
}
