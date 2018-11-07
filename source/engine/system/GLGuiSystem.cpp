#include "GLGuiSystem.h"
#include "../component/WindowSystemSingletonComponent.h"
#include "../component/GLWindowSystemSingletonComponent.h"
#include "../component/ShadowRenderPassSingletonComponent.h"
#include "../component/GeometryRenderPassSingletonComponent.h"
#include "../component/LightRenderPassSingletonComponent.h"
#include "../component/GLFinalRenderPassSingletonComponent.h"
#include "../component/RenderingSystemSingletonComponent.h"

#include "../third-party/ImGui/imgui.h"
#include "../third-party/ImGui/imgui_impl_glfw_gl3.h"

#include "ICoreSystem.h"

extern ICoreSystem* g_pCoreSystem;

class ImGuiWrapper
{
public:
	~ImGuiWrapper() {};

	static ImGuiWrapper& getInstance()
	{
		static ImGuiWrapper instance;
		return instance;
	}
	void setup();
	void initialize();
	void update();
	void terminate();
	void zoom(bool zoom, ImTextureID textureID, ImVec2 renderTargetSize);

private:
	ImGuiWrapper() {};
};

INNO_PRIVATE_SCOPE GLGuiSystemNS
{
	objectStatus m_objectStatus = objectStatus::SHUTDOWN;
}

INNO_SYSTEM_EXPORT bool GLGuiSystem::setup()
{
	ImGuiWrapper::getInstance().setup();
	GLGuiSystemNS::m_objectStatus = objectStatus::ALIVE;
	return true;
}

INNO_SYSTEM_EXPORT bool GLGuiSystem::initialize()
{
	ImGuiWrapper::getInstance().initialize();
	g_pCoreSystem->getLogSystem()->printLog("GLGuiSystem has been initialized.");
	return true;
}

INNO_SYSTEM_EXPORT bool GLGuiSystem::update()
{
	ImGuiWrapper::getInstance().update();
	return true;
}

INNO_SYSTEM_EXPORT bool GLGuiSystem::terminate()
{
	GLGuiSystemNS::m_objectStatus = objectStatus::STANDBY;
	ImGuiWrapper::getInstance().terminate();

	GLGuiSystemNS::m_objectStatus = objectStatus::SHUTDOWN;
	g_pCoreSystem->getLogSystem()->printLog("GLGuiSystem has been terminated.");
	return true;
}

INNO_SYSTEM_EXPORT objectStatus GLGuiSystem::getStatus()
{
	return GLGuiSystemNS::m_objectStatus;
}

void ImGuiWrapper::setup()
{
	// Setup Dear ImGui binding
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;   // Enable Gamepad Controls
	ImGui_ImplGlfwGL3_Init(GLWindowSystemSingletonComponent::getInstance().m_window, true);

	// Setup style
	ImGui::StyleColorsDark();
	//ImGui::StyleColorsClassic();

	// Load Fonts
	io.Fonts->AddFontFromFileTTF("..//res//fonts//FreeSans.otf", 16.0f);
}

void ImGuiWrapper::initialize()
{
}

void ImGuiWrapper::update()
{
	auto l_renderTargetSize = ImVec2((float)RenderingSystemSingletonComponent::getInstance().m_renderTargetSize.x / 4.0f, (float)RenderingSystemSingletonComponent::getInstance().m_renderTargetSize.y / 4.0f);
#ifdef DEBUG
#ifndef INNO_PLATFORM_LINUX64

	static bool l_zoom = false;
	static bool l_showRenderPassResult = false;
	ImGui_ImplGlfwGL3_NewFrame();
	{
		ImGui::Begin("Profiler", 0, ImGuiWindowFlags_AlwaysAutoResize);
		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
		ImGui::Checkbox("Use zoom", &l_zoom);
		ImGui::Checkbox("Show render pass result", &l_showRenderPassResult);
		ImGui::End();
	}
	if(l_showRenderPassResult)
	{
		ImGui::Begin("Light Pass", 0, ImGuiWindowFlags_AlwaysAutoResize);
		{
			ImGui::Image(ImTextureID((GLuint64)LightRenderPassSingletonComponent::getInstance().m_GLTDC->m_TAO), l_renderTargetSize, ImVec2(0.0, 1.0), ImVec2(1.0, 0.0));
			zoom(l_zoom, ImTextureID((GLuint64)LightRenderPassSingletonComponent::getInstance().m_GLTDC->m_TAO), l_renderTargetSize);
		}
		ImGui::End();

		ImGui::Begin("Geometry Pass", 0, ImGuiWindowFlags_AlwaysAutoResize);
		{
			{
				ImGui::BeginChild("World Space Position(RGB) + Metallic(A)", l_renderTargetSize, true, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar);
				ImGui::Text("World Space Position(RGB) + Metallic(A)");
				ImGui::Image(ImTextureID((GLuint64)GeometryRenderPassSingletonComponent::getInstance().m_GLTDCs[0]->m_TAO), l_renderTargetSize, ImVec2(0.0, 1.0), ImVec2(1.0, 0.0));
				ImGui::EndChild();

				ImGui::SameLine();

				ImGui::BeginChild("World Space Normal(RGB) + Roughness(A)", l_renderTargetSize, true, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar);
				ImGui::Text("World Space Normal(RGB) + Roughness(A)");
				ImGui::Image(ImTextureID((GLuint64)GeometryRenderPassSingletonComponent::getInstance().m_GLTDCs[1]->m_TAO), l_renderTargetSize, ImVec2(0.0, 1.0), ImVec2(1.0, 0.0));
				ImGui::EndChild();
			}
			{
				ImGui::BeginChild("Albedo(RGB) + Ambient Occlusion(A)", l_renderTargetSize, true, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar);
				ImGui::Text("Albedo(RGB) + Ambient Occlusion(A)");
				ImGui::Image(ImTextureID((GLuint64)GeometryRenderPassSingletonComponent::getInstance().m_GLTDCs[2]->m_TAO), l_renderTargetSize, ImVec2(0.0, 1.0), ImVec2(1.0, 0.0));
				ImGui::EndChild();

				ImGui::SameLine();

				ImGui::BeginChild("Screen Space Motion Vector", l_renderTargetSize, true, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar);
				ImGui::Text("Screen Space Motion Vector");
				ImGui::Image(ImTextureID((GLuint64)GeometryRenderPassSingletonComponent::getInstance().m_GLTDCs[3]->m_TAO), l_renderTargetSize, ImVec2(0.0, 1.0), ImVec2(1.0, 0.0));
				ImGui::EndChild();
			}
			{
				ImGui::BeginChild("Light Space Position 0", l_renderTargetSize, true, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar);
				ImGui::Text("Light Space Position 0");
				ImGui::Image(ImTextureID((GLuint64)GeometryRenderPassSingletonComponent::getInstance().m_GLTDCs[4]->m_TAO), l_renderTargetSize, ImVec2(0.0, 1.0), ImVec2(1.0, 0.0));
				ImGui::EndChild();

				ImGui::SameLine();

				ImGui::BeginChild("Light Space Position 1", l_renderTargetSize, true, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar);
				ImGui::Text("Light Space Position 1");
				ImGui::Image(ImTextureID((GLuint64)GeometryRenderPassSingletonComponent::getInstance().m_GLTDCs[5]->m_TAO), l_renderTargetSize, ImVec2(0.0, 1.0), ImVec2(1.0, 0.0));
				ImGui::EndChild();
			}
			{
				ImGui::BeginChild("Light Space Position 2", l_renderTargetSize, true, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar);
				ImGui::Text("Light Space Position 2");
				ImGui::Image(ImTextureID((GLuint64)GeometryRenderPassSingletonComponent::getInstance().m_GLTDCs[6]->m_TAO), l_renderTargetSize, ImVec2(0.0, 1.0), ImVec2(1.0, 0.0));
				ImGui::EndChild();

				ImGui::SameLine();

				ImGui::BeginChild("Light Space Position 3", l_renderTargetSize, true, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar);
				ImGui::Text("Light Space Position 3");
				ImGui::Image(ImTextureID((GLuint64)GeometryRenderPassSingletonComponent::getInstance().m_GLTDCs[7]->m_TAO), l_renderTargetSize, ImVec2(0.0, 1.0), ImVec2(1.0, 0.0));
				ImGui::EndChild();
			}
		}
		ImGui::End();

		auto l_shadowPassWindowSize = ImVec2(128.0, 128.0);
		ImGui::Begin("Shadow Pass", 0, ImGuiWindowFlags_AlwaysAutoResize);
		ImGui::BeginChild("Shadow Pass Depth Buffer 0", l_shadowPassWindowSize, true, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar);
		ImGui::Image(ImTextureID((GLuint64)ShadowRenderPassSingletonComponent::getInstance().m_GLTDCs[0]->m_TAO), l_shadowPassWindowSize, ImVec2(0.0, 1.0), ImVec2(1.0, 0.0));
		ImGui::EndChild();
		ImGui::SameLine();
		ImGui::BeginChild("Shadow Pass Depth Buffer 1", l_shadowPassWindowSize, true, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar);
		ImGui::Image(ImTextureID((GLuint64)ShadowRenderPassSingletonComponent::getInstance().m_GLTDCs[1]->m_TAO), l_shadowPassWindowSize, ImVec2(0.0, 1.0), ImVec2(1.0, 0.0));
		ImGui::EndChild();
		ImGui::BeginChild("Shadow Pass Depth Buffer 2", l_shadowPassWindowSize, true, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar);
		ImGui::Image(ImTextureID((GLuint64)ShadowRenderPassSingletonComponent::getInstance().m_GLTDCs[2]->m_TAO), l_shadowPassWindowSize, ImVec2(0.0, 1.0), ImVec2(1.0, 0.0));
		ImGui::EndChild();
		ImGui::SameLine();
		ImGui::BeginChild("Shadow Pass Depth Buffer 3", l_shadowPassWindowSize, true, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar);
		ImGui::Image(ImTextureID((GLuint64)ShadowRenderPassSingletonComponent::getInstance().m_GLTDCs[3]->m_TAO), l_shadowPassWindowSize, ImVec2(0.0, 1.0), ImVec2(1.0, 0.0));
		ImGui::EndChild();
		ImGui::End();
	}

	// Rendering
	glViewport(0, 0, (GLsizei)WindowSystemSingletonComponent::getInstance().m_windowResolution.x, (GLsizei)WindowSystemSingletonComponent::getInstance().m_windowResolution.y);
	ImGui::Render();
	ImGui_ImplGlfwGL3_RenderDrawData(ImGui::GetDrawData());
#else
	// @TODO: Linux ImGui WIP
#endif
#else
	ImGui_ImplGlfwGL3_NewFrame();
	{
		// @TODO: handle GUI component
		ImGui::Begin("Main Menu", 0, ImGuiWindowFlags_AlwaysAutoResize);
		ImGui::Button("Start");
		ImGui::End();
		glViewport(0, 0, (GLsizei)WindowSystemSingletonComponent::getInstance().m_windowResolution.x, (GLsizei)WindowSystemSingletonComponent::getInstance().m_windowResolution.y);
		ImGui::Render();
		ImGui_ImplGlfwGL3_RenderDrawData(ImGui::GetDrawData());
	}
#endif

}

void ImGuiWrapper::terminate()
{
}

void ImGuiWrapper::zoom(bool zoom, ImTextureID textureID, ImVec2 renderTargetSize)
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
			float region_y = io.MousePos.y - pos.y - region_sz * 0.5f; if (region_y < 0.0f) region_y = 0.0f; else if (region_y > renderTargetSize.y - region_sz) region_y = renderTargetSize.y - region_sz;
			float zoom = 4.0f;
			ImGui::Text("Min: (%.2f, %.2f)", region_x, region_y);
			ImGui::Text("Max: (%.2f, %.2f)", region_x + region_sz, region_y + region_sz);
			ImVec2 uv0 = ImVec2((region_x) / renderTargetSize.x, (region_y) / renderTargetSize.y);
			ImVec2 uv1 = ImVec2((region_x + region_sz) / renderTargetSize.x, (region_y + region_sz) / renderTargetSize.y);
			ImGui::Image(textureID, ImVec2(region_sz * zoom, region_sz * zoom), uv0, uv1, ImColor(255, 255, 255, 255), ImColor(255, 255, 255, 128));
			ImGui::EndTooltip();
		}
	}
}
