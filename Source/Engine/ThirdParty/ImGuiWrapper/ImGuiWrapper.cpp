#include "ImGuiWrapper.h"
#include "../../Common/Array.h"
#include "../ImGui/imgui.h"

#include "IImGuiWindow.h"
#include "IImGuiRenderer.h"

#if defined INNO_PLATFORM_WIN
#include "ImGuiWindowWin.h"
#endif

#if defined INNO_PLATFORM_MAC
#include "ImGuiWindowMac.h"
#endif

#if defined INNO_PLATFORM_LINUX
#include "ImGuiWindowLinux.h"
#endif

#if defined INNO_RENDERER_DIRECTX
#include "ImGuiRendererDX12.h"
#endif

#if defined INNO_RENDERER_VULKAN
#include "ImGuiRendererVK.h"
#endif

#if defined INNO_RENDERER_METAL
#include "ImGuiRendererMT.h"
#endif

#include "../../Common/RingBuffer.h"
#include "../../Common/IOService.h"
#include "../../Common/TaskScheduler.h"
#include "../../Services/PhysicsSimulationService.h"
#include "../../Services/SceneService.h"
#include "../../Services/RenderingConfigurationService.h"
#include "../../Services/ConfigurationService.h"
#include "../../RayTracer/RayTracer.h"

#include "../../Engine.h"

using namespace Inno;
;

namespace ImGuiWrapperNS
{
	void showApplicationProfiler();
	void zoom(bool zoom, ImTextureID textureID, ImVec2 renderTargetSize);
	void showConcurrencyProfiler();

	bool m_isParity = true;

	static RenderingConfig m_renderingConfig;
	static bool m_useZoom = false;
	static bool m_showRenderPassResult = false;
	static bool m_showConcurrencyProfiler = false;
	Inno::Array<RingBuffer<TaskReport, true>> m_taskReports;

	IImGuiWindow* m_windowImpl;
	IImGuiRenderer* m_rendererImpl;

	Inno::Array<std::function<void()>*> m_userDrawCallbacks;
}

using namespace ImGuiWrapperNS;

bool ImGuiWrapper::Setup()
{
	auto l_graphicsService = g_Engine->Get<ConfigurationService>()->GetGraphicsService();

#if defined INNO_PLATFORM_WIN
	m_windowImpl = new ImGuiWindowWin();
#endif
#if defined INNO_PLATFORM_MAC
	ImGuiWrapperNS::m_isParity = false;
#endif
#if defined INNO_PLATFORM_LINUX
	ImGuiWrapperNS::m_isParity = false;
#endif

	switch (l_graphicsService)
	{
	case GraphicsService::DX12:
#if defined INNO_RENDERER_DIRECTX
		m_rendererImpl = new ImGuiRendererDX12();
#endif
		break;
	case GraphicsService::VK:
#if defined INNO_RENDERER_VULKAN
		m_rendererImpl = new ImGuiRendererVK();
#endif
		break;
	case GraphicsService::MT:
#if defined INNO_RENDERER_METAL
		ImGuiWrapperNS::m_isParity = false;
#endif
		break;
	default:
		break;
	}

	if (ImGuiWrapperNS::m_isParity)
	{
		// Setup Dear ImGui binding
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();

		ImGuiWrapperNS::m_windowImpl->Setup();
		ImGuiWrapperNS::m_rendererImpl->Setup();
	}

	auto l_maxThreads = g_Engine->Get<TaskScheduler>()->GetThreadCounts();
	m_taskReports.resize(l_maxThreads);

	return true;
}

bool ImGuiWrapper::Initialize()
{
	if (ImGuiWrapperNS::m_isParity)
	{
		ImGuiWrapperNS::m_windowImpl->Initialize();
		ImGuiWrapperNS::m_rendererImpl->Initialize();

		ImGuiIO& io = ImGui::GetIO(); (void)io;

		// Setup style
		ImGui::StyleColorsDark();
		ImGuiStyle& style = ImGui::GetStyle();
		ImVec4* colors = ImGui::GetStyle().Colors;

		colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
		colors[ImGuiCol_TextDisabled] = ImVec4(0.36f, 0.36f, 0.36f, 1.00f);
		colors[ImGuiCol_WindowBg] = ImVec4(0.13f, 0.13f, 0.13f, 1.00f);
		colors[ImGuiCol_ChildBg] = ImVec4(1.00f, 1.00f, 1.00f, 0.00f);
		colors[ImGuiCol_PopupBg] = ImVec4(0.08f, 0.08f, 0.08f, 0.94f);
		colors[ImGuiCol_Border] = ImVec4(0.50f, 0.50f, 0.50f, 0.50f);
		colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
		colors[ImGuiCol_FrameBg] = ImVec4(0.36f, 0.36f, 0.36f, 0.50f);
		colors[ImGuiCol_FrameBgHovered] = ImVec4(0.50f, 0.50f, 0.50f, 0.71f);
		colors[ImGuiCol_FrameBgActive] = ImVec4(0.75f, 0.75f, 0.75f, 0.79f);
		colors[ImGuiCol_TitleBg] = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
		colors[ImGuiCol_TitleBgActive] = ImVec4(0.21f, 0.21f, 0.21f, 1.00f);
		colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 0.50f);
		colors[ImGuiCol_MenuBarBg] = ImVec4(0.17f, 0.17f, 0.17f, 1.00f);
		colors[ImGuiCol_ScrollbarBg] = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
		colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.36f, 0.36f, 0.36f, 1.00f);
		colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
		colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.75f, 0.75f, 0.75f, 1.00f);
		colors[ImGuiCol_CheckMark] = ImVec4(0.75f, 0.75f, 0.75f, 1.00f);
		colors[ImGuiCol_SliderGrab] = ImVec4(0.75f, 0.75f, 0.76f, 1.00f);
		colors[ImGuiCol_SliderGrabActive] = ImVec4(0.75f, 0.75f, 0.75f, 1.00f);
		colors[ImGuiCol_Button] = ImVec4(0.36f, 0.36f, 0.36f, 0.50f);
		colors[ImGuiCol_ButtonHovered] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
		colors[ImGuiCol_ButtonActive] = ImVec4(0.75f, 0.75f, 0.75f, 1.00f);
		colors[ImGuiCol_Header] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
		colors[ImGuiCol_HeaderHovered] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
		colors[ImGuiCol_HeaderActive] = ImVec4(0.75f, 0.75f, 0.75f, 1.00f);
		colors[ImGuiCol_Separator] = ImVec4(0.50f, 0.50f, 0.50f, 0.50f);
		colors[ImGuiCol_SeparatorHovered] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
		colors[ImGuiCol_SeparatorActive] = ImVec4(0.75f, 0.75f, 0.75f, 1.00f);
		colors[ImGuiCol_ResizeGrip] = ImVec4(0.36f, 0.36f, 0.36f, 0.25f);
		colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.50f, 0.50f, 0.50f, 0.50f);
		colors[ImGuiCol_ResizeGripActive] = ImVec4(0.75f, 0.75f, 0.75f, 1.00f);
		colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
		colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
		colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
		colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
		colors[ImGuiCol_TextSelectedBg] = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
		colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
		colors[ImGuiCol_NavHighlight] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
		colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
		colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
		colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);

		// Load Fonts
		auto l_fontPath = g_Engine->Get<IOService>()->GetEngineDirectory();
		l_fontPath += "Fonts/FreeSans.otf";
		auto font = io.Fonts->AddFontFromFileTTF(l_fontPath.c_str(), 16.0f);
		if (font == nullptr)
		{
			Log(Error, "Failed to load font.");
		}

		ImGuiWrapperNS::m_renderingConfig = g_Engine->Get<RenderingConfigurationService>()->GetRenderingConfig();
	}

	return true;
}

bool ImGuiWrapper::Prepare()
{
	if (ImGuiWrapperNS::m_isParity)
	{
		ImGuiWrapperNS::m_rendererImpl->NewFrame();
		ImGuiWrapperNS::m_windowImpl->NewFrame();

		ImGui::NewFrame();
		{
			ImGuiWrapperNS::showApplicationProfiler();
			ImGuiWrapperNS::showConcurrencyProfiler();

			for (auto* l_Callback : ImGuiWrapperNS::m_userDrawCallbacks)
			{
				if (l_Callback && *l_Callback)
					(*l_Callback)();
			}
		}
		ImGui::Render();

		ImGuiWrapperNS::m_rendererImpl->Prepare();
	}

	return true;
}

bool ImGuiWrapper::ExecuteCommands()
{
	if (ImGuiWrapperNS::m_isParity)
	{
		ImGuiWrapperNS::m_rendererImpl->ExecuteCommands();
	}

	return true;
}

bool ImGuiWrapper::Terminate()
{
	if (ImGuiWrapperNS::m_isParity)
	{
		ImGuiWrapperNS::m_windowImpl->Terminate();
		ImGuiWrapperNS::m_rendererImpl->Terminate();
		ImGui::DestroyContext();
	}
	ImGuiWrapperNS::m_userDrawCallbacks.clear();
	return true;
}

void ImGuiWrapper::AddUserDrawCallback(std::function<void()>* in_Callback)
{
	if (!in_Callback)
	{
		Log(Warning, "ImGuiWrapper::AddUserDrawCallback rejected: null callback pointer.");
		return;
	}
	ImGuiWrapperNS::m_userDrawCallbacks.push_back(in_Callback);
}

void ImGuiWrapperNS::showApplicationProfiler()
{
	ImGui::Begin("Profiler", 0, ImGuiWindowFlags_AlwaysAutoResize);
	ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
	ImGui::Text("Culling result: %zu", g_Engine->Get<PhysicsSimulationService>()->GetCullingResult().size());
	ImGui::Checkbox("Show concurrency profiler", &m_showConcurrencyProfiler);
	ImGui::Checkbox("Use Motion Blur", &m_renderingConfig.useMotionBlur);
	ImGui::Checkbox("Use TAA", &m_renderingConfig.useTAA);
	ImGui::Checkbox("Use Bloom", &m_renderingConfig.useBloom);
	ImGui::Checkbox("Draw terrain", &m_renderingConfig.drawTerrain);
	ImGui::Checkbox("Draw sky", &m_renderingConfig.drawSky);
	ImGui::Checkbox("Draw debug object", &m_renderingConfig.drawDebugObject);
	ImGui::Checkbox("CSM fit to scene", &m_renderingConfig.CSMFitToScene);
	ImGui::Checkbox("CSM adjust draw distance", &m_renderingConfig.CSMAdjustDrawDistance);
	ImGui::Checkbox("CSM adjust side plane", &m_renderingConfig.CSMAdjustSidePlane);

	ImGui::Checkbox("Use zoom", &m_useZoom);

	const char* items[] = { "Shadow", "GI", "Opaque", "Light", "Transparent", "Terrain", "Post-processing", "Development" };

	static int32_t l_showRenderPassResultItem = 0;

	ImGui::Combo("Choose render pass", &l_showRenderPassResultItem, items, IM_ARRAYSIZE(items));

	ImGui::Checkbox("Show render pass result", &m_showRenderPassResult);

	if (m_showRenderPassResult)
	{
		ImGuiWrapperNS::m_rendererImpl->ShowRenderResult(RenderPassType(l_showRenderPassResultItem));
	}

	if (ImGui::Button("Run ray trace"))
	{
		g_Engine->Get<RayTracer>()->Execute();
	}

	static char scene_filePath[128];
	ImGui::InputText("Scene file path", scene_filePath, IM_ARRAYSIZE(scene_filePath));

	if (ImGui::Button("Save scene"))
	{
		g_Engine->Get<SceneService>()->Save(scene_filePath);
	}
	if (ImGui::Button("Load scene"))
	{
		// AsyncLoad=true: ImGui callback fires mid-frame after pass commands
		// have been recorded; LoadSync here races the DX12 resource-state
		// tracker. (.claude/state/engine-invariants.md)
		g_Engine->Get<SceneService>()->Load(scene_filePath, true);
	}

	ImGui::End();

	g_Engine->Get<RenderingConfigurationService>()->SetRenderingConfig(m_renderingConfig);
}

void ImGuiWrapperNS::zoom(bool zoom, ImTextureID textureID, ImVec2 renderTargetSize)
{
	if (zoom)
	{
		ImGuiIO& io = ImGui::GetIO(); (void)io;
		ImVec2 pos = ImGui::GetCursorScreenPos();
		if (ImGui::IsItemHovered())
		{
			ImGui::BeginTooltip();
			float region_sz = 32.0f;
			float region_x = io.MousePos.x - pos.x - region_sz * 0.5f; if (region_x < 0.0f) region_x = 0.0f; else if (region_x > renderTargetSize.x - region_sz) region_x = renderTargetSize.x - region_sz;
			float region_y = pos.y - io.MousePos.y - region_sz * 0.5f; if (region_y < 0.0f) region_y = 0.0f; else if (region_y > renderTargetSize.y - region_sz) region_y = renderTargetSize.y - region_sz;
			float zoom = 4.0f;
			ImGui::Text("Min: (%.2f, %.2f)", region_x, region_y);
			ImGui::Text("Max: (%.2f, %.2f)", region_x + region_sz, region_y + region_sz);
			ImVec2 uv0 = ImVec2((region_x) / renderTargetSize.x, (region_y + region_sz) / renderTargetSize.y);
			ImVec2 uv1 = ImVec2((region_x + region_sz) / renderTargetSize.x, (region_y) / renderTargetSize.y);
			ImGui::Image(textureID, ImVec2(region_sz * zoom, region_sz * zoom), uv0, uv1, ImColor(255, 255, 255, 255), ImColor(255, 255, 255, 128));
			ImGui::EndTooltip();
		}
	}
}

ImVec4 generateButtonColor(const char* name)
{
	auto l_ptr = reinterpret_cast<intptr_t>(name);
	auto l_hue = l_ptr * 4 % 180;
	auto l_RGB = Math::HSVtoRGB(Vec4((float)l_hue, 1.0f, 1.0f, 1.0f));
	return ImVec4(l_RGB.x, l_RGB.y, l_RGB.z, 1.0f);
}

void ImGuiWrapperNS::showConcurrencyProfiler()
{
	if (!m_showConcurrencyProfiler)
		return;

	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 1));
	ImGui::Begin("ConcurrencyProfiler", 0);

	static float l_scopeInMs = 1000.0f;
	ImGui::DragFloat("Scope (Ms)", &l_scopeInMs, 0.1f, 0.0f, 10000.0f);

	auto l_maxThreads = g_Engine->Get<TaskScheduler>()->GetThreadCounts();
	for (uint32_t i = 0; i < l_maxThreads; i++)
	{
		auto l_taskReport = g_Engine->Get<TaskScheduler>()->GetTaskReport(i);

		uint32_t l_taskReportCount = l_taskReport.size();

		ImGui::Text("Thread %d", i);
		ImGui::Separator();

		ImGui::BeginGroup();
		auto l_windowWidth = ImGui::GetWindowContentRegionWidth();
		constexpr auto l_padding = 1.0f;
		auto l_minimumButtonWidth = 0.005f * l_windowWidth;
		auto l_maximumButtonWidth = l_windowWidth;
		auto l_buttonHeight = 20.0f;
		auto l_lastButtonPos = l_taskReport[0].m_StartTime;
		auto l_mostRightPos = l_scopeInMs * 1000.0f + l_lastButtonPos;

		for (uint32_t j = 0; j < l_taskReportCount; j++)
		{
			auto l_startTime = l_taskReport[j].m_StartTime;
			auto l_finishTime = l_taskReport[j].m_FinishTime;

			if (l_finishTime > l_mostRightPos)
				continue;

			auto l_InvalidButtonWidth = (float)(l_startTime - l_lastButtonPos) / l_scopeInMs;
			if (l_InvalidButtonWidth > 0)
			{
				l_InvalidButtonWidth += l_padding;
				l_InvalidButtonWidth = Math::clamp(l_InvalidButtonWidth, l_minimumButtonWidth, l_maximumButtonWidth);
				ImGui::Button("", ImVec2(l_InvalidButtonWidth, l_buttonHeight));
				ImGui::SameLine();
			}

			auto l_visibleButtonWidth = (float)(l_finishTime - l_startTime) / l_scopeInMs;
			l_visibleButtonWidth += l_padding;
			l_visibleButtonWidth = Math::clamp(l_visibleButtonWidth, l_minimumButtonWidth, l_maximumButtonWidth);

			ImGui::PushStyleColor(ImGuiCol_Button, generateButtonColor(l_taskReport[j].m_TaskName));
			ImGui::Button("", ImVec2(l_visibleButtonWidth, l_buttonHeight));
			if (ImGui::IsItemHovered())
			{
				ImGui::SetTooltip("%s\nStart %.6f ms\nEnd %.6f ms\nDuration %.6f ms",
					l_taskReport[j].m_TaskName,
					(double)l_startTime / 1000.0,
					(double)l_finishTime / 1000.0,
					((double)l_finishTime - (double)l_startTime) / 1000.0
				);
			}
			ImGui::PopStyleColor(1);
			ImGui::SameLine();

			l_lastButtonPos = l_finishTime;
		}

		ImGui::Separator();
		ImGui::EndGroup();

		ImGui::Separator();
	}
	ImGui::PopStyleVar();
	ImGui::End();
}