#include "ImGuiWrapper.h"

#if defined INNO_PLATFORM_WIN
#include "ImGuiWrapperWinDX.h"
#include "ImGuiWrapperWinGL.h"
#include "ImGuiWrapperWinVK.h"
#endif

#include "../ICoreSystem.h"

extern ICoreSystem* g_pCoreSystem;

INNO_PRIVATE_SCOPE ImGuiWrapperNS
{
	struct GameConfig
	{
		bool pauseGameUpdate = false;
	};

	void showApplicationProfiler();
	void zoom(bool zoom, ImTextureID textureID, ImVec2 renderTargetSize);

	void showFileExplorer();
	void showAssetConvertPopWindow(bool &l_popMenuOpened, std::string &l_fileFullPath);
	void showSceneLoadingPopWindow(bool &l_popMenuOpened, std::string &l_fileFullPath);

	void showWorldExplorer();
	void showTransformComponentPropertyEditor(void* rhs);
	void showVisiableComponentPropertyEditor(void* rhs);
	void showDirectionalLightComponentPropertyEditor(void* rhs);
	void showPointLightComponentPropertyEditor(void* rhs);
	void showSphereLightComponentPropertyEditor(void* rhs);

	bool m_isParity = true;

	IImGuiWrapperImpl* m_wrapperImpl;
}

bool ImGuiWrapper::setup()
{
	auto l_initConfig = g_pCoreSystem->getVisionSystem()->getInitConfig();

	switch (l_initConfig.renderingBackend)
	{
	case RenderingBackend::GL:
		ImGuiWrapperNS::m_wrapperImpl = new ImGuiWrapperWinGL();
		break;
	case RenderingBackend::DX:
		ImGuiWrapperNS::m_wrapperImpl = new ImGuiWrapperWinDX();
		break;
	case RenderingBackend::VK:
		ImGuiWrapperNS::m_isParity = false;
		//ImGuiWrapperNS::m_wrapperImpl = new ImGuiWrapperWinVK();
		break;
	default:
		break;
	}

	if (ImGuiWrapperNS::m_isParity)
	{
		// Setup Dear ImGui binding
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();

		ImGuiWrapperNS::m_wrapperImpl->setup();
	}

	return true;
}

bool ImGuiWrapper::initialize()
{
	if (ImGuiWrapperNS::m_isParity)
	{
		ImGuiWrapperNS::m_wrapperImpl->initialize();

		ImGuiIO& io = ImGui::GetIO(); (void)io;
		//io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls
		//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;   // Enable Gamepad Controls

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
		io.Fonts->AddFontFromFileTTF("..//res//fonts//FreeSans.otf", 16.0f);
	}

	return true;
}

bool ImGuiWrapper::update()
{
	if (ImGuiWrapperNS::m_isParity)
	{
		ImGuiWrapperNS::m_wrapperImpl->newFrame();

#ifdef DEBUG
		ImGui::NewFrame();
		{
			ImGuiWrapperNS::showApplicationProfiler();
			//ImGuiWrapperNS::showFileExplorer();
			//ImGuiWrapperNS::showWorldExplorer();
#else
		// @TODO: handle GUI component
		ImGui::Begin("Main Menu", 0, ImGuiWindowFlags_AlwaysAutoResize);
		ImGui::Button("Start");
		ImGui::End();
#endif
		}
	ImGui::Render();
	ImGuiWrapperNS::m_wrapperImpl->render();
	}
return true;
}

bool ImGuiWrapper::terminate()
{
	if (ImGuiWrapperNS::m_isParity)
	{
		ImGuiWrapperNS::m_wrapperImpl->terminate();
		ImGui::DestroyContext();
	}
	return true;
}

void ImGuiWrapperNS::showApplicationProfiler()
{
	static RenderingConfig l_renderingConfig;
	static GameConfig l_gameConfig;
	static bool l_useZoom;
	static bool l_showRenderPassResult;

	ImGui::Begin("Profiler", 0, ImGuiWindowFlags_AlwaysAutoResize);
	ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
	ImGui::Checkbox("Use TAA", &l_renderingConfig.useTAA);
	ImGui::Checkbox("Use Bloom", &l_renderingConfig.useBloom);
	ImGui::Checkbox("Draw terrain", &l_renderingConfig.drawTerrain);
	ImGui::Checkbox("Draw sky", &l_renderingConfig.drawSky);
	ImGui::Checkbox("Draw debug object", &l_renderingConfig.drawDebugObject);
	ImGui::Checkbox("Pause game update", &l_gameConfig.pauseGameUpdate);
	ImGui::Checkbox("Use zoom", &l_useZoom);
	ImGui::Checkbox("Show render pass result", &l_showRenderPassResult);

	const char* items[] = { "OpaquePass", "TransparentPass", "TerrainPass", "LightPass", "FinalPass" };
	static int item_current = 0;
	ImGui::Combo("Choose shader", &item_current, items, IM_ARRAYSIZE(items));

	if (ImGui::Button("Reload Shader"))
	{
		g_pCoreSystem->getVisionSystem()->getRenderingBackend()->reloadShader(RenderPassType(item_current));
	}
	if (ImGui::Button("Bake GI"))
	{
		g_pCoreSystem->getVisionSystem()->getRenderingBackend()->bakeGI();
	}

	static char scene_filePath[128] = "..//res//scenes//Intro.InnoScene";
	ImGui::InputText("Scene file path", scene_filePath, IM_ARRAYSIZE(scene_filePath));

	if (ImGui::Button("Save scene"))
	{
		g_pCoreSystem->getFileSystem()->saveScene(scene_filePath);
	}
	if (ImGui::Button("Load scene"))
	{
		g_pCoreSystem->getFileSystem()->loadScene(scene_filePath);
	}

	if (l_showRenderPassResult)
	{
		ImGuiWrapperNS::m_wrapperImpl->showRenderResult();
	}

	ImGui::End();

	g_pCoreSystem->getVisionSystem()->getRenderingFrontend()->setRenderingConfig(l_renderingConfig);
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

void ImGuiWrapperNS::showFileExplorer()
{
	auto l_iconSize = ImVec2(48.0f, 48.0f);

	ImGui::Begin("File Explorer", 0);

	static DirectoryMetadata* currentDirectoryMetadata = g_pCoreSystem->getAssetSystem()->getRootDirectoryMetadata();

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
			ImGui::Image(ImGuiWrapperNS::m_wrapperImpl->getFileExplorerIconTextureID(FileExplorerIconType::UNKNOWN), l_iconSize, ImVec2(0.0, 1.0), ImVec2(1.0, 0.0));
			{
				if (ImGui::OpenPopupOnItemClick("Directory Left-click", 0))
				{
					currentDirectoryMetadata = l_currentActivateDir;
				}
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

	static bool l_AssetConvertPopMenuOpened = false;
	static bool l_SceneLoadingPopMenuOpened = false;
	static std::string l_fileFullPath;
	for (auto& i : currentDirectoryMetadata->childrenAssets)
	{
		ImGui::BeginGroup();
		{
			ImGui::Image(ImGuiWrapperNS::m_wrapperImpl->getFileExplorerIconTextureID(i.iconType), l_iconSize, ImVec2(0.0, 1.0), ImVec2(1.0, 0.0));
			{
				if (ImGui::OpenPopupOnItemClick("Asset Right-click", 1))
				{
					if (i.extension == ".obj")
					{
						l_fileFullPath = i.fullPath;
						l_AssetConvertPopMenuOpened = true;
					}
					else if (i.extension == ".InnoScene")
					{
						l_fileFullPath = i.fullPath;
						l_SceneLoadingPopMenuOpened = true;
					}
				}
			}
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

	if (l_AssetConvertPopMenuOpened)
	{
		showAssetConvertPopWindow(l_AssetConvertPopMenuOpened, l_fileFullPath);
	}
	if (l_SceneLoadingPopMenuOpened)
	{
		showSceneLoadingPopWindow(l_SceneLoadingPopMenuOpened, l_fileFullPath);
	}

	ImGui::End();
}

void ImGuiWrapperNS::showAssetConvertPopWindow(bool &l_popMenuOpened, std::string &l_fileFullPath)
{
	ImGui::OpenPopup("Convert Asset");

	if (ImGui::BeginPopupModal("Convert Asset", NULL, ImGuiWindowFlags_AlwaysAutoResize))
	{
		static char exportAsset_filePath[128] = "..//res//convertedAssets//";
		ImGui::InputText("Export asset file path", exportAsset_filePath, IM_ARRAYSIZE(exportAsset_filePath));

		ImGui::Separator();

		if (ImGui::Button("Convert", ImVec2(120, 0))) {
			l_popMenuOpened = false;
			g_pCoreSystem->getFileSystem()->convertModel(l_fileFullPath, exportAsset_filePath);
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine();
		if (ImGui::Button("Cancel", ImVec2(120, 0))) {
			l_popMenuOpened = false;
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}
}

void ImGuiWrapperNS::showSceneLoadingPopWindow(bool &l_popMenuOpened, std::string &l_fileFullPath)
{
	ImGui::OpenPopup("Load scene");

	if (ImGui::BeginPopupModal("Load scene", NULL, ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::Separator();

		if (ImGui::Button("Load", ImVec2(120, 0))) {
			l_popMenuOpened = false;
			g_pCoreSystem->getFileSystem()->loadScene(l_fileFullPath);
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine();
		if (ImGui::Button("Cancel", ImVec2(120, 0))) {
			l_popMenuOpened = false;
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}
}

void ImGuiWrapperNS::showWorldExplorer()
{
	ImGui::Begin("World Explorer", 0);

	static void* selectedComponent = nullptr;
	static componentType selectedComponentType;

	auto l_entityNameMap = g_pCoreSystem->getGameSystem()->getEntityNameMap();
	auto l_entityChildrenComponentsMetadataMap = g_pCoreSystem->getGameSystem()->getEntityChildrenComponentsMetadataMap();

	for (auto i : l_entityNameMap)
	{
		if (ImGui::TreeNode(i.second.c_str()))
		{
			auto result = l_entityChildrenComponentsMetadataMap.find(i.first);
			if (result != l_entityChildrenComponentsMetadataMap.end())
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
			case componentType::DirectionalLightComponent: showDirectionalLightComponentPropertyEditor(selectedComponent); break;
			case componentType::PointLightComponent: showPointLightComponentPropertyEditor(selectedComponent); break;
			case componentType::SphereLightComponent: showSphereLightComponentPropertyEditor(selectedComponent); break;
			default:
				break;
			}
		}
	}
	ImGui::End();
}

void ImGuiWrapperNS::showTransformComponentPropertyEditor(void * rhs)
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

	static float rot_min = -180.0f;
	static float rot_max = 180.0f;

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

void ImGuiWrapperNS::showVisiableComponentPropertyEditor(void * rhs)
{
	auto l_rhs = reinterpret_cast<VisibleComponent*>(rhs);

	const char* meshPrimitiveTopology_items[] = { "Point", "Line", "Triangle", "Triangle-strip" };
	static int meshPrimitiveTopology_item_current = (int)l_rhs->m_meshPrimitiveTopology;
	if (ImGui::Combo("Mesh primitive topology", &meshPrimitiveTopology_item_current, meshPrimitiveTopology_items, IM_ARRAYSIZE(meshPrimitiveTopology_items)))
	{
		l_rhs->m_meshPrimitiveTopology = MeshPrimitiveTopology(meshPrimitiveTopology_item_current);
	}

	static char modelFileName[128];
	ImGui::InputText("Model file name", modelFileName, IM_ARRAYSIZE(modelFileName));

	if (ImGui::Button("Save"))
	{
		l_rhs->m_modelFileName = modelFileName;
	}

	static MaterialDataComponent* selectedComponent = nullptr;

	{
		ImGui::BeginChild("Children MaterialDataComponents", ImVec2(ImGui::GetWindowContentRegionWidth() * 0.3f, 400.0f), true, ImGuiWindowFlags_HorizontalScrollbar);
		{
			for (auto& i : l_rhs->m_modelMap)
			{
				if (ImGui::Selectable(i.first->m_parentEntity.c_str(), selectedComponent == i.second))
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
			ImGui::BeginChild("MaterialDataComponent Editor", ImVec2(ImGui::GetWindowContentRegionWidth() * 0.7f, 400.0f), true, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar);
			{
				auto l_material = &selectedComponent->m_meshCustomMaterial;

				static float float_min = 0.0f;
				static float float_max = 1.0f;

				static ImVec4 albedo = ImColor(l_material->albedo_r, l_material->albedo_g, l_material->albedo_b, l_material->alpha);

				if (ImGui::ColorPicker4("Albedo color", (float*)&albedo, ImGuiColorEditFlags_RGB))
				{
					l_material->albedo_r = albedo.x;
					l_material->albedo_g = albedo.y;
					l_material->albedo_b = albedo.z;
					l_material->alpha = albedo.w;
				}

				const ImVec2 small_slider_size(18, 180);

				auto tt = ImGui::GetCursorPos().x;
				static float metallic = l_material->metallic;
				if (ImGui::DragFloat("Metallic", &metallic, 0.01f, float_min, float_max))
				{
					l_material->metallic = metallic;
				}

				static float roughness = l_material->roughness;
				if (ImGui::DragFloat("Roughness", &roughness, 0.01f, float_min, float_max))
				{
					l_material->roughness = roughness;
				}

				static float ao = l_material->ao;
				if (ImGui::DragFloat("Ambient Occlusion", &ao, 0.01f, float_min, float_max))
				{
					l_material->ao = ao;
				}

				if (l_rhs->m_visiblilityType == VisiblilityType::INNO_TRANSPARENT)
				{
					static float thickness = l_material->thickness;
					if (ImGui::DragFloat("Thickness", &thickness, 0.01f, float_min, float_max))
					{
						l_material->thickness = thickness;
					}
				}
			}
			ImGui::EndChild();
		}
	}
}

void ImGuiWrapperNS::showDirectionalLightComponentPropertyEditor(void * rhs)
{
	auto l_rhs = reinterpret_cast<DirectionalLightComponent*>(rhs);
	ImGui::BeginChild("DirectionalLightComponent Editor", ImVec2(ImGui::GetWindowContentRegionWidth(), 400.0f), true, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar);
	{
		static ImVec4 radiance = ImColor(l_rhs->m_color.x, l_rhs->m_color.y, l_rhs->m_color.z, l_rhs->m_color.w);

		if (ImGui::ColorPicker4("Radiance color", (float*)&radiance, ImGuiColorEditFlags_RGB))
		{
			l_rhs->m_color.x = radiance.x;
			l_rhs->m_color.y = radiance.y;
			l_rhs->m_color.z = radiance.z;
			l_rhs->m_color.w = radiance.w;
		}
		static float luminousFlux = l_rhs->m_luminousFlux;
		if (ImGui::DragFloat("Luminous flux", &luminousFlux, 0.01f, 0.0f, 100000.0f))
		{
			l_rhs->m_luminousFlux = luminousFlux;
		}
	}
	ImGui::EndChild();
}

void ImGuiWrapperNS::showPointLightComponentPropertyEditor(void * rhs)
{
	auto l_rhs = reinterpret_cast<PointLightComponent*>(rhs);
	ImGui::BeginChild("DirectionalLightComponent Editor", ImVec2(ImGui::GetWindowContentRegionWidth(), 400.0f), true, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar);
	{
		static ImVec4 radiance = ImColor(l_rhs->m_color.x, l_rhs->m_color.y, l_rhs->m_color.z, l_rhs->m_color.w);

		if (ImGui::ColorPicker4("Radiance color", (float*)&radiance, ImGuiColorEditFlags_RGB))
		{
			l_rhs->m_color.x = radiance.x;
			l_rhs->m_color.y = radiance.y;
			l_rhs->m_color.z = radiance.z;
			l_rhs->m_color.w = radiance.w;
		}
		static float luminousFlux = l_rhs->m_luminousFlux;
		if (ImGui::DragFloat("Luminous flux", &luminousFlux, 0.01f, 0.0f, 100000.0f))
		{
			l_rhs->m_luminousFlux = luminousFlux;
		}
	}
	ImGui::EndChild();
}

void ImGuiWrapperNS::showSphereLightComponentPropertyEditor(void * rhs)
{
	auto l_rhs = reinterpret_cast<SphereLightComponent*>(rhs);
	ImGui::BeginChild("DirectionalLightComponent Editor", ImVec2(ImGui::GetWindowContentRegionWidth(), 400.0f), true, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar);
	{
		static ImVec4 radiance = ImColor(l_rhs->m_color.x, l_rhs->m_color.y, l_rhs->m_color.z, l_rhs->m_color.w);

		if (ImGui::ColorPicker4("Radiance color", (float*)&radiance, ImGuiColorEditFlags_RGB))
		{
			l_rhs->m_color.x = radiance.x;
			l_rhs->m_color.y = radiance.y;
			l_rhs->m_color.z = radiance.z;
			l_rhs->m_color.w = radiance.w;
		}
		static float luminousFlux = l_rhs->m_luminousFlux;
		if (ImGui::DragFloat("Luminous flux", &luminousFlux, 0.01f, 0.0f, 100000.0f))
		{
			l_rhs->m_luminousFlux = luminousFlux;
		}
		static float sphereRadius = l_rhs->m_sphereRadius;
		if (ImGui::DragFloat("Sphere radius", &sphereRadius, 0.01f, 0.1f, 10000.0f))
		{
			l_rhs->m_sphereRadius = sphereRadius;
		}
	}
	ImGui::EndChild();
}