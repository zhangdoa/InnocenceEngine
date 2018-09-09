#include "GLGuiSystem.h"

void GLGuiSystem::setup()
{
	// Setup Dear ImGui binding
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;   // Enable Gamepad Controls
	ImGui_ImplGlfwGL3_Init(WindowSystemSingletonComponent::getInstance().m_window, true);

	// Setup style
	ImGui::StyleColorsDark();
	//ImGui::StyleColorsClassic();

	// Load Fonts
	io.Fonts->AddFontFromFileTTF("..//res//fonts//FreeSans.otf", 16.0f);

	m_objectStatus = objectStatus::ALIVE;
}

void GLGuiSystem::initialize()
{
	g_pLogSystem->printLog("GLGuiSystem has been initialized.");
}

void GLGuiSystem::update()
{
	auto l_renderTargetSize = ImVec2((float)RenderingSystemSingletonComponent::getInstance().m_renderTargetSize.x, (float)RenderingSystemSingletonComponent::getInstance().m_renderTargetSize.y);
#ifdef BUILD_EDITOR
#ifndef INNO_PLATFORM_LINUX64
	const char* items[] = { "Final Pass", "Light Pass", "Geometry Pass", "Shadow Pass" };
	static const char* item_current = items[0];

	ImGui_ImplGlfwGL3_NewFrame();
	{
		ImGui::Begin("Global Settings", 0, ImGuiWindowFlags_AlwaysAutoResize);

		static float f = 0.0f;
		static int counter = 0;
		ImGui::Text("Global Settings");                           // Display some text (you can use a format string too)
		ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
																//ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color

																//ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our windows open/close state
																//ImGui::Checkbox("Another Window", &show_another_window);

		if (ImGui::Button("Button"))                            // Buttons return true when clicked (NB: most widgets return true when edited/activated)
			counter++;
		ImGui::SameLine();
		ImGui::Text("counter = %d", counter);

		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

		// Here our selection is a single pointer stored outside the object.
		if (ImGui::BeginCombo("Active render pass result", item_current)) // The second parameter is the label previewed before opening the combo.
		{
			for (int n = 0; n < IM_ARRAYSIZE(items); n++)
			{
				bool is_selected = (item_current == items[n]);
				if (ImGui::Selectable(items[n], is_selected))
					item_current = items[n];
				if (is_selected)
					ImGui::SetItemDefaultFocus();   // Set the initial focus when opening the combo (scrolling + for keyboard navigation support in the upcoming navigation branch)
			}
			ImGui::EndCombo();
		}

		ImGui::End();
	}

	{
		if (item_current == items[0])
		{
			ImGui::Begin("Final Pass", 0, ImGuiWindowFlags_AlwaysAutoResize); \
				ImGui::Image(ImTextureID((GLuint64)FinalRenderPassSingletonComponent::getInstance().m_finalBlendPassTexture.m_TAO), l_renderTargetSize, ImVec2(1.0, 1.0), ImVec2(0.0, 0.0));
			ImGui::End();
		}
		else if (item_current == items[1])
		{
			ImGui::Begin("Light Pass", 0, ImGuiWindowFlags_AlwaysAutoResize);
			ImGui::Image(ImTextureID((GLuint64)LightRenderPassSingletonComponent::getInstance().m_lightPassTexture.m_TAO), l_renderTargetSize, ImVec2(1.0, 1.0), ImVec2(0.0, 0.0));
			ImGui::End();
		}
		else if (item_current == items[2])
		{
			ImGui::Begin("Geometry Pass", 0, ImGuiWindowFlags_AlwaysAutoResize);
			ImGui::BeginChild("World Space Position(RGB) + Metallic(A)", l_renderTargetSize, true, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar);
			ImGui::Image(ImTextureID((GLuint64)GeometryRenderPassSingletonComponent::getInstance().m_geometryPassTextures[0].m_TAO), l_renderTargetSize, ImVec2(1.0, 1.0), ImVec2(0.0, 0.0));
			ImGui::EndChild();
			ImGui::BeginChild("World Space Normal(RGB) + Roughness(A)", l_renderTargetSize, true, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar);
			ImGui::Image(ImTextureID((GLuint64)GeometryRenderPassSingletonComponent::getInstance().m_geometryPassTextures[1].m_TAO), l_renderTargetSize, ImVec2(1.0, 1.0), ImVec2(0.0, 0.0));
			ImGui::EndChild();
			ImGui::BeginChild("Albedo(RGB) + Ambient Occlusion(A)", l_renderTargetSize, true, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar);
			ImGui::Image(ImTextureID((GLuint64)GeometryRenderPassSingletonComponent::getInstance().m_geometryPassTextures[2].m_TAO), l_renderTargetSize, ImVec2(1.0, 1.0), ImVec2(0.0, 0.0));
			ImGui::EndChild();
			ImGui::BeginChild("Screen Space Motion Vector", l_renderTargetSize, true, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar);
			ImGui::Image(ImTextureID((GLuint64)GeometryRenderPassSingletonComponent::getInstance().m_geometryPassTextures[3].m_TAO), l_renderTargetSize, ImVec2(1.0, 1.0), ImVec2(0.0, 0.0));
			ImGui::EndChild();
			ImGui::BeginChild("Light Space Position 0", l_renderTargetSize, true, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar);
			ImGui::Image(ImTextureID((GLuint64)GeometryRenderPassSingletonComponent::getInstance().m_geometryPassTextures[4].m_TAO), l_renderTargetSize, ImVec2(1.0, 1.0), ImVec2(0.0, 0.0));
			ImGui::EndChild();
			ImGui::BeginChild("Light Space Position 1", l_renderTargetSize, true, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar);
			ImGui::Image(ImTextureID((GLuint64)GeometryRenderPassSingletonComponent::getInstance().m_geometryPassTextures[5].m_TAO), l_renderTargetSize, ImVec2(1.0, 1.0), ImVec2(0.0, 0.0));
			ImGui::EndChild();
			ImGui::BeginChild("Light Space Position 2", l_renderTargetSize, true, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar);
			ImGui::Image(ImTextureID((GLuint64)GeometryRenderPassSingletonComponent::getInstance().m_geometryPassTextures[6].m_TAO), l_renderTargetSize, ImVec2(1.0, 1.0), ImVec2(0.0, 0.0));
			ImGui::EndChild();
			ImGui::BeginChild("Light Space Position 3", l_renderTargetSize, true, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar);
			ImGui::Image(ImTextureID((GLuint64)GeometryRenderPassSingletonComponent::getInstance().m_geometryPassTextures[7].m_TAO), l_renderTargetSize, ImVec2(1.0, 1.0), ImVec2(0.0, 0.0));
			ImGui::EndChild();
			ImGui::End();
		}
		else if (item_current == items[3])
		{
			auto l_shadowPassWindowSize = ImVec2(512.0, 512.0);
			ImGui::Begin("Shadow Pass", 0, ImGuiWindowFlags_AlwaysAutoResize);
			ImGui::BeginChild("Shadow Pass Depth Buffer 0", l_shadowPassWindowSize, true, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar);
			ImGui::Image(ImTextureID((GLuint64)ShadowRenderPassSingletonComponent::getInstance().m_frameBufferTextureVector[0].second.m_TAO), l_shadowPassWindowSize, ImVec2(1.0, 1.0), ImVec2(0.0, 0.0));
			ImGui::EndChild();
			ImGui::BeginChild("Shadow Pass Depth Buffer 1", l_shadowPassWindowSize, true, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar);
			ImGui::Image(ImTextureID((GLuint64)ShadowRenderPassSingletonComponent::getInstance().m_frameBufferTextureVector[1].second.m_TAO), l_shadowPassWindowSize, ImVec2(1.0, 1.0), ImVec2(0.0, 0.0));
			ImGui::EndChild();
			ImGui::BeginChild("Shadow Pass Depth Buffer 2", l_shadowPassWindowSize, true, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar);
			ImGui::Image(ImTextureID((GLuint64)ShadowRenderPassSingletonComponent::getInstance().m_frameBufferTextureVector[2].second.m_TAO), l_shadowPassWindowSize, ImVec2(1.0, 1.0), ImVec2(0.0, 0.0));
			ImGui::EndChild();
			ImGui::BeginChild("Shadow Pass Depth Buffer 3", l_shadowPassWindowSize, true, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar);
			ImGui::Image(ImTextureID((GLuint64)ShadowRenderPassSingletonComponent::getInstance().m_frameBufferTextureVector[3].second.m_TAO), l_shadowPassWindowSize, ImVec2(1.0, 1.0), ImVec2(0.0, 0.0));
			ImGui::EndChild();
			ImGui::End();
		}
	}

	// Rendering
	glViewport(0, 0, (GLsizei)WindowSystemSingletonComponent::getInstance().m_windowResolution.x, (GLsizei)WindowSystemSingletonComponent::getInstance().m_windowResolution.y);
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	ImGui::Render();
	ImGui_ImplGlfwGL3_RenderDrawData(ImGui::GetDrawData());
#else
	//@TODO: Linux ImGui WIP
#endif
#else
	ImGui_ImplGlfwGL3_NewFrame();
	{
		//@TODO: handle GUI component
		ImGui::Begin("Main Menu", 0, ImGuiWindowFlags_AlwaysAutoResize);
		ImGui::Button("Start");
		ImGui::End();
		glViewport(0, 0, (GLsizei)WindowSystemSingletonComponent::getInstance().m_windowResolution.x, (GLsizei)WindowSystemSingletonComponent::getInstance().m_windowResolution.y);
		ImGui::Render();
		ImGui_ImplGlfwGL3_RenderDrawData(ImGui::GetDrawData());
	}
#endif
}

void GLGuiSystem::shutdown()
{
	m_objectStatus = objectStatus::STANDBY;

	m_objectStatus = objectStatus::SHUTDOWN;
	g_pLogSystem->printLog("GLGuiSystem has been shutdown.");
}

const objectStatus & GLGuiSystem::getStatus() const
{
	return m_objectStatus;
}
