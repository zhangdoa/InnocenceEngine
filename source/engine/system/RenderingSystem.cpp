#include "RenderingSystem.h"

void RenderingSystem::setup()
{
	setupWindow();
	setupInput();
	setupRendering();
	setupGui();

	m_objectStatus = objectStatus::ALIVE;
}

void RenderingSystem::setupWindow()
{
	//setup window
	if (glfwInit() != GL_TRUE)
	{
		m_objectStatus = objectStatus::STANDBY;
		g_pLogSystem->printLog("Failed to initialize GLFW.");
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3); // We want OpenGL 3.3
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
#ifdef INNO_PLATFORM_MACOS
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // To make MacOS happy; should not be needed
#endif
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); //We don't want the old OpenGL

																   // Open a window and create its OpenGL context
	m_window = glfwCreateWindow((int)m_screenResolution.x, (int)m_screenResolution.y, m_windowName.c_str(), NULL, NULL);
	glfwMakeContextCurrent(m_window);
	if (m_window == nullptr) {
		m_objectStatus = objectStatus::STANDBY;
		g_pLogSystem->printLog("Failed to open GLFW window. If you have an Intel GPU, they are not 3.3 compatible. Try the 2.1 version of the tutorials.");
		glfwTerminate();
	}
	// glad: load all OpenGL function pointers
	// ---------------------------------------
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		m_objectStatus = objectStatus::STANDBY;
		g_pLogSystem->printLog("Failed to initialize GLAD.");
	}
}

void RenderingSystem::setupInput()
{
	//setup input
	glfwSetInputMode(m_window, GLFW_STICKY_KEYS, GL_TRUE);

	for (int i = 0; i < NUM_KEYCODES; i++)
	{
		m_keyButtonMap.emplace(i, keyButton());
	}

	f_changeDrawPolygonMode = std::bind(&RenderingSystem::changeDrawPolygonMode, this);
	f_changeDrawTextureMode = std::bind(&RenderingSystem::changeDrawTextureMode, this);
	f_changeShadingMode = std::bind(&RenderingSystem::changeShadingMode, this);
}

void RenderingSystem::setupRendering()
{
	//setup rendering
	m_GLRenderingSystem = g_pMemorySystem->spawn<GLRenderingSystem>();
	m_GLRenderingSystem->setup();

	m_canRender = true;
}

inline void RenderingSystem::setupGui()
{
	// Setup Dear ImGui binding
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;   // Enable Gamepad Controls
	ImGui_ImplGlfwGL3_Init(m_window, true);

	// Setup style
	ImGui::StyleColorsDark();
	//ImGui::StyleColorsClassic();

	// Load Fonts
	io.Fonts->AddFontFromFileTTF("../res/fonts/FreeSans.otf", 16.0f);
}

void RenderingSystem::initialize()
{
	initializeWindow();
	initializeInput();
	initializeRendering();
	initializeGui();

	g_pLogSystem->printLog("RenderingSystem has been initialized.");
}

void RenderingSystem::initializeWindow()
{
	//initialize window
	windowCallbackWrapper::getInstance().setRenderingSystem(this);
	windowCallbackWrapper::getInstance().initialize();
}

void RenderingSystem::initializeInput()
{
	//initialize input
	for (size_t i = 0; i < g_pGameSystem->getInputComponents().size(); i++)
	{
		// @TODO: multi input components need to register to multi map
		addKeyboardInputCallback(g_pGameSystem->getInputComponents()[i]->getKeyboardInputCallbackContainer());
		addMouseMovementCallback(g_pGameSystem->getInputComponents()[i]->getMouseInputCallbackContainer());
	}

	// @TODO: debt I owe
	addKeyboardInputCallback(INNO_KEY_Q, &f_changeDrawPolygonMode);
	m_keyButtonMap.find(INNO_KEY_Q)->second.m_keyPressType = keyPressType::ONCE;
	addKeyboardInputCallback(GLFW_KEY_E, &f_changeDrawTextureMode);
	m_keyButtonMap.find(INNO_KEY_E)->second.m_keyPressType = keyPressType::ONCE;
	addKeyboardInputCallback(INNO_KEY_R, &f_changeShadingMode);
	m_keyButtonMap.find(INNO_KEY_R)->second.m_keyPressType = keyPressType::ONCE;
}

void RenderingSystem::initializeRendering()
{
	setupComponents();
	//initialize rendering
	m_GLRenderingSystem->initialize();
}

void RenderingSystem::initializeGui()
{
}

void RenderingSystem::setupComponents()
{
	setupCameraComponents();
	setupVisibleComponents();
	setupLightComponents();
}

void RenderingSystem::setupCameraComponents()
{
	for (auto& i : g_pGameSystem->getCameraComponents())
	{
		setupCameraComponentProjectionMatrix(i);
		setupCameraComponentRayOfEye(i);
		setupCameraComponentFrustumVertices(i);
		generateAABB(*i);
	}
}

void RenderingSystem::setupCameraComponentProjectionMatrix(CameraComponent* cameraComponent)
{
	cameraComponent->m_projectionMatrix.initializeToPerspectiveMatrix((cameraComponent->m_FOV / 180.0) * PI, cameraComponent->m_WHRatio, cameraComponent->m_zNear, cameraComponent->m_zFar);
}

void RenderingSystem::setupCameraComponentRayOfEye(CameraComponent * cameraComponent)
{
	cameraComponent->m_rayOfEye.m_origin = g_pGameSystem->getTransformComponent(cameraComponent->getParentEntity())->m_transform.caclGlobalPos();
	cameraComponent->m_rayOfEye.m_direction = g_pGameSystem->getTransformComponent(cameraComponent->getParentEntity())->m_transform.getDirection(direction::BACKWARD);
}

void RenderingSystem::setupCameraComponentFrustumVertices(CameraComponent * cameraComponent)
{
	auto l_NDC = generateNDC();
	auto l_pCamera = cameraComponent->m_projectionMatrix;
	auto l_rCamera = g_pGameSystem->getTransformComponent(cameraComponent->getParentEntity())->m_transform.caclGlobalRot().toRotationMatrix();
	auto l_tCamera = g_pGameSystem->getTransformComponent(cameraComponent->getParentEntity())->m_transform.caclGlobalPos().toTranslationMatrix();

	for (auto& l_vertexData : l_NDC)
	{
		vec4 l_mulPos;
		l_mulPos = l_vertexData.m_pos;
		// from projection space to view space
		//Column-Major memory layout
#ifdef USE_COLUMN_MAJOR_MEMORY_LAYOUT
		l_mulPos = l_mulPos * l_pCamera.inverse();
#endif
		//Row-Major memory layout
#ifdef USE_ROW_MAJOR_MEMORY_LAYOUT
		l_mulPos = l_pCamera.inverse() * l_mulPos;
#endif
		// perspective division
		l_mulPos = l_mulPos * (1.0 / l_mulPos.w);
		// from view space to world space
		//Column-Major memory layout
#ifdef USE_COLUMN_MAJOR_MEMORY_LAYOUT
		l_mulPos = l_mulPos * l_rCamera;
		l_mulPos = l_mulPos * l_tCamera;
#endif
		//Row-Major memory layout
#ifdef USE_ROW_MAJOR_MEMORY_LAYOUT
		l_mulPos = l_rCamera * l_mulPos;
		l_mulPos = l_tCamera * l_mulPos;
#endif
		l_vertexData.m_pos = l_mulPos;
	}

	for (auto& l_vertexData : l_NDC)
	{
		l_vertexData.m_normal = vec4(l_vertexData.m_pos.x, l_vertexData.m_pos.y, l_vertexData.m_pos.z, 0.0).normalize();
	}

	cameraComponent->m_frustumVertices = l_NDC;

	cameraComponent->m_frustumIndices = { 0, 1, 3, 1, 2, 3,
		4, 5, 0, 5, 1, 0,
		7, 6, 4, 6, 5, 4,
		3, 2, 7, 2, 6 ,7,
		4, 0, 7, 0, 3, 7,
		1, 5, 2, 5, 6, 2 };
}

void RenderingSystem::setupVisibleComponents()
{
	for (auto i : g_pGameSystem->getVisibleComponents())
	{
		if (i->m_visiblilityType != visiblilityType::INVISIBLE)
		{
			if (i->m_visiblilityType == visiblilityType::EMISSIVE)
			{
				GLRenderingSystemSingletonComponent::getInstance().m_emissiveVisibleComponents.emplace_back(i);
			}
			else if (i->m_visiblilityType == visiblilityType::STATIC_MESH)
			{
				GLRenderingSystemSingletonComponent::getInstance().m_staticMeshVisibleComponents.emplace_back(i);
			}
			generateAABB(*i);
		}
	}
}

void RenderingSystem::setupLightComponents()
{
	for (auto& i : g_pGameSystem->getLightComponents())
	{
		i->m_direction = vec4(0.0, 0.0, 1.0, 0.0);
		i->m_constantFactor = 1.0;
		i->m_linearFactor = 0.14;
		i->m_quadraticFactor = 0.07;
		setupLightComponentRadius(i);
		i->m_color = vec4(1.0, 1.0, 1.0, 1.0);

		if (i->m_lightType == lightType::DIRECTIONAL)
		{
			generateAABB(*i);
		}
	}
}

void RenderingSystem::setupLightComponentRadius(LightComponent * lightComponent)
{
	double l_lightMaxIntensity = std::fmax(std::fmax(lightComponent->m_color.x, lightComponent->m_color.y), lightComponent->m_color.z);
	lightComponent->m_radius = -lightComponent->m_linearFactor + std::sqrt(lightComponent->m_linearFactor * lightComponent->m_linearFactor - 4.0 * lightComponent->m_quadraticFactor * (lightComponent->m_constantFactor - (256.0 / 5.0) * l_lightMaxIntensity)) / (2.0 * lightComponent->m_quadraticFactor);
}

vec4 RenderingSystem::calcMousePositionInWorldSpace()
{
	auto l_x = 2.0 * m_mouseLastX / m_screenResolution.x - 1.0;
	auto l_y = 1.0 - 2.0 * m_mouseLastY / m_screenResolution.y;
	auto l_z = -1.0;
	auto l_w = 1.0;
	vec4 l_ndcSpace = vec4(l_x, l_y, l_z, l_w);

	auto pCamera = g_pGameSystem->getCameraComponents()[0]->m_projectionMatrix;
	auto rCamera = g_pGameSystem->getTransformComponent(g_pGameSystem->getCameraComponents()[0]->getParentEntity())->m_transform.getInvertGlobalRotMatrix();
	auto tCamera = g_pGameSystem->getTransformComponent(g_pGameSystem->getCameraComponents()[0]->getParentEntity())->m_transform.getInvertGlobalTranslationMatrix();
	//Column-Major memory layout
#ifdef USE_COLUMN_MAJOR_MEMORY_LAYOUT
	l_ndcSpace = l_ndcSpace * pCamera.inverse();
	l_ndcSpace.z = -1.0;
	l_ndcSpace.w = 0.0;
	l_ndcSpace = l_ndcSpace * rCamera.inverse();
	l_ndcSpace = l_ndcSpace * tCamera.inverse();
#endif
	//Row-Major memory layout
#ifdef USE_ROW_MAJOR_MEMORY_LAYOUT

	l_ndcSpace = pCamera.inverse() * l_ndcSpace;
	l_ndcSpace.z = -1.0;
	l_ndcSpace.w = 0.0;
	l_ndcSpace = tCamera.inverse() * l_ndcSpace;
	l_ndcSpace = rCamera.inverse() * l_ndcSpace;
#endif
	l_ndcSpace = l_ndcSpace.normalize();
	return l_ndcSpace;
}

void RenderingSystem::updateInput()
{
	for (int i = 0; i < NUM_KEYCODES; i++)
	{
		//if key pressed
		if (glfwGetKey(m_window, i) == GLFW_PRESS)
		{
			auto l_keyButton = m_keyButtonMap.find(i);
			if (l_keyButton != m_keyButtonMap.end())
			{
				//check whether it's still pressed/ the bound functions has been invoked
				if (l_keyButton->second.m_allowCallback)
				{
					auto l_keybinding = m_keyboardInputCallback.find(i);
					if (l_keybinding != m_keyboardInputCallback.end())
					{
						for (auto j : l_keybinding->second)
						{
							if (j)
							{
								(*j)();
							}
						}
					}
					if (l_keyButton->second.m_keyPressType == keyPressType::ONCE)
					{
						l_keyButton->second.m_allowCallback = false;
					}
				}

			}
		}
		else
		{
			auto l_keyButton = m_keyButtonMap.find(i);
			if (l_keyButton != m_keyButtonMap.end())
			{
				if (l_keyButton->second.m_keyPressType == keyPressType::ONCE)
				{
					l_keyButton->second.m_allowCallback = true;
				}
			}
		}
	}
	if (glfwGetMouseButton(m_window, 1) == GLFW_PRESS)
	{
		hideMouseCursor();
		if (m_mouseMovementCallback.size() != 0)
		{
			if (m_mouseXOffset != 0)
			{
				for (auto j : m_mouseMovementCallback.find(0)->second)
				{
					(*j)(m_mouseXOffset);
				};
			}
			if (m_mouseYOffset != 0)
			{
				for (auto j : m_mouseMovementCallback.find(1)->second)
				{
					(*j)(m_mouseYOffset);
				};
			}
			if (m_mouseXOffset != 0 || m_mouseYOffset != 0)
			{
				m_mouseXOffset = 0;
				m_mouseYOffset = 0;
			}
		}
	}
	else
	{
		showMouseCursor();
	}
}

void RenderingSystem::updateCameraComponents()
{
	if (g_pGameSystem->getCameraComponents().size() > 0)
	{
		for (auto& i : g_pGameSystem->getCameraComponents())
		{
			setupCameraComponentRayOfEye(i);
			setupCameraComponentFrustumVertices(i);
		}

		generateAABB(*g_pGameSystem->getCameraComponents()[0]);

		m_mouseRay.m_origin = g_pGameSystem->getTransformComponent(g_pGameSystem->getCameraComponents()[0]->getParentEntity())->m_transform.caclGlobalPos();
		m_mouseRay.m_direction = calcMousePositionInWorldSpace();

		auto l_cameraAABB = g_pGameSystem->getCameraComponents()[0]->m_AABB;

		auto l_ray = g_pGameSystem->getCameraComponents()[0]->m_rayOfEye;

		for (auto& j : g_pGameSystem->getVisibleComponents())
		{
			if (j->m_visiblilityType == visiblilityType::STATIC_MESH)
			{
				if (j->m_AABB.intersectCheck(m_mouseRay))
				{
					GLRenderingSystemSingletonComponent::getInstance().m_selectedVisibleComponents.emplace_back(j);
				}
				if (l_cameraAABB.intersectCheck(j->m_AABB))
				{
					GLRenderingSystemSingletonComponent::getInstance().m_inFrustumVisibleComponents.emplace_back(j);
				}
			}
		}
	}
}

void RenderingSystem::updateLightComponents()
{
	if (g_pGameSystem->getLightComponents().size() > 0)
	{
		// generate AABB for CSM
		for (auto& i : g_pGameSystem->getLightComponents())
		{
			setupLightComponentRadius(i);
			if (i->m_lightType == lightType::DIRECTIONAL)
			{
				generateAABB(*i);
			}
		}
	}
}

void RenderingSystem::updatePhysics()
{
	GLRenderingSystemSingletonComponent::getInstance().m_selectedVisibleComponents.clear();
	GLRenderingSystemSingletonComponent::getInstance().m_inFrustumVisibleComponents.clear();

	// camera update
	updateCameraComponents();

	updateLightComponents();
}

void RenderingSystem::updateGui()
{
	#ifndef INNO_PLATFORM_LINUX64
	const char* items[] = { "Final Pass", "Light Pass", "Geometry Pass", "Shadow Pass" };
	static const char* item_current = items[0];

	ImGui_ImplGlfwGL3_NewFrame();
	{
		ImGui::Begin("Global Settings", false, ImGuiWindowFlags_AlwaysAutoResize);

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

	auto l_renderTargetSize = ImVec2(GLRenderingSystemSingletonComponent::getInstance().m_renderTargetSize.x, GLRenderingSystemSingletonComponent::getInstance().m_renderTargetSize.y);
	{
		if (item_current == items[0])
		{
			ImGui::Begin("Final Pass", false, ImGuiWindowFlags_AlwaysAutoResize);
			ImGui::Image(ImTextureID(FinalRenderPassSingletonComponent::getInstance().m_finalBlendPassTexture.m_TAO), l_renderTargetSize, ImVec2(1.0, 1.0), ImVec2(0.0, 0.0));
			ImGui::End();
		}
		else if (item_current == items[1])
		{
			ImGui::Begin("Light Pass", false, ImGuiWindowFlags_AlwaysAutoResize);
			ImGui::Image(ImTextureID(LightRenderPassSingletonComponent::getInstance().m_lightPassTexture.m_TAO), l_renderTargetSize, ImVec2(1.0, 1.0), ImVec2(0.0, 0.0));
			ImGui::End();
		}
		else if (item_current == items[2])
		{
			ImGui::Begin("Geometry Pass", false, ImGuiWindowFlags_AlwaysAutoResize);
			ImGui::BeginChild("World Space Position(RGB) + Metallic(A)", l_renderTargetSize, true, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar);
			ImGui::Image(ImTextureID(GeometryRenderPassSingletonComponent::getInstance().m_geometryPassTextures[0].m_TAO), l_renderTargetSize, ImVec2(1.0, 1.0), ImVec2(0.0, 0.0));
			ImGui::EndChild();
			ImGui::BeginChild("World Space Normal(RGB) + Roughness(A)", l_renderTargetSize, true, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar);
			ImGui::Image(ImTextureID(GeometryRenderPassSingletonComponent::getInstance().m_geometryPassTextures[1].m_TAO), l_renderTargetSize, ImVec2(1.0, 1.0), ImVec2(0.0, 0.0));
			ImGui::EndChild();
			ImGui::BeginChild("Albedo(RGB) + Ambient Occlusion(A)", l_renderTargetSize, true, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar);
			ImGui::Image(ImTextureID(GeometryRenderPassSingletonComponent::getInstance().m_geometryPassTextures[2].m_TAO), l_renderTargetSize, ImVec2(1.0, 1.0), ImVec2(0.0, 0.0));
			ImGui::EndChild();
			ImGui::BeginChild("Screen Space Motion Vector", l_renderTargetSize, true, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar);
			ImGui::Image(ImTextureID(GeometryRenderPassSingletonComponent::getInstance().m_geometryPassTextures[3].m_TAO), l_renderTargetSize, ImVec2(1.0, 1.0), ImVec2(0.0, 0.0));
			ImGui::EndChild();
			ImGui::BeginChild("Light Space Position 0", l_renderTargetSize, true, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar);
			ImGui::Image(ImTextureID(GeometryRenderPassSingletonComponent::getInstance().m_geometryPassTextures[4].m_TAO), l_renderTargetSize, ImVec2(1.0, 1.0), ImVec2(0.0, 0.0));
			ImGui::EndChild();
			ImGui::BeginChild("Light Space Position 1", l_renderTargetSize, true, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar);
			ImGui::Image(ImTextureID(GeometryRenderPassSingletonComponent::getInstance().m_geometryPassTextures[5].m_TAO), l_renderTargetSize, ImVec2(1.0, 1.0), ImVec2(0.0, 0.0));
			ImGui::EndChild();
			ImGui::BeginChild("Light Space Position 2", l_renderTargetSize, true, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar);
			ImGui::Image(ImTextureID(GeometryRenderPassSingletonComponent::getInstance().m_geometryPassTextures[6].m_TAO), l_renderTargetSize, ImVec2(1.0, 1.0), ImVec2(0.0, 0.0));
			ImGui::EndChild();
			ImGui::BeginChild("Light Space Position 3", l_renderTargetSize, true, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar);
			ImGui::Image(ImTextureID(GeometryRenderPassSingletonComponent::getInstance().m_geometryPassTextures[7].m_TAO), l_renderTargetSize, ImVec2(1.0, 1.0), ImVec2(0.0, 0.0));
			ImGui::EndChild();
			ImGui::End();
		}
		else if (item_current == items[3])
		{
			auto l_shadowPassWindowSize = ImVec2(512.0, 512.0);
			ImGui::Begin("Shadow Pass", false, ImGuiWindowFlags_AlwaysAutoResize);
			ImGui::BeginChild("Shadow Pass Depth Buffer 0", l_shadowPassWindowSize, true, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar);
			ImGui::Image(ImTextureID(ShadowRenderPassSingletonComponent::getInstance().m_frameBufferTextureVector[0].second.m_TAO), l_shadowPassWindowSize, ImVec2(1.0, 1.0), ImVec2(0.0, 0.0));
			ImGui::EndChild();
			ImGui::BeginChild("Shadow Pass Depth Buffer 1", l_shadowPassWindowSize, true, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar);
			ImGui::Image(ImTextureID(ShadowRenderPassSingletonComponent::getInstance().m_frameBufferTextureVector[1].second.m_TAO), l_shadowPassWindowSize, ImVec2(1.0, 1.0), ImVec2(0.0, 0.0));
			ImGui::EndChild();
			ImGui::BeginChild("Shadow Pass Depth Buffer 2", l_shadowPassWindowSize, true, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar);
			ImGui::Image(ImTextureID(ShadowRenderPassSingletonComponent::getInstance().m_frameBufferTextureVector[2].second.m_TAO), l_shadowPassWindowSize, ImVec2(1.0, 1.0), ImVec2(0.0, 0.0));
			ImGui::EndChild();
			ImGui::BeginChild("Shadow Pass Depth Buffer 3", l_shadowPassWindowSize, true, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar);
			ImGui::Image(ImTextureID(ShadowRenderPassSingletonComponent::getInstance().m_frameBufferTextureVector[3].second.m_TAO), l_shadowPassWindowSize, ImVec2(1.0, 1.0), ImVec2(0.0, 0.0));
			ImGui::EndChild();
			ImGui::End();
		}
	}

	// Rendering
	glViewport(0, 0, m_screenResolution.x, m_screenResolution.y);
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	ImGui::Render();
	ImGui_ImplGlfwGL3_RenderDrawData(ImGui::GetDrawData());
	#else
	//@TODO: Linux ImGui WIP
	#endif
}

void RenderingSystem::update()
{
	if (m_window != nullptr && glfwWindowShouldClose(m_window) == 0)
	{
		glfwPollEvents();

		// input update
		updateInput();
		// physics update
		updatePhysics();

		// defer render
		if (m_canRender)
		{
			m_canRender = false;
			m_GLRenderingSystem->update();
			updateGui();
			//swap framebuffers
			glfwSwapBuffers(m_window);
			m_canRender = true;
		}

		// update the transform data @TODO: ugly
		std::for_each(g_pGameSystem->getTransformComponents().begin(), g_pGameSystem->getTransformComponents().end(), [&](TransformComponent* val)
		{
			val->m_transform.update();
		});
	}
	else
	{
		g_pLogSystem->printLog("Input error or Window closed.");
		g_pLogSystem->printLog("RenderingSystem is stand-by.");
		m_objectStatus = objectStatus::STANDBY;
	}
}

void RenderingSystem::shutdown()
{
	if (m_window != nullptr)
	{
		m_GLRenderingSystem->shutdown();

		glfwSetInputMode(m_window, GLFW_STICKY_KEYS, GL_FALSE);
		glfwDestroyWindow(m_window);
		glfwTerminate();
		g_pLogSystem->printLog("Window closed.");
	}
	m_objectStatus = objectStatus::SHUTDOWN;
	g_pLogSystem->printLog("RenderingSystem has been shutdown.");
}

const objectStatus & RenderingSystem::getStatus() const
{
	return m_objectStatus;
}

void RenderingSystem::setWindowName(const std::string & windowName)
{
	m_windowName = windowName;
}

void RenderingSystem::hideMouseCursor() const
{
	glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}

void RenderingSystem::showMouseCursor() const
{
	glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
}

void RenderingSystem::addKeyboardInputCallback(int keyCode, std::function<void()>* keyboardInputCallback)
{
	auto l_keyboardInputCallbackFunctionVector = m_keyboardInputCallback.find(keyCode);
	if (l_keyboardInputCallbackFunctionVector != m_keyboardInputCallback.end())
	{
		l_keyboardInputCallbackFunctionVector->second.emplace_back(keyboardInputCallback);
	}
	else
	{
		m_keyboardInputCallback.emplace(keyCode, std::vector<std::function<void()>*>{keyboardInputCallback});
	}
}

void RenderingSystem::addKeyboardInputCallback(int keyCode, std::vector<std::function<void()>*>& keyboardInputCallback)
{
	for (auto i : keyboardInputCallback)
	{
		addKeyboardInputCallback(keyCode, i);
	}
}

void RenderingSystem::addKeyboardInputCallback(std::unordered_map<int, std::vector<std::function<void()>*>>& keyboardInputCallback)
{
	for (auto i : keyboardInputCallback)
	{
		addKeyboardInputCallback(i.first, i.second);
	}
}

void RenderingSystem::addMouseMovementCallback(int mouseCode, std::function<void(double)>* mouseMovementCallback)
{
	auto l_mouseMovementCallbackFunctionVector = m_mouseMovementCallback.find(mouseCode);
	if (l_mouseMovementCallbackFunctionVector != m_mouseMovementCallback.end())
	{
		l_mouseMovementCallbackFunctionVector->second.emplace_back(mouseMovementCallback);
	}
	else
	{
		m_mouseMovementCallback.emplace(mouseCode, std::vector<std::function<void(double)>*>{mouseMovementCallback});
	}
}

void RenderingSystem::addMouseMovementCallback(int mouseCode, std::vector<std::function<void(double)>*>& mouseMovementCallback)
{
	for (auto i : mouseMovementCallback)
	{
		addMouseMovementCallback(mouseCode, i);
	}
}

void RenderingSystem::addMouseMovementCallback(std::unordered_map<int, std::vector<std::function<void(double)>*>>& mouseMovementCallback)
{
	for (auto i : mouseMovementCallback)
	{
		addMouseMovementCallback(i.first, i.second);
	}
}

void windowCallbackWrapper::setRenderingSystem(RenderingSystem * RenderingSystem)
{
	m_renderingSystem = RenderingSystem;
}

void windowCallbackWrapper::initialize()
{
	glfwSetFramebufferSizeCallback(m_renderingSystem->m_window, &framebufferSizeCallback);
	glfwSetCursorPosCallback(m_renderingSystem->m_window, &mousePositionCallback);
	glfwSetScrollCallback(m_renderingSystem->m_window, &scrollCallback);
}

void windowCallbackWrapper::framebufferSizeCallback(GLFWwindow * window, int width, int height)
{
	getInstance().framebufferSizeCallbackImpl(window, width, height);
}

void windowCallbackWrapper::mousePositionCallback(GLFWwindow * window, double mouseXPos, double mouseYPos)
{
	getInstance().mousePositionCallbackImpl(window, mouseXPos, mouseYPos);
}

void windowCallbackWrapper::scrollCallback(GLFWwindow * window, double xoffset, double yoffset)
{
	getInstance().scrollCallbackImpl(window, xoffset, yoffset);
}

void windowCallbackWrapper::framebufferSizeCallbackImpl(GLFWwindow * window, int width, int height)
{
	m_renderingSystem->m_screenResolution.x = width;
	m_renderingSystem->m_screenResolution.y = height;
	glViewport(0, 0, width, height);
}

void windowCallbackWrapper::mousePositionCallbackImpl(GLFWwindow * window, double mouseXPos, double mouseYPos)
{
	m_renderingSystem->m_mouseXOffset = mouseXPos - m_renderingSystem->m_mouseLastX;
	m_renderingSystem->m_mouseYOffset = m_renderingSystem->m_mouseLastY - mouseYPos;

	m_renderingSystem->m_mouseLastX = mouseXPos;
	m_renderingSystem->m_mouseLastY = mouseYPos;
}

void windowCallbackWrapper::scrollCallbackImpl(GLFWwindow * window, double xoffset, double yoffset)
{
	//@TODO: context based binding
	if (yoffset >= 0.0)
	{
		g_pGameSystem->getCameraComponents()[0]->m_FOV += 1.0;
	}
	else
	{
		g_pGameSystem->getCameraComponents()[0]->m_FOV -= 1.0;
	}
	m_renderingSystem->setupCameraComponentProjectionMatrix(g_pGameSystem->getCameraComponents()[0]);
}

std::vector<Vertex> RenderingSystem::generateNDC()
{
	Vertex l_VertexData_1;
	l_VertexData_1.m_pos = vec4(1.0, 1.0, 1.0, 1.0);
	l_VertexData_1.m_texCoord = vec2(1.0, 1.0);

	Vertex l_VertexData_2;
	l_VertexData_2.m_pos = vec4(1.0, -1.0, 1.0, 1.0);
	l_VertexData_2.m_texCoord = vec2(1.0, 0.0);

	Vertex l_VertexData_3;
	l_VertexData_3.m_pos = vec4(-1.0, -1.0, 1.0, 1.0);
	l_VertexData_3.m_texCoord = vec2(0.0, 0.0);

	Vertex l_VertexData_4;
	l_VertexData_4.m_pos = vec4(-1.0, 1.0, 1.0, 1.0);
	l_VertexData_4.m_texCoord = vec2(0.0, 1.0);

	Vertex l_VertexData_5;
	l_VertexData_5.m_pos = vec4(1.0, 1.0, -1.0, 1.0);
	l_VertexData_5.m_texCoord = vec2(1.0, 1.0);

	Vertex l_VertexData_6;
	l_VertexData_6.m_pos = vec4(1.0, -1.0, -1.0, 1.0);
	l_VertexData_6.m_texCoord = vec2(1.0, 0.0);

	Vertex l_VertexData_7;
	l_VertexData_7.m_pos = vec4(-1.0, -1.0, -1.0, 1.0);
	l_VertexData_7.m_texCoord = vec2(0.0, 0.0);

	Vertex l_VertexData_8;
	l_VertexData_8.m_pos = vec4(-1.0, 1.0, -1.0, 1.0);
	l_VertexData_8.m_texCoord = vec2(0.0, 1.0);

	std::vector<Vertex> l_vertices = { l_VertexData_1, l_VertexData_2, l_VertexData_3, l_VertexData_4, l_VertexData_5, l_VertexData_6, l_VertexData_7, l_VertexData_8 };

	for (auto& l_vertexData : l_vertices)
	{
		l_vertexData.m_normal = vec4(l_vertexData.m_pos.x, l_vertexData.m_pos.y, l_vertexData.m_pos.z, 0.0).normalize();
	}

	return l_vertices;
}

std::vector<Vertex> RenderingSystem::generateViewFrustum(const mat4& transformMatrix)
{
	auto l_NDCube = generateNDC();
	//@TODO: const influenza
	mat4 l_transformMatrix = transformMatrix;
	for (auto& l_vertexData : l_NDCube)
	{
		vec4 l_mulPos;
		l_mulPos = l_vertexData.m_pos;
		// from projection space to view space
		//Column-Major memory layout
#ifdef USE_COLUMN_MAJOR_MEMORY_LAYOUT
		l_mulPos = l_mulPos * transformMatrix;
#endif
		//Row-Major memory layout
#ifdef USE_ROW_MAJOR_MEMORY_LAYOUT
		l_mulPos = l_transformMatrix * l_mulPos;
#endif
		// perspective division
		l_mulPos = l_mulPos / l_mulPos.w;
		l_vertexData.m_pos = l_mulPos;
	}

	for (auto& l_vertexData : l_NDCube)
	{
		l_vertexData.m_normal = vec4(l_vertexData.m_pos.x, l_vertexData.m_pos.y, l_vertexData.m_pos.z, 0.0).normalize();
	}

	return l_NDCube;
}

void RenderingSystem::generateAABB(VisibleComponent & visibleComponent)
{
	double maxX = 0;
	double maxY = 0;
	double maxZ = 0;
	double minX = 0;
	double minY = 0;
	double minZ = 0;

	std::vector<vec4> l_cornerVertices;

	for (auto& l_graphicData : visibleComponent.getModelMap())
	{
		// get corner vertices from sub meshes
		l_cornerVertices.emplace_back(g_pAssetSystem->findMaxVertex(l_graphicData.first));
		l_cornerVertices.emplace_back(g_pAssetSystem->findMinVertex(l_graphicData.first));
	}

	std::for_each(l_cornerVertices.begin(), l_cornerVertices.end(), [&](vec4 val)
	{
		if (val.x >= maxX)
		{
			maxX = val.x;
		};
		if (val.y >= maxY)
		{
			maxY = val.y;
		};
		if (val.z >= maxZ)
		{
			maxZ = val.z;
		};
		if (val.x <= minX)
		{
			minX = val.x;
		};
		if (val.y <= minY)
		{
			minY = val.y;
		};
		if (val.z <= minZ)
		{
			minZ = val.z;
		};
	});

	visibleComponent.m_AABB = generateAABB(vec4(maxX, maxY, maxZ, 1.0), vec4(minX, minY, minZ, 1.0));
	auto l_worldTm = g_pGameSystem->getTransformComponent(visibleComponent.getParentEntity())->m_transform.caclGlobalTransformationMatrix();
	//Column-Major memory layout
#ifdef USE_COLUMN_MAJOR_MEMORY_LAYOUT
	visibleComponent.m_AABB.m_boundMax = visibleComponent.m_AABB.m_boundMax * l_worldTm;
	visibleComponent.m_AABB.m_boundMin = visibleComponent.m_AABB.m_boundMin *l_worldTm;
	visibleComponent.m_AABB.m_center = visibleComponent.m_AABB.m_center * l_worldTm;
	for (auto& l_vertexData : visibleComponent.m_AABB.m_vertices)
	{
		l_vertexData.m_pos = l_vertexData.m_pos * l_worldTm;
	}
#endif
	//Row-Major memory layout
#ifdef USE_ROW_MAJOR_MEMORY_LAYOUT
	visibleComponent.m_AABB.m_boundMax = l_worldTm * visibleComponent.m_AABB.m_boundMax;
	visibleComponent.m_AABB.m_boundMin = l_worldTm * visibleComponent.m_AABB.m_boundMin;
	visibleComponent.m_AABB.m_center = l_worldTm * visibleComponent.m_AABB.m_center;
	for (auto& l_vertexData : visibleComponent.m_AABB.m_vertices)
	{
		l_vertexData.m_pos = l_worldTm * l_vertexData.m_pos;
	}
#endif
}

void RenderingSystem::generateAABB(LightComponent & lightComponent)
{
	//1.translate the big frustum to light space
	auto l_camera = g_pGameSystem->getCameraComponents()[0];
	auto l_frustumVertices = l_camera->m_frustumVertices;

	auto l_lightInvertRotationMatrix = g_pGameSystem->getTransformComponent(lightComponent.getParentEntity())->m_transform.getInvertLocalRotMatrix();

	for (auto& l_vertexData : l_frustumVertices)
	{
		l_vertexData.m_pos = l_lightInvertRotationMatrix * l_vertexData.m_pos;
	}

	//2.calculate splited planes' corners
	std::vector<vec4> l_frustumsCornerPos;
	for (size_t i = 0; i < 4; i++)
	{
		l_frustumsCornerPos.emplace_back(l_frustumVertices[i].m_pos);
	}

	for (size_t i = 0; i < 4; i++)
	{
		for (size_t j = 0; j < 4; j++)
		{
			auto l_splitedPlaneCornerPos = l_frustumVertices[j].m_pos + ((l_frustumVertices[j + 4].m_pos - l_frustumVertices[j].m_pos) * (i + 1.0) / 4.0);
			l_frustumsCornerPos.emplace_back(l_splitedPlaneCornerPos);
		}
	}

	//3.assemble splited frustums
	std::vector<Vertex> l_frustumsCornerVertices;
	auto l_NDC = generateNDC();
	for (size_t i = 0; i < 4; i++)
	{
		l_frustumsCornerVertices.insert(l_frustumsCornerVertices.end(), l_NDC.begin(), l_NDC.end());
	}
	for (size_t i = 0; i < 4; i++)
	{
		for (size_t j = 0; j < 8; j++)
		{
			l_frustumsCornerVertices[i * 8 + j].m_pos = l_frustumsCornerPos[i * 4 + j];
		}
	}
	std::vector<std::vector<Vertex>> l_splitedFrustums;
	for (size_t i = 0; i < 4; i++)
	{
		l_splitedFrustums.emplace_back(std::vector<Vertex>(l_frustumsCornerVertices.begin() + i * 8, l_frustumsCornerVertices.begin() + 8 + i * 8));
	}

	//4.generate AABBs for the splited frustums
	for (size_t i = 0; i < 4; i++)
	{
		lightComponent.m_AABBs.emplace_back(generateAABB(l_splitedFrustums[i]));
	}
}

void RenderingSystem::generateAABB(CameraComponent & cameraComponent)
{
	auto l_frustumCorners = cameraComponent.m_frustumVertices;
	cameraComponent.m_AABB = generateAABB(l_frustumCorners);
}

AABB RenderingSystem::generateAABB(const std::vector<Vertex>& vertices)
{
	double maxX = vertices[0].m_pos.x;
	double maxY = vertices[0].m_pos.y;
	double maxZ = vertices[0].m_pos.z;
	double minX = vertices[0].m_pos.x;
	double minY = vertices[0].m_pos.y;
	double minZ = vertices[0].m_pos.z;

	for (auto& l_vertexData : vertices)
	{
		if (l_vertexData.m_pos.x >= maxX)
		{
			maxX = l_vertexData.m_pos.x;
		}
		if (l_vertexData.m_pos.y >= maxY)
		{
			maxY = l_vertexData.m_pos.y;
		}
		if (l_vertexData.m_pos.z >= maxZ)
		{
			maxZ = l_vertexData.m_pos.z;
		}
		if (l_vertexData.m_pos.x <= minX)
		{
			minX = l_vertexData.m_pos.x;
		}
		if (l_vertexData.m_pos.y <= minY)
		{
			minY = l_vertexData.m_pos.y;
		}
		if (l_vertexData.m_pos.z <= minZ)
		{
			minZ = l_vertexData.m_pos.z;
		}
	}

	return generateAABB(vec4(maxX, maxY, maxZ, 1.0), vec4(minX, minY, minZ, 1.0));
}

AABB RenderingSystem::generateAABB(const vec4 & boundMax, const vec4 & boundMin)
{
	AABB l_AABB;

	l_AABB.m_boundMin = boundMin;
	l_AABB.m_boundMax = boundMax;

	l_AABB.m_center = (l_AABB.m_boundMax + l_AABB.m_boundMin) * 0.5;
	l_AABB.m_sphereRadius = std::max(std::max((l_AABB.m_boundMax.x - l_AABB.m_boundMin.x) / 2.0, (l_AABB.m_boundMax.y - l_AABB.m_boundMin.y) / 2.0), (l_AABB.m_boundMax.z - l_AABB.m_boundMin.z) / 2.0);

	Vertex l_VertexData_1;
	l_VertexData_1.m_pos = (vec4(boundMax.x, boundMax.y, boundMax.z, 1.0));
	l_VertexData_1.m_texCoord = vec2(1.0, 1.0);

	Vertex l_VertexData_2;
	l_VertexData_2.m_pos = (vec4(boundMax.x, boundMin.y, boundMax.z, 1.0));
	l_VertexData_2.m_texCoord = vec2(1.0, 0.0);

	Vertex l_VertexData_3;
	l_VertexData_3.m_pos = (vec4(boundMin.x, boundMin.y, boundMax.z, 1.0));
	l_VertexData_3.m_texCoord = vec2(0.0, 0.0);

	Vertex l_VertexData_4;
	l_VertexData_4.m_pos = (vec4(boundMin.x, boundMax.y, boundMax.z, 1.0));
	l_VertexData_4.m_texCoord = vec2(0.0, 1.0);

	Vertex l_VertexData_5;
	l_VertexData_5.m_pos = (vec4(boundMax.x, boundMax.y, boundMin.z, 1.0));
	l_VertexData_5.m_texCoord = vec2(1.0, 1.0);

	Vertex l_VertexData_6;
	l_VertexData_6.m_pos = (vec4(boundMax.x, boundMin.y, boundMin.z, 1.0));
	l_VertexData_6.m_texCoord = vec2(1.0, 0.0);

	Vertex l_VertexData_7;
	l_VertexData_7.m_pos = (vec4(boundMin.x, boundMin.y, boundMin.z, 1.0));
	l_VertexData_7.m_texCoord = vec2(0.0, 0.0);

	Vertex l_VertexData_8;
	l_VertexData_8.m_pos = (vec4(boundMin.x, boundMax.y, boundMin.z, 1.0));
	l_VertexData_8.m_texCoord = vec2(0.0, 1.0);


	l_AABB.m_vertices = { l_VertexData_1, l_VertexData_2, l_VertexData_3, l_VertexData_4, l_VertexData_5, l_VertexData_6, l_VertexData_7, l_VertexData_8 };

	for (auto& l_vertexData : l_AABB.m_vertices)
	{
		l_vertexData.m_normal = vec4(l_vertexData.m_pos.x, l_vertexData.m_pos.y, l_vertexData.m_pos.z, 0.0).normalize();
	}

	l_AABB.m_indices = { 0, 1, 3, 1, 2, 3,
		4, 5, 0, 5, 1, 0,
		7, 6, 4, 6, 5, 4,
		3, 2, 7, 2, 6 ,7,
		4, 0, 7, 0, 3, 7,
		1, 5, 2, 5, 6, 2 };

	return l_AABB;
}

void RenderingSystem::changeDrawPolygonMode()
{
	if (m_polygonMode == 2)
	{
		m_polygonMode = 0;
	}
	else
	{
		m_polygonMode += 1;
	}
}

void RenderingSystem::changeDrawTextureMode()
{
	if (m_textureMode == 4)
	{
		m_textureMode = 0;
	}
	else
	{
		m_textureMode += 1;
	}
}

void RenderingSystem::changeShadingMode()
{
	if (m_shadingMode == 1)
	{
		m_shadingMode = 0;
	}
	else
	{
		m_shadingMode += 1;
	}
}
