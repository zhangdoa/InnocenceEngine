#include "GLGuiSystem.h"
#include "../component/WindowSystemComponent.h"
#include "../component/GLWindowSystemComponent.h"
#include "../component/GLEnvironmentRenderPassComponent.h"
#include "../component/GLShadowRenderPassComponent.h"
#include "../component/GLGeometryRenderPassComponent.h"
#include "../component/GLTerrainRenderPassComponent.h"
#include "../component/GLLightRenderPassComponent.h"
#include "../component/GLFinalRenderPassComponent.h"
#include "../component/RenderingSystemComponent.h"
#include "../component/GLRenderingSystemComponent.h"
#include "../component/AssetSystemComponent.h"
#include "../component/GameSystemComponent.h"
#include "../component/PhysicsSystemComponent.h"

#include "../third-party/ImGui/imgui.h"
#include "../third-party/ImGui/imgui_impl_glfw.h"
#include "../third-party/ImGui/imgui_impl_opengl3.h"

#include "ICoreSystem.h"

extern ICoreSystem* g_pCoreSystem;

struct RenderingConfig
{
	bool useTAA = false;
	bool useBloom = false;
	bool useZoom = false;
	bool drawTerrain = false;
	bool drawSky = false;
	bool drawOverlapWireframe = false;
	bool showRenderPassResult = false;
};

struct GameConfig
{
	bool pauseGameUpdate = false;
};

class ImGuiWrapper
{
public:
	~ImGuiWrapper() {};

	static ImGuiWrapper& get()
	{
		static ImGuiWrapper instance;
		return instance;
	}
	void setup();
	void initialize();
	void update();
	void terminate();

	void showFileExplorer();
	void showWorldExplorer();
	void zoom(bool zoom, ImTextureID textureID, ImVec2 renderTargetSize);

	void setRenderingConfig();
private:
	ImGuiWrapper() {};

	void showTransformComponentPropertyEditor(void* rhs)
	{
		auto l_rhs = reinterpret_cast<TransformComponent*>(rhs);

		ImGui::Text("Transform Hierarchy Level: %.i", l_rhs->m_transformHierarchyLevel);

		ImGui::Text("Local Transform Vector");

		static float float_min = std::numeric_limits<float>::min();
		static float float_max = std::numeric_limits<float>::max();

		static float pos[4];
		pos[0] = l_rhs->m_localTransformVector.m_pos.x;
		pos[1] = l_rhs->m_localTransformVector.m_pos.y;
		pos[2] = l_rhs->m_localTransformVector.m_pos.z;
		pos[3] = 0.0f;

		if (ImGui::DragFloat3("Position", pos, 0.0001f, float_min, float_max))
		{
			l_rhs->m_localTransformVector.m_pos.x = pos[0];
			l_rhs->m_localTransformVector.m_pos.y = pos[1];
			l_rhs->m_localTransformVector.m_pos.z = pos[2];
		}

		static float rot_min = 0.0f;
		static float rot_max = 360.0f;

		static float rot[4];
		vec4 eulerAngles = InnoMath::quatToEulerAngle(l_rhs->m_localTransformVector.m_rot);
		rot[0] = InnoMath::radianToAngle(eulerAngles.x);
		rot[1] = InnoMath::radianToAngle(eulerAngles.y);
		rot[2] = InnoMath::radianToAngle(eulerAngles.z);
		rot[3] = 0.0f;

		if (ImGui::DragFloat3("Rotation", rot, 0.01f, rot_min, rot_max))
		{
			auto roll = InnoMath::angleToRadian(rot[0]);
			auto pitch = InnoMath::angleToRadian(rot[1]);
			auto yaw = InnoMath::angleToRadian(rot[2]);

			l_rhs->m_localTransformVector.m_rot = InnoMath::eulerAngleToQuat(roll, pitch, yaw);
		}
	}
	void showVisiableComponentPropertyEditor(void* rhs)
	{
		auto l_rhs = reinterpret_cast<VisibleComponent*>(rhs);

		static MaterialDataComponent* selectedComponent = nullptr;

		{
			ImGui::BeginChild("Children MaterialDataComponents", ImVec2(ImGui::GetWindowContentRegionWidth() * 0.2f, 360.0f), true, ImGuiWindowFlags_HorizontalScrollbar);
			{
				for (auto& i : l_rhs->m_modelMap)
				{
					if (ImGui::Selectable(std::to_string(i.first->m_parentEntity).c_str(), selectedComponent == i.second))
					{
						selectedComponent = i.second;
					}
				}
			}
			ImGui::EndChild();
		}

		ImGui::SameLine();

		{
			if (selectedComponent)
			{
				ImGui::BeginChild("MaterialDataComponent Editor", ImVec2(ImGui::GetWindowContentRegionWidth() * 0.8f, 360.0f), true, ImGuiWindowFlags_HorizontalScrollbar);
				{
					auto l_material = &selectedComponent->m_meshCustomMaterial;

					static float float_min = 0.0f;
					static float float_max = 1.0f;

					static ImVec4 albedo = ImColor(l_material->albedo_r, l_material->albedo_g, l_material->albedo_b, l_material->alpha);

					if (ImGui::ColorPicker4("Albedo Color", (float*)&albedo, ImGuiColorEditFlags_RGB))
					{
						l_material->albedo_r = albedo.x;
						l_material->albedo_g = albedo.y;
						l_material->albedo_b = albedo.z;
						l_material->alpha = albedo.w;
					}

					ImGui::SameLine();

					const ImVec2 small_slider_size(18, 180);
	
					static float metallic;
					if (ImGui::VSliderFloat("Metallic", small_slider_size, &metallic, 0.0f, 1.0f, ""))
					{
						l_material->metallic = metallic;
					}

					ImGui::SameLine();

					static float roughness;
					if (ImGui::VSliderFloat("Roughness", small_slider_size, &roughness, 0.0f, 1.0f, ""))
					{
						l_material->roughness = roughness;
					}

					ImGui::SameLine();

					static float ao;
					if (ImGui::VSliderFloat("Ambient Occlusion", small_slider_size, &ao, 0.0f, 1.0f, ""))
					{
						l_material->ao = ao;
					}
				}
				ImGui::EndChild();
			}
		}
	}
};

INNO_PRIVATE_SCOPE GLGuiSystemNS
{
	ObjectStatus m_objectStatus = ObjectStatus::SHUTDOWN;
}

INNO_SYSTEM_EXPORT bool GLGuiSystem::setup()
{
	ImGuiWrapper::get().setup();
	GLGuiSystemNS::m_objectStatus = ObjectStatus::ALIVE;
	return true;
}

INNO_SYSTEM_EXPORT bool GLGuiSystem::initialize()
{
	ImGuiWrapper::get().initialize();
	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "GLGuiSystem has been initialized.");
	return true;
}

INNO_SYSTEM_EXPORT bool GLGuiSystem::update()
{
	ImGuiWrapper::get().update();
	return true;
}

INNO_SYSTEM_EXPORT bool GLGuiSystem::terminate()
{
	GLGuiSystemNS::m_objectStatus = ObjectStatus::STANDBY;
	ImGuiWrapper::get().terminate();

	GLGuiSystemNS::m_objectStatus = ObjectStatus::SHUTDOWN;
	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "GLGuiSystem has been terminated.");
	return true;
}

INNO_SYSTEM_EXPORT ObjectStatus GLGuiSystem::getStatus()
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
	ImGui_ImplGlfw_InitForOpenGL(GLWindowSystemComponent::get().m_window, true);
	ImGui_ImplOpenGL3_Init(NULL);

	// Setup style
	ImGui::StyleColorsDark();

	// Load Fonts
	io.Fonts->AddFontFromFileTTF("..//res//fonts//FreeSans.otf", 16.0f);
}

void ImGuiWrapper::initialize()
{
}

void ImGuiWrapper::update()
{
	auto l_renderTargetSize = ImVec2((float)WindowSystemComponent::get().m_windowResolution.x / 4.0f, (float)WindowSystemComponent::get().m_windowResolution.y / 4.0f);
#ifdef DEBUG
#ifndef INNO_PLATFORM_LINUX64

	static RenderingConfig l_renderingConfig;
	static GameConfig l_gameConfig;

	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
	{
		ImGui::Begin("Profiler", 0, ImGuiWindowFlags_AlwaysAutoResize);
		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
		ImGui::Checkbox("Use TAA", &l_renderingConfig.useTAA);
		ImGui::Checkbox("Use Bloom", &l_renderingConfig.useBloom);
		ImGui::Checkbox("Use zoom", &l_renderingConfig.useZoom);
		ImGui::Checkbox("Draw terrain", &l_renderingConfig.drawTerrain);
		ImGui::Checkbox("Draw sky", &l_renderingConfig.drawSky);
		ImGui::Checkbox("Draw overlap wireframe", &l_renderingConfig.drawOverlapWireframe);
		ImGui::Checkbox("Show render pass result", &l_renderingConfig.showRenderPassResult);
		ImGui::Checkbox("Pause game update", &l_gameConfig.pauseGameUpdate);
		const char* items[] = { "OpaquePass", "TransparentPass", "TerrainPass", "LightPass", "FinalPass" };
		static int item_current = 0;
		ImGui::Combo("Choose shader", &item_current, items, IM_ARRAYSIZE(items));
		if (ImGui::Button("Reload Shader"))
		{
			RenderingSystemComponent::get().f_reloadShader(RenderPassType(item_current));
		}
		if (ImGui::Button("Capture environment"))
		{
			RenderingSystemComponent::get().f_captureEnvironment();
		}
		ImGui::End();

		RenderingSystemComponent::get().m_useTAA = l_renderingConfig.useTAA;
		RenderingSystemComponent::get().m_useBloom = l_renderingConfig.useBloom;
		RenderingSystemComponent::get().m_drawTerrain = l_renderingConfig.drawTerrain;
		RenderingSystemComponent::get().m_drawSky = l_renderingConfig.drawSky;
		RenderingSystemComponent::get().m_drawOverlapWireframe = l_renderingConfig.drawOverlapWireframe;

		GameSystemComponent::get().m_pauseGameUpdate = l_gameConfig.pauseGameUpdate;

		if (l_renderingConfig.showRenderPassResult)
		{
			ImGui::Begin("Opaque Pass", 0, ImGuiWindowFlags_AlwaysAutoResize);
			{
				{
					ImGui::BeginChild("World Space Position(RGB) + Metallic(A)", l_renderTargetSize, true, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar);
					ImGui::Text("World Space Position(RGB) + Metallic(A)");
					ImGui::Image(ImTextureID((GLuint64)GLGeometryRenderPassComponent::get().m_opaquePass_GLRPC->m_GLTDCs[0]->m_TAO), l_renderTargetSize, ImVec2(0.0, 1.0), ImVec2(1.0, 0.0));
					ImGui::EndChild();

					ImGui::SameLine();

					ImGui::BeginChild("World Space Normal(RGB) + Roughness(A)", l_renderTargetSize, true, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar);
					ImGui::Text("World Space Normal(RGB) + Roughness(A)");
					ImGui::Image(ImTextureID((GLuint64)GLGeometryRenderPassComponent::get().m_opaquePass_GLRPC->m_GLTDCs[1]->m_TAO), l_renderTargetSize, ImVec2(0.0, 1.0), ImVec2(1.0, 0.0));
					ImGui::EndChild();
				}
				{
					ImGui::BeginChild("Albedo(RGB) + Ambient Occlusion(A)", l_renderTargetSize, true, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar);
					ImGui::Text("Albedo(RGB) + Ambient Occlusion(A)");
					ImGui::Image(ImTextureID((GLuint64)GLGeometryRenderPassComponent::get().m_opaquePass_GLRPC->m_GLTDCs[2]->m_TAO), l_renderTargetSize, ImVec2(0.0, 1.0), ImVec2(1.0, 0.0));
					ImGui::EndChild();

					ImGui::SameLine();

					ImGui::BeginChild("Screen Space Motion Vector", l_renderTargetSize, true, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar);
					ImGui::Text("Screen Space Motion Vector");
					ImGui::Image(ImTextureID((GLuint64)GLGeometryRenderPassComponent::get().m_opaquePass_GLRPC->m_GLTDCs[3]->m_TAO), l_renderTargetSize, ImVec2(0.0, 1.0), ImVec2(1.0, 0.0));
					ImGui::EndChild();
				}
			}
			ImGui::End();

			ImGui::Begin("SSAO Pass", 0, ImGuiWindowFlags_AlwaysAutoResize);
			{
				ImGui::Image(ImTextureID((GLuint64)GLGeometryRenderPassComponent::get().m_SSAOPass_GLRPC->m_GLTDCs[0]->m_TAO), l_renderTargetSize, ImVec2(0.0, 1.0), ImVec2(1.0, 0.0));
				zoom(l_renderingConfig.useZoom, ImTextureID((GLuint64)GLGeometryRenderPassComponent::get().m_SSAOPass_GLRPC->m_GLTDCs[0]->m_TAO), l_renderTargetSize);
				
				ImGui::Image(ImTextureID((GLuint64)GLGeometryRenderPassComponent::get().m_SSAOBlurPass_GLRPC->m_GLTDCs[0]->m_TAO), l_renderTargetSize, ImVec2(0.0, 1.0), ImVec2(1.0, 0.0));

				ImGui::Image(ImTextureID((GLuint64)GLGeometryRenderPassComponent::get().m_noiseGLTDC->m_TAO), l_renderTargetSize, ImVec2(0.0, 1.0), ImVec2(1.0, 0.0));
			}
			ImGui::End();

			ImGui::Begin("Transparent Pass", 0, ImGuiWindowFlags_AlwaysAutoResize);
			{
				ImGui::Image(ImTextureID((GLuint64)GLGeometryRenderPassComponent::get().m_transparentPass_GLRPC->m_GLTDCs[0]->m_TAO), l_renderTargetSize, ImVec2(0.0, 1.0), ImVec2(1.0, 0.0));
				zoom(l_renderingConfig.useZoom, ImTextureID((GLuint64)GLGeometryRenderPassComponent::get().m_transparentPass_GLRPC->m_GLTDCs[0]->m_TAO), l_renderTargetSize);
			}
			ImGui::End();

			ImGui::Begin("Terrain Pass", 0, ImGuiWindowFlags_AlwaysAutoResize);
			{
				ImGui::Image(ImTextureID((GLuint64)GLTerrainRenderPassComponent::get().m_GLRPC->m_GLTDCs[0]->m_TAO), l_renderTargetSize, ImVec2(0.0, 1.0), ImVec2(1.0, 0.0));
				zoom(l_renderingConfig.useZoom, ImTextureID((GLuint64)GLTerrainRenderPassComponent::get().m_GLRPC->m_GLTDCs[0]->m_TAO), l_renderTargetSize);
			}
			ImGui::End();

			ImGui::Begin("Light Pass", 0, ImGuiWindowFlags_AlwaysAutoResize);
			{
				ImGui::Image(ImTextureID((GLuint64)GLLightRenderPassComponent::get().m_GLRPC->m_GLTDCs[0]->m_TAO), l_renderTargetSize, ImVec2(0.0, 1.0), ImVec2(1.0, 0.0));
				zoom(l_renderingConfig.useZoom, ImTextureID((GLuint64)GLLightRenderPassComponent::get().m_GLRPC->m_GLTDCs[0]->m_TAO), l_renderTargetSize);
			}
			ImGui::End();

			ImGui::Begin("Final Pass", 0, ImGuiWindowFlags_AlwaysAutoResize);
			{
				{
					ImGui::BeginChild("Sky Pass", l_renderTargetSize, true, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar);
					ImGui::Text("Sky Pass");
					ImGui::Image(ImTextureID((GLuint64)GLFinalRenderPassComponent::get().m_skyPassGLRPC->m_GLTDCs[0]->m_TAO), l_renderTargetSize, ImVec2(0.0, 1.0), ImVec2(1.0, 0.0));
					ImGui::EndChild();

					ImGui::SameLine();

					ImGui::BeginChild("Pre TAA Pass", l_renderTargetSize, true, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar);
					ImGui::Text("Pre TAA Pass");
					ImGui::Image(ImTextureID((GLuint64)GLFinalRenderPassComponent::get().m_preTAAPassGLRPC->m_GLTDCs[0]->m_TAO), l_renderTargetSize, ImVec2(0.0, 1.0), ImVec2(1.0, 0.0));
					ImGui::EndChild();
				}
				{
					ImGui::BeginChild("TAA Ping Pass", l_renderTargetSize, true, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar);
					ImGui::Text("TAA Ping Pass");
					ImGui::Image(ImTextureID((GLuint64)GLFinalRenderPassComponent::get().m_TAAPingPassGLRPC->m_GLTDCs[0]->m_TAO), l_renderTargetSize, ImVec2(0.0, 1.0), ImVec2(1.0, 0.0));
					ImGui::EndChild();

					ImGui::SameLine();

					ImGui::BeginChild("TAA Sharpen Pass", l_renderTargetSize, true, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar);
					ImGui::Text("TAA Sharpen Pass");
					ImGui::Image(ImTextureID((GLuint64)GLFinalRenderPassComponent::get().m_TAASharpenPassGLRPC->m_GLTDCs[0]->m_TAO), l_renderTargetSize, ImVec2(0.0, 1.0), ImVec2(1.0, 0.0));
					ImGui::EndChild();
				}
				{
					ImGui::BeginChild("Bloom Extract Pass", l_renderTargetSize, true, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar);
					ImGui::Text("Bloom Extract Pass");
					ImGui::Image(ImTextureID((GLuint64)GLFinalRenderPassComponent::get().m_bloomExtractPassGLRPC->m_GLTDCs[0]->m_TAO), l_renderTargetSize, ImVec2(0.0, 1.0), ImVec2(1.0, 0.0));
					ImGui::EndChild();

					ImGui::SameLine();

					ImGui::BeginChild("Bloom Blur Ping Pass", l_renderTargetSize, true, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar);
					ImGui::Text("Bloom Blur Ping Pass");
					ImGui::Image(ImTextureID((GLuint64)GLFinalRenderPassComponent::get().m_bloomBlurPingPassGLRPC->m_GLTDCs[0]->m_TAO), l_renderTargetSize, ImVec2(0.0, 1.0), ImVec2(1.0, 0.0));
					ImGui::EndChild();
				}
				{
					ImGui::BeginChild("Motion Blur Pass", l_renderTargetSize, true, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar);
					ImGui::Text("Motion Blur Pass");
					ImGui::Image(ImTextureID((GLuint64)GLFinalRenderPassComponent::get().m_motionBlurPassGLRPC->m_GLTDCs[0]->m_TAO), l_renderTargetSize, ImVec2(0.0, 1.0), ImVec2(1.0, 0.0));
					ImGui::EndChild();

					ImGui::SameLine();

					ImGui::BeginChild("Billboard Pass", l_renderTargetSize, true, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar);
					ImGui::Text("Billboard Pass");
					ImGui::Image(ImTextureID((GLuint64)GLFinalRenderPassComponent::get().m_billboardPassGLRPC->m_GLTDCs[0]->m_TAO), l_renderTargetSize, ImVec2(0.0, 1.0), ImVec2(1.0, 0.0));
					ImGui::EndChild();
				}
				{
					ImGui::BeginChild("Debugger Pass", l_renderTargetSize, true, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar);
					ImGui::Text("Debugger Pass");
					ImGui::Image(ImTextureID((GLuint64)GLFinalRenderPassComponent::get().m_debuggerPassGLRPC->m_GLTDCs[0]->m_TAO), l_renderTargetSize, ImVec2(0.0, 1.0), ImVec2(1.0, 0.0));
					ImGui::EndChild();

					ImGui::SameLine();

					ImGui::BeginChild("Final Blend Pass", l_renderTargetSize, true, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar);
					ImGui::Text("Final Blend Pass");
					ImGui::Image(ImTextureID((GLuint64)GLFinalRenderPassComponent::get().m_finalBlendPassGLRPC->m_GLTDCs[0]->m_TAO), l_renderTargetSize, ImVec2(0.0, 1.0), ImVec2(1.0, 0.0));
					ImGui::EndChild();
				}
			}
			ImGui::End();

			auto l_shadowPassWindowSize = ImVec2(512.0, 512.0);
			ImGui::Begin("Shadow Pass", 0, ImGuiWindowFlags_AlwaysAutoResize);
			ImGui::Text("Depth Buffer");
			ImGui::Image(ImTextureID((GLuint64)GLShadowRenderPassComponent::get().m_DirLight_GLRPC->m_GLTDCs[0]->m_TAO), l_shadowPassWindowSize, ImVec2(0.0, 1.0), ImVec2(1.0, 0.0));
			ImGui::End();

			auto l_BRDFLUT = ImVec2(128.0, 128.0);
			ImGui::Begin("BRDF lookup table", 0, ImGuiWindowFlags_AlwaysAutoResize);
			{
				ImGui::BeginChild("IBL LUT", l_BRDFLUT, true, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar);
				ImGui::Image(ImTextureID((GLuint64)GLEnvironmentRenderPassComponent::get().m_BRDFSplitSumLUTPassGLTDC->m_TAO), l_BRDFLUT, ImVec2(0.0, 1.0), ImVec2(1.0, 0.0));
				zoom(l_renderingConfig.useZoom, ImTextureID((GLuint64)GLEnvironmentRenderPassComponent::get().m_BRDFSplitSumLUTPassGLTDC->m_TAO), l_BRDFLUT);
				ImGui::EndChild();

				ImGui::SameLine();

				ImGui::BeginChild("Multi-Scattering LUT", l_BRDFLUT, true, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar);
				ImGui::Image(ImTextureID((GLuint64)GLEnvironmentRenderPassComponent::get().m_BRDFMSAverageLUTPassGLTDC->m_TAO), l_BRDFLUT, ImVec2(0.0, 1.0), ImVec2(1.0, 0.0));
				zoom(l_renderingConfig.useZoom, ImTextureID((GLuint64)GLEnvironmentRenderPassComponent::get().m_BRDFMSAverageLUTPassGLTDC->m_TAO), l_BRDFLUT);
				ImGui::EndChild();
			}
			ImGui::End();
		}

		ImGuiWrapper::get().showFileExplorer();
		ImGuiWrapper::get().showWorldExplorer();

		// Rendering
		glViewport(0, 0, (GLsizei)WindowSystemComponent::get().m_windowResolution.x, (GLsizei)WindowSystemComponent::get().m_windowResolution.y);
		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
#else
	// @TODO: Linux ImGui WIP
#endif
#else
	// @TODO: handle GUI component
	ImGui::Begin("Main Menu", 0, ImGuiWindowFlags_AlwaysAutoResize);
	ImGui::Button("Start");
	ImGui::End();
	glViewport(0, 0, (GLsizei)WindowSystemComponent::get().m_windowResolution.x, (GLsizei)WindowSystemComponent::get().m_windowResolution.y);
	ImGui::Render();
	ImGui_ImplGlfwGL3_RenderDrawData(ImGui::GetDrawData());
#endif
	}
}

void ImGuiWrapper::terminate()
{
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
}

void ImGuiWrapper::showFileExplorer()
{
	auto l_iconSize = ImVec2(48.0f, 48.0f);

	std::function<ImTextureID(const FileExplorerIconType iconType)> f_getTextID =
		[&](const FileExplorerIconType iconType) -> ImTextureID
	{
		switch (iconType)
		{
		case FileExplorerIconType::OBJ:
			return ImTextureID((GLuint64)GLRenderingSystemComponent::get().m_iconTemplate_OBJ->m_TAO); break;
		case FileExplorerIconType::PNG:
			return ImTextureID((GLuint64)GLRenderingSystemComponent::get().m_iconTemplate_PNG->m_TAO); break;
		case FileExplorerIconType::SHADER:
			return ImTextureID((GLuint64)GLRenderingSystemComponent::get().m_iconTemplate_SHADER->m_TAO); break;
		case FileExplorerIconType::UNKNOWN:
			return ImTextureID((GLuint64)GLRenderingSystemComponent::get().m_iconTemplate_UNKNOWN->m_TAO); break;
		default:
			return nullptr; break;
		}
	};

	ImGui::Begin("File Explorer", 0);

	static DirectoryMetadata* currentDirectoryMetadata = &AssetSystemComponent::get().m_rootDirectoryMetadata;

	if (ImGui::Button("Return"))
	{
		if (currentDirectoryMetadata->depth > 0)
		{
			currentDirectoryMetadata = currentDirectoryMetadata->parentDirectory;
		}
	}

	ImGuiStyle& style = ImGui::GetStyle();
	float window_visible_x2 = ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x;

	for (size_t i = 0; i < currentDirectoryMetadata->childrenDirectories.size(); i++)
	{
		ImGui::PushID((int)i);

		auto l_currentActivateDir = &currentDirectoryMetadata->childrenDirectories[i];

		ImGui::BeginGroup();
		{
			if (ImGui::ImageButton(f_getTextID(FileExplorerIconType::UNKNOWN), l_iconSize, ImVec2(0.0, 1.0), ImVec2(1.0, 0.0), -1, ImColor(0, 0, 0, 255)))
			{
				currentDirectoryMetadata = l_currentActivateDir;
			}
			ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + l_iconSize.x);
			ImGui::Text((l_currentActivateDir->directoryName).c_str());
			ImGui::PopTextWrapPos();
		}
		ImGui::EndGroup();

		float last_button_x2 = ImGui::GetItemRectMax().x;
		float next_button_x2 = last_button_x2 + style.ItemSpacing.x + l_iconSize.x;
		if (next_button_x2 < window_visible_x2)
		{
			ImGui::SameLine();
		}

		ImGui::PopID();
	}
	for (auto& i : currentDirectoryMetadata->childrenAssets)
	{
		ImGui::BeginGroup();
		{
			ImGui::ImageButton(f_getTextID(i.iconType), l_iconSize, ImVec2(0.0, 1.0), ImVec2(1.0, 0.0), -1, ImColor(0, 0, 0, 255));
			ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + l_iconSize.x);
			ImGui::Text((i.fileName + i.extension).c_str());
			ImGui::PopTextWrapPos();
		}
		ImGui::EndGroup();

		float last_button_x2 = ImGui::GetItemRectMax().x;
		float next_button_x2 = last_button_x2 + style.ItemSpacing.x + l_iconSize.x;
		if (next_button_x2 < window_visible_x2)
		{
			ImGui::SameLine();
		}
	}

	ImGui::End();
}

void ImGuiWrapper::showWorldExplorer()
{
	ImGui::Begin("World Explorer", 0);

	static void* selectedComponent = nullptr;
	static componentType selectedComponentType;

	for (auto& i : GameSystemComponent::get().m_enitityNameMap)
	{
		if (ImGui::TreeNode(i.second.c_str()))
		{
			auto& result = GameSystemComponent::get().m_enitityChildrenComponentsMetadataMap.find(i.first);
			if (result != GameSystemComponent::get().m_enitityChildrenComponentsMetadataMap.end())
			{
				auto& l_componentNameMap = result->second;

				for (auto& j : l_componentNameMap)
				{
					if (ImGui::Selectable(j.second.second.c_str(), selectedComponent == j.first))
					{
						selectedComponent = j.first;
						selectedComponentType = j.second.first;
					}
				}
			}
			ImGui::TreePop();
		}
	}
	ImGui::End();

	ImGui::Begin("Property Editor", 0);
	{
		if (selectedComponent)
		{
			switch (selectedComponentType)
			{
			case componentType::TransformComponent: showTransformComponentPropertyEditor(selectedComponent); break;
			case componentType::VisibleComponent: showVisiableComponentPropertyEditor(selectedComponent); break;
			default:
				break;
			}
		}
	}
	ImGui::End();
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
