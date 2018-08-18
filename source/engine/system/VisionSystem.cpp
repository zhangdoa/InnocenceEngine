#include "VisionSystem.h"

void VisionSystem::setup()
{
	setupWindow();
	setupInput();
	setupRendering();
	setupGui();

	m_objectStatus = objectStatus::ALIVE;
}

void VisionSystem::setupWindow()
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

void VisionSystem::setupInput()
{
	//setup input
	glfwSetInputMode(m_window, GLFW_STICKY_KEYS, GL_TRUE);

	for (int i = 0; i < NUM_KEYCODES; i++)
	{
		m_keyButtonMap.emplace(i, keyButton());
	}

	f_changeDrawPolygonMode = std::bind(&VisionSystem::changeDrawPolygonMode, this);
	f_changeDrawTextureMode = std::bind(&VisionSystem::changeDrawTextureMode, this);
	f_changeShadingMode = std::bind(&VisionSystem::changeShadingMode, this);
}

void VisionSystem::setupRendering()
{
	//setup rendering
	for (auto i : g_pGameSystem->getVisibleComponents())
	{
		if (i->m_visiblilityType != visiblilityType::INVISIBLE)
		{
			if (i->m_visiblilityType == visiblilityType::EMISSIVE)
			{
				RenderingSystemSingletonComponent::getInstance().m_emissiveVisibleComponents.emplace_back(i);
			}
			else if (i->m_visiblilityType == visiblilityType::STATIC_MESH)
			{
				RenderingSystemSingletonComponent::getInstance().m_staticMeshVisibleComponents.emplace_back(i);
			}
		}
	}
#ifdef INNO_RENDERER_OPENGL
	m_RenderingSystem = g_pMemorySystem->spawn<GLRenderingSystem>();
#elif INNO_RENDERER_DX
#elif INNO_RENDERER_VULKAN
#elif INNO_RENDERER_METAL
#endif
	m_RenderingSystem->setup();

	m_canRender = true;
}

inline void VisionSystem::setupGui()
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
	io.Fonts->AddFontFromFileTTF("..//res//fonts//FreeSans.otf", 16.0f);
}

void VisionSystem::initialize()
{
	initializeWindow();
	initializeInput();
	initializeRendering();
	initializeGui();

	g_pLogSystem->printLog("VisionSystem has been initialized.");
}

void VisionSystem::initializeWindow()
{
	//initialize window
	windowCallbackWrapper::getInstance().setVisionSystem(this);
	windowCallbackWrapper::getInstance().initialize();
}

void VisionSystem::initializeInput()
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

void VisionSystem::initializeRendering()
{
	m_RenderingSystem->initialize();
}

void VisionSystem::initializeGui()
{
}


void VisionSystem::updateInput()
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
		// @TODO: relative offset for editor window
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

void VisionSystem::updateRendering()
{
	RenderingSystemSingletonComponent::getInstance().m_selectedVisibleComponents.clear();
	RenderingSystemSingletonComponent::getInstance().m_inFrustumVisibleComponents.clear();

	if (g_pGameSystem->getCameraComponents().size() > 0)
	{
		m_mouseRay.m_origin = g_pGameSystem->getTransformComponent(g_pGameSystem->getCameraComponents()[0]->getParentEntity())->m_transform.caclGlobalPos();
		m_mouseRay.m_direction = calcMousePositionInWorldSpace();

		auto l_cameraAABB = g_pGameSystem->getCameraComponents()[0]->m_AABB;

		auto l_ray = g_pGameSystem->getCameraComponents()[0]->m_rayOfEye;

		for (auto& j : g_pGameSystem->getVisibleComponents())
		{
			if (j->m_visiblilityType == visiblilityType::STATIC_MESH || j->m_visiblilityType == visiblilityType::EMISSIVE)
			{
				if (j->m_AABB.intersectCheck(m_mouseRay))
				{
					RenderingSystemSingletonComponent::getInstance().m_selectedVisibleComponents.emplace_back(j);
				}
				if (l_cameraAABB.intersectCheck(j->m_AABB))
				{
					RenderingSystemSingletonComponent::getInstance().m_inFrustumVisibleComponents.emplace_back(j);
				}
			}
		}
	}
}

void VisionSystem::updateGui()
{
	auto l_renderTargetSize = ImVec2(RenderingSystemSingletonComponent::getInstance().m_renderTargetSize.x, RenderingSystemSingletonComponent::getInstance().m_renderTargetSize.y);
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
	glViewport(0, 0, m_screenResolution.x, m_screenResolution.y);
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
	ImGui::Begin("Window", 0, ImGuiWindowFlags_AlwaysAutoResize);
	ImGui::Image(ImTextureID((GLuint64)FinalRenderPassSingletonComponent::getInstance().m_finalBlendPassTexture.m_TAO), l_renderTargetSize, ImVec2(1.0, 1.0), ImVec2(0.0, 0.0));
	ImGui::End();
	glViewport(0, 0, m_screenResolution.x, m_screenResolution.y);
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	ImGui::Render();
	ImGui_ImplGlfwGL3_RenderDrawData(ImGui::GetDrawData());
}
#endif
}

void VisionSystem::update()
{
	if (m_window != nullptr && glfwWindowShouldClose(m_window) == 0)
	{
		glfwPollEvents();

		// input update
		updateInput();
		// rendeing update
		updateRendering();

		// defer render
		if (m_canRender)
		{
			m_canRender = false;
			m_RenderingSystem->update();
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
		g_pLogSystem->printLog("VisionSystem is stand-by.");
		m_objectStatus = objectStatus::STANDBY;
	}
}

void VisionSystem::shutdown()
{
	if (m_window != nullptr)
	{
		m_RenderingSystem->shutdown();

		glfwSetInputMode(m_window, GLFW_STICKY_KEYS, GL_FALSE);
		glfwDestroyWindow(m_window);
		glfwTerminate();
		g_pLogSystem->printLog("Window closed.");
	}
	m_objectStatus = objectStatus::SHUTDOWN;
	g_pLogSystem->printLog("VisionSystem has been shutdown.");
}

const objectStatus & VisionSystem::getStatus() const
{
	return m_objectStatus;
}

void VisionSystem::setWindowName(const std::string & windowName)
{
	m_windowName = windowName;
}

void VisionSystem::hideMouseCursor() const
{
	glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}

void VisionSystem::showMouseCursor() const
{
	glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
}

void VisionSystem::addKeyboardInputCallback(int keyCode, std::function<void()>* keyboardInputCallback)
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

void VisionSystem::addKeyboardInputCallback(int keyCode, std::vector<std::function<void()>*>& keyboardInputCallback)
{
	for (auto i : keyboardInputCallback)
	{
		addKeyboardInputCallback(keyCode, i);
	}
}

void VisionSystem::addKeyboardInputCallback(std::unordered_map<int, std::vector<std::function<void()>*>>& keyboardInputCallback)
{
	for (auto i : keyboardInputCallback)
	{
		addKeyboardInputCallback(i.first, i.second);
	}
}

void VisionSystem::addMouseMovementCallback(int mouseCode, std::function<void(double)>* mouseMovementCallback)
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

void VisionSystem::addMouseMovementCallback(int mouseCode, std::vector<std::function<void(double)>*>& mouseMovementCallback)
{
	for (auto i : mouseMovementCallback)
	{
		addMouseMovementCallback(mouseCode, i);
	}
}

void VisionSystem::addMouseMovementCallback(std::unordered_map<int, std::vector<std::function<void(double)>*>>& mouseMovementCallback)
{
	for (auto i : mouseMovementCallback)
	{
		addMouseMovementCallback(i.first, i.second);
	}
}

void VisionSystem::framebufferSizeCallback(int width, int height)
{
	m_screenResolution.x = width;
	m_screenResolution.y = height;
	glViewport(0, 0, width, height);
}

void VisionSystem::mousePositionCallback(double mouseXPos, double mouseYPos)
{
	m_mouseXOffset = mouseXPos -m_mouseLastX;
	m_mouseYOffset = m_mouseLastY - mouseYPos;

	m_mouseLastX = mouseXPos;
	m_mouseLastY = mouseYPos;
}

void VisionSystem::scrollCallback(double xoffset, double yoffset)
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
	//g_pPhysicsSystem->setupCameraComponentProjectionMatrix(g_pGameSystem->getCameraComponents()[0]);
}

void windowCallbackWrapper::setVisionSystem(VisionSystem * VisionSystem)
{
	m_visionSystem = VisionSystem;
}

void windowCallbackWrapper::initialize()
{
	glfwSetFramebufferSizeCallback(m_visionSystem->m_window, &framebufferSizeCallback);
	glfwSetCursorPosCallback(m_visionSystem->m_window, &mousePositionCallback);
	glfwSetScrollCallback(m_visionSystem->m_window, &scrollCallback);
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
	m_visionSystem->framebufferSizeCallback(width, height);
}

void windowCallbackWrapper::mousePositionCallbackImpl(GLFWwindow * window, double mouseXPos, double mouseYPos)
{
	m_visionSystem->mousePositionCallback(mouseXPos, mouseYPos);
}

void windowCallbackWrapper::scrollCallbackImpl(GLFWwindow * window, double xoffset, double yoffset)
{
	m_visionSystem->scrollCallback(xoffset, yoffset);
}


vec4 VisionSystem::calcMousePositionInWorldSpace()
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

void VisionSystem::changeDrawPolygonMode()
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

void VisionSystem::changeDrawTextureMode()
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

void VisionSystem::changeShadingMode()
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
