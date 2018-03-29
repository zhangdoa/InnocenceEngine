#include "RenderingSystem.h"

void RenderingSystem::setup()
{
	//setup window
	if (glfwInit() != GL_TRUE)
	{
		m_objectStatus = objectStatus::STANDBY;
		g_pLogSystem->printLog("Failed to initialize GLFW.");
	}

	glfwWindowHint(GLFW_SAMPLES, 16); // 16x antialiasing
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3); // We want OpenGL 3.3
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
#ifdef INNO_PLATFORM_MACOS
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // To make MacOS happy; should not be needed
#endif
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); //We don't want the old OpenGL 

	// Open a window and create its OpenGL context
	m_window = glfwCreateWindow(m_screenResolution.x, m_screenResolution.y, m_windowName.c_str(), NULL, NULL);
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

	// MSAA
	glEnable(GL_MULTISAMPLE);
	// enable seamless cubemap sampling for lower mip levels in the pre-filter map.
	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

	//setup input
	glfwSetInputMode(m_window, GLFW_STICKY_KEYS, GL_TRUE);

	for (int i = 0; i < NUM_KEYCODES; i++)
	{
		m_keyButtonMap.emplace(i, keyButton());
	}

	f_changeDrawPolygonMode = std::bind(&RenderingSystem::changeDrawPolygonMode, this);
	f_changeDrawTextureMode = std::bind(&RenderingSystem::changeDrawTextureMode, this);
	f_changeShadingMode = std::bind(&RenderingSystem::changeShadingMode, this);

	//setup rendering
	m_shadowForwardPassShaderProgram = g_pMemorySystem->spawn<ShadowForwardPassShaderProgram>();
	//m_shadowDeferPassShaderProgram = g_pMemorySystem->spawn<ShadowDeferPassShaderProgram>();

	m_environmentCapturePassShaderProgram = g_pMemorySystem->spawn<EnvironmentCapturePassPBSShaderProgram>();
	m_environmentConvolutionPassShaderProgram = g_pMemorySystem->spawn<EnvironmentConvolutionPassPBSShaderProgram>();
	m_environmentPreFilterPassShaderProgram = g_pMemorySystem->spawn<EnvironmentPreFilterPassPBSShaderProgram>();
	m_environmentBRDFLUTPassShaderProgram = g_pMemorySystem->spawn<EnvironmentBRDFLUTPassPBSShaderProgram>();
	//@TODO: add a switch for different shader model
	//m_geometryPassShader = g_pMemorySystem->spawn<GeometryPassBlinnPhongShader>();
	m_geometryPassShaderProgram = g_pMemorySystem->spawn<GeometryPassPBSShaderProgram>();

	//m_lightPassShader = g_pMemorySystem->spawn<LightPassBlinnPhongShader>();
	m_lightPassShaderProgram = g_pMemorySystem->spawn<LightPassPBSShaderProgram>();

	m_skyForwardPassShaderProgram = g_pMemorySystem->spawn<SkyForwardPassPBSShaderProgram>();
	m_skyDeferPassShaderProgram = g_pMemorySystem->spawn<SkyDeferPassPBSShaderProgram>();
	m_debuggerPassShaderProgram = g_pMemorySystem->spawn<DebuggerShaderProgram>();
	m_billboardPassShaderProgram = g_pMemorySystem->spawn<BillboardPassShaderProgram>();
	m_finalPassShaderProgram = g_pMemorySystem->spawn<FinalPassShaderProgram>();

	m_objectStatus = objectStatus::ALIVE;
}

void RenderingSystem::loadDefaultAssets()
{
	m_basicNormalTemplate = addTexture(textureType::NORMAL);
	m_basicAlbedoTemplate = addTexture(textureType::ALBEDO);
	m_basicMetallicTemplate = addTexture(textureType::METALLIC);
	m_basicRoughnessTemplate = addTexture(textureType::ROUGHNESS);
	m_basicAOTemplate = addTexture(textureType::AMBIENT_OCCLUSION);

	g_pAssetSystem->loadTextureFromDisk({ "basic_normal.png" }, textureType::NORMAL, textureWrapMethod::REPEAT, getTexture(textureType::NORMAL, m_basicNormalTemplate));
	g_pAssetSystem->loadTextureFromDisk({ "basic_albedo.png" }, textureType::ALBEDO, textureWrapMethod::REPEAT, getTexture(textureType::ALBEDO, m_basicAlbedoTemplate));
	g_pAssetSystem->loadTextureFromDisk({ "basic_metallic.png" }, textureType::METALLIC, textureWrapMethod::REPEAT, getTexture(textureType::METALLIC, m_basicMetallicTemplate));
	g_pAssetSystem->loadTextureFromDisk({ "basic_roughness.png" }, textureType::ROUGHNESS, textureWrapMethod::REPEAT, getTexture(textureType::ROUGHNESS, m_basicRoughnessTemplate));
	g_pAssetSystem->loadTextureFromDisk({ "basic_ao.png" }, textureType::AMBIENT_OCCLUSION, textureWrapMethod::REPEAT, getTexture(textureType::AMBIENT_OCCLUSION, m_basicAOTemplate));

	m_Unit3DQuadTemplate = addMesh(meshType::THREE_DIMENSION);
	auto last3DQuadMeshData = getMesh(meshType::THREE_DIMENSION, m_Unit3DQuadTemplate);
	last3DQuadMeshData->addUnitQuad();
	last3DQuadMeshData->setup(meshType::THREE_DIMENSION, meshDrawMethod::TRIANGLE, true, true);
	last3DQuadMeshData->initialize();

	m_Unit2DQuadTemplate = addMesh(meshType::TWO_DIMENSION);
	auto last2DQuadMeshData = getMesh(meshType::TWO_DIMENSION, m_Unit2DQuadTemplate);
	last2DQuadMeshData->addUnitQuad();
	last2DQuadMeshData->setup(meshType::TWO_DIMENSION, meshDrawMethod::TRIANGLE_STRIP, false, false);
	last2DQuadMeshData->initialize();

	m_UnitCubeTemplate = addMesh(meshType::THREE_DIMENSION);
	auto lastCubeMeshData = getMesh(meshType::THREE_DIMENSION, m_UnitCubeTemplate);
	lastCubeMeshData->addUnitCube();
	lastCubeMeshData->setup(meshType::THREE_DIMENSION, meshDrawMethod::TRIANGLE, false, false);
	lastCubeMeshData->initialize();

	m_UnitSphereTemplate = addMesh(meshType::THREE_DIMENSION);
	auto lastSphereMeshData = getMesh(meshType::THREE_DIMENSION, m_UnitSphereTemplate);
	lastSphereMeshData->addUnitSphere();
	lastSphereMeshData->setup(meshType::THREE_DIMENSION, meshDrawMethod::TRIANGLE_STRIP, false, false);
	lastSphereMeshData->initialize();

	m_UnitLineTemplate = addMesh(meshType::THREE_DIMENSION);
	auto lastLineMeshData = getMesh(meshType::THREE_DIMENSION, m_UnitLineTemplate);
	lastLineMeshData->addUnitLine();
	lastLineMeshData->setup(meshType::THREE_DIMENSION, meshDrawMethod::TRIANGLE_STRIP, false, false);
	lastLineMeshData->initialize();
}

void RenderingSystem::loadAssetsForComponents()
{
	for (auto i : g_pGameSystem->getVisibleComponents())
	{
		if (i->m_visiblilityType != visiblilityType::INVISIBLE)
		{
			// load or assign mesh data
			if (i->m_meshType == meshShapeType::CUSTOM)
			{
				if (i->m_modelFileName != "")
				{
					loadModel(i->m_modelFileName, *i);
				}
			}
			else
			{
				assignUnitMesh(*i, i->m_meshType);
			}
		}


		if (i->m_textureFileNameMap.size() != 0)
		{
			for (auto& j : i->m_textureFileNameMap)
			{
				loadTexture({ j.second }, j.first, *i);
			}
		}
	}

	// generate AABB for CSM
	for (auto& i : g_pGameSystem->getLightComponents())
	{
		if (i->getLightType() == lightType::DIRECTIONAL)
		{
			generateAABB(*i);
		}
	}

}

void RenderingSystem::initialize()
{
	//initialize window
	windowCallbackWrapper::getInstance().setRenderingSystem(this);
	windowCallbackWrapper::getInstance().initialize();

	//initialize input
	for (size_t i = 0; i < g_pGameSystem->getInputComponents().size(); i++)
	{
		// @TODO: multi input components need to register to multi map
		addKeyboardInputCallback(g_pGameSystem->getInputComponents()[i]->getKeyboardInputCallbackImpl());
		addMouseMovementCallback(g_pGameSystem->getInputComponents()[i]->getMouseInputCallbackImpl());
	}

	// @TODO: debt I owe
	addKeyboardInputCallback(INNO_KEY_Q, &f_changeDrawPolygonMode);
	m_keyButtonMap.find(INNO_KEY_Q)->second.m_keyPressType = keyPressType::ONCE;
	addKeyboardInputCallback(GLFW_KEY_E, &f_changeDrawTextureMode);
	m_keyButtonMap.find(INNO_KEY_E)->second.m_keyPressType = keyPressType::ONCE;
	addKeyboardInputCallback(INNO_KEY_R, &f_changeShadingMode);
	m_keyButtonMap.find(INNO_KEY_R)->second.m_keyPressType = keyPressType::ONCE;

	//load assets
	loadDefaultAssets();
	loadAssetsForComponents();

	//initialize rendering
	glEnable(GL_TEXTURE_2D);
	initializeShadowPass();
	initializeBackgroundPass();
	initializeGeometryPass();
	initializeLightPass();
	initializeFinalPass();

	m_objectStatus = objectStatus::ALIVE;
	g_pLogSystem->printLog("RenderingSystem has been initialized.");
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

void RenderingSystem::updatePhysics()
{
	// Raycast from camera
	m_selectedVisibleComponents.clear();
	for (auto& i : g_pGameSystem->getCameraComponents())
	{
		auto& l_ray = i->m_rayOfEye;

		for (auto& j : g_pGameSystem->getVisibleComponents())
		{
			if (j->m_visiblilityType == visiblilityType::STATIC_MESH)
			{
				if (j->m_AABB.intersectCheck(l_ray))
				{
					m_selectedVisibleComponents.emplace_back(j);
				}
			}
		}
	}
	// generate AABB for CSM
	for (auto& i : g_pGameSystem->getLightComponents())
	{
		if (i->getLightType() == lightType::DIRECTIONAL)
		{
			generateAABB(*i);
		}
	}
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
	}
	else
	{
		g_pLogSystem->printLog("Input error!");
		g_pLogSystem->printLog("RenderingSystem is stand-by.");
		m_objectStatus = objectStatus::STANDBY;
	}
}

void RenderingSystem::shutdown()
{
	if (m_window != nullptr)
	{
		glfwSetInputMode(m_window, GLFW_STICKY_KEYS, GL_FALSE);
		glfwDestroyWindow(m_window);
		glfwTerminate();
		g_pLogSystem->printLog("Window closed.");
	}
	m_objectStatus = objectStatus::SHUTDOWN;
	g_pLogSystem->printLog("RenderingSystem has been shutdown.");
}

bool RenderingSystem::canRender()
{
	return m_canRender;
}

const objectStatus & RenderingSystem::getStatus() const
{
	return m_objectStatus;
}

GLFWwindow * RenderingSystem::getWindow() const
{
	return m_window;
}

vec2 RenderingSystem::getScreenCenterPosition() const
{
	return vec2(m_screenResolution.x / 2, m_screenResolution.y / 2);
}

vec2 RenderingSystem::getScreenResolution() const
{
	return m_screenResolution;
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

void RenderingSystem::addKeyboardInputCallback(std::multimap<int, std::vector<std::function<void()>*>>& keyboardInputCallback)
{
	for (auto i : keyboardInputCallback)
	{
		addKeyboardInputCallback(i.first, i.second);
	}
}

void RenderingSystem::addMouseMovementCallback(int keyCode, std::function<void(double)>* mouseMovementCallback)
{
	auto l_mouseMovementCallbackFunctionVector = m_mouseMovementCallback.find(keyCode);
	if (l_mouseMovementCallbackFunctionVector != m_mouseMovementCallback.end())
	{
		l_mouseMovementCallbackFunctionVector->second.emplace_back(mouseMovementCallback);
	}
	else
	{
		m_mouseMovementCallback.emplace(keyCode, std::vector<std::function<void(double)>*>{mouseMovementCallback});
	}
}

void RenderingSystem::addMouseMovementCallback(int keyCode, std::vector<std::function<void(double)>*>& mouseMovementCallback)
{
	for (auto i : mouseMovementCallback)
	{
		addMouseMovementCallback(keyCode, i);
	}
}

void RenderingSystem::addMouseMovementCallback(std::multimap<int, std::vector<std::function<void(double)>*>>& mouseMovementCallback)
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
}

meshID RenderingSystem::addMesh(meshType meshType)
{
	BaseMesh* newMesh = g_pMemorySystem->spawn<MESH_CLASS>();
	if (meshType == meshType::AABB)
	{
		m_AABBMeshMap.emplace(std::pair<meshID, BaseMesh*>(newMesh->getMeshID(), newMesh));
	}
	else
	{
		m_meshMap.emplace(std::pair<meshID, BaseMesh*>(newMesh->getMeshID(), newMesh));
	}
	return newMesh->getMeshID();
}

textureID RenderingSystem::addTexture(textureType textureType)
{
	BaseTexture* newTexture = g_pMemorySystem->spawn<TEXTURE_CLASS>();
	m_textureMap.emplace(std::pair<textureID, BaseTexture*>(newTexture->getTextureID(), newTexture));
	return newTexture->getTextureID();
}

BaseMesh* RenderingSystem::getMesh(meshType meshType, meshID meshID)
{
	if (meshType == meshType::AABB)
	{
		return m_AABBMeshMap.find(meshID)->second;
	}
	else
	{
		return m_meshMap.find(meshID)->second;
	}
}

BaseTexture * RenderingSystem::getTexture(textureType textureType, textureID textureID)
{
	return m_textureMap.find(textureID)->second;
}

void RenderingSystem::assignUnitMesh(VisibleComponent & visibleComponent, meshShapeType meshType)
{
	meshID l_UnitMeshTemplate;
	switch (meshType)
	{
	case meshShapeType::QUAD: l_UnitMeshTemplate = m_Unit3DQuadTemplate; break;
	case meshShapeType::CUBE: l_UnitMeshTemplate = m_UnitCubeTemplate; break;
	case meshShapeType::SPHERE: l_UnitMeshTemplate = m_UnitSphereTemplate; break;
	}
	visibleComponent.addMeshData(l_UnitMeshTemplate);

	// generate AABB
	generateAABB(visibleComponent);

	assignDefaultTextures(textureAssignType::OVERWRITE, visibleComponent);
}

void RenderingSystem::assignLoadedTexture(textureAssignType textureAssignType, texturePair& loadedtexturePair, VisibleComponent & visibleComponent)
{
	if (textureAssignType == textureAssignType::ADD_DEFAULT)
	{
		visibleComponent.addTextureData(loadedtexturePair);
	}
	else if (textureAssignType == textureAssignType::OVERWRITE)
	{
		visibleComponent.overwriteTextureData(loadedtexturePair);
	}
}

void RenderingSystem::assignDefaultTextures(textureAssignType textureAssignType, VisibleComponent & visibleComponent)
{
	if (visibleComponent.m_visiblilityType == visiblilityType::STATIC_MESH)
	{
		assignLoadedTexture(textureAssignType, texturePair(textureType::NORMAL, m_basicNormalTemplate), visibleComponent);
		assignLoadedTexture(textureAssignType, texturePair(textureType::ALBEDO, m_basicAlbedoTemplate), visibleComponent);
		assignLoadedTexture(textureAssignType, texturePair(textureType::METALLIC, m_basicMetallicTemplate), visibleComponent);
		assignLoadedTexture(textureAssignType, texturePair(textureType::ROUGHNESS, m_basicRoughnessTemplate), visibleComponent);
		assignLoadedTexture(textureAssignType, texturePair(textureType::AMBIENT_OCCLUSION, m_basicAOTemplate), visibleComponent);
	}
}

void RenderingSystem::assignloadedModel(modelMap& loadedmodelMap, VisibleComponent & visibleComponent)
{
	visibleComponent.setModelMap(loadedmodelMap);
	assignDefaultTextures(textureAssignType::ADD_DEFAULT, visibleComponent);
}

std::vector<Vertex> RenderingSystem::generateNDC()
{
	Vertex l_VertexData_1;
	l_VertexData_1.m_pos = vec4(1.0f, 1.0f, 1.0f, 1.0f);
	l_VertexData_1.m_texCoord = vec2(1.0f, 1.0f);

	Vertex l_VertexData_2;
	l_VertexData_2.m_pos = vec4(1.0f, -1.0f, 1.0f, 1.0f);
	l_VertexData_2.m_texCoord = vec2(1.0f, 0.0f);

	Vertex l_VertexData_3;
	l_VertexData_3.m_pos = vec4(-1.0f, -1.0f, 1.0f, 1.0f);
	l_VertexData_3.m_texCoord = vec2(0.0f, 0.0f);

	Vertex l_VertexData_4;
	l_VertexData_4.m_pos = vec4(-1.0f, 1.0f, 1.0f, 1.0f);
	l_VertexData_4.m_texCoord = vec2(0.0f, 1.0f);

	Vertex l_VertexData_5;
	l_VertexData_5.m_pos = vec4(1.0f, 1.0f, -1.0f, 1.0f);
	l_VertexData_5.m_texCoord = vec2(1.0f, 1.0f);

	Vertex l_VertexData_6;
	l_VertexData_6.m_pos = vec4(1.0f, -1.0f, -1.0f, 1.0f);
	l_VertexData_6.m_texCoord = vec2(1.0f, 0.0f);

	Vertex l_VertexData_7;
	l_VertexData_7.m_pos = vec4(-1.0f, -1.0f, -1.0f, 1.0f);
	l_VertexData_7.m_texCoord = vec2(0.0f, 0.0f);

	Vertex l_VertexData_8;
	l_VertexData_8.m_pos = vec4(-1.0f, 1.0f, -1.0f, 1.0f);
	l_VertexData_8.m_texCoord = vec2(0.0f, 1.0f);

	std::vector<Vertex> l_vertices = { l_VertexData_1, l_VertexData_2, l_VertexData_3, l_VertexData_4, l_VertexData_5, l_VertexData_6, l_VertexData_7, l_VertexData_8 };
	
	for (auto& l_vertexData : l_vertices)
	{
		l_vertexData.m_normal = vec4(l_vertexData.m_pos.x, l_vertexData.m_pos.y, l_vertexData.m_pos.z, 0.0).normalize();
	}

	return l_vertices;
}

void RenderingSystem::generateAABB(VisibleComponent & visibleComponent)
{
	auto l_scale = visibleComponent.getParentEntity()->getTransform()->getScale();

	double maxX = 0;
	double maxY = 0;
	double maxZ = 0;
	double minX = 0;
	double minY = 0;
	double minZ = 0;

	std::vector<vec4> l_maxVertices;
	std::vector<vec4> l_minVertices;

	for (auto& l_graphicData : visibleComponent.getModelMap())
	{
		// draw meshes
		l_maxVertices.emplace_back(m_meshMap.find(l_graphicData.first)->second->findMaxVertex());
		l_minVertices.emplace_back(m_meshMap.find(l_graphicData.first)->second->findMinVertex());
	}

	std::for_each(l_maxVertices.begin(), l_maxVertices.end(), [&](vec4 val)
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
	});

	std::for_each(l_minVertices.begin(), l_minVertices.end(), [&](vec4 val)
	{
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

	visibleComponent.m_AABB = generateAABB(vec4(maxX, maxY, maxZ, 1.0), vec4(minX, minY, minZ, 1.0), l_scale);

	if (visibleComponent.m_drawAABB)
	{
		visibleComponent.m_AABBMeshID = addAABBMesh(visibleComponent.m_AABB);
	}
}

void RenderingSystem::generateAABB(LightComponent & lightComponent)
{
	double maxX = 0;
	double maxY = 0;
	double maxZ = 0;
	double minX = 0;
	double minY = 0;
	double minZ = 0;

	//auto pCamera = g_pGameSystem->getCameraComponents()[0]->getProjectionMatrix();
	auto pCamera = mat4();
	pCamera.initializeToPerspectiveMatrix((70.0 / 180.0) * PI, (16.0 / 9.0), 0.1, 1.0);
	auto rCamera = g_pGameSystem->getCameraComponents()[0]->getRotMatrix();
	auto tCamera = g_pGameSystem->getCameraComponents()[0]->getPosMatrix();
	auto l_vLight = mat4().lookAt(vec4(0.0, 0.0, 0.0, 1.0), vec4(0.0, 0.0, 0.0, 1.0) + lightComponent.getParentEntity()->getTransform()->getDirection(Transform::direction::FORWARD), lightComponent.getParentEntity()->getTransform()->getDirection(Transform::direction::UP));

	// get view frustrum's corner in world space
	auto l_vertices = generateNDC();
	for (auto& l_vertexData : l_vertices)
	{
		vec4 l_mulPos = pCamera.inverse() * l_vertexData.m_pos;
		// perspective division
		l_mulPos = l_mulPos * (1.0 / l_mulPos.w);
		// to view space
		l_mulPos = rCamera.inverse() * l_mulPos;
		l_mulPos = tCamera.inverse() * l_mulPos;
		// trasform view frustrum's corner to light space
		l_mulPos = l_vLight * l_mulPos;
		l_vertexData.m_pos = l_mulPos;	
	}
	for (auto& l_vertexData : l_vertices)
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
	
	lightComponent.m_AABB = generateAABB(vec4(maxX, maxY, maxZ, 1.0), vec4(minX, minY, minZ, 1.0), vec4(1.0, 1.0, 1.0, 1.0));
	lightComponent.getParentEntity()->getTransform()->setPos(lightComponent.getParentEntity()->getTransform()->getDirection(Transform::direction::FORWARD) * (maxZ - minZ) + lightComponent.m_AABB.m_center);

	//if (lightComponent.m_drawAABB)
	//{
	//	lightComponent.m_AABBMeshID = addAABBMesh(lightComponent.m_AABB);
	//}
}

AABB RenderingSystem::generateAABB(const vec4 & boundMax, const vec4 & boundMin, const vec4& scale)
{
	AABB l_AABB;
	// unscaled version for visualization
	auto l_unscaledBoundMin = boundMin;
	auto l_unscaledBoundMax = boundMax;

	// scaled version for collision calculation
	l_AABB.m_boundMin = l_unscaledBoundMin.scale(scale);
	l_AABB.m_boundMax = l_unscaledBoundMax.scale(scale);

	l_AABB.m_center = (l_AABB.m_boundMax + l_AABB.m_boundMin) * 0.5;
	l_AABB.m_sphereRadius = std::max(std::max((l_AABB.m_boundMax.x - l_AABB.m_boundMin.x) / 2, (l_AABB.m_boundMax.y - l_AABB.m_boundMin.y) / 2), (l_AABB.m_boundMax.z - l_AABB.m_boundMin.z) / 2);

	Vertex l_VertexData_1;
	l_VertexData_1.m_pos = (vec4(l_unscaledBoundMax.x, l_unscaledBoundMax.y, l_unscaledBoundMax.z, 1.0));
	l_VertexData_1.m_texCoord = vec2(1.0f, 1.0f);

	Vertex l_VertexData_2;
	l_VertexData_2.m_pos = (vec4(l_unscaledBoundMax.x, l_unscaledBoundMin.y, l_unscaledBoundMax.z, 1.0));
	l_VertexData_2.m_texCoord = vec2(1.0f, 0.0f);

	Vertex l_VertexData_3;
	l_VertexData_3.m_pos = (vec4(l_unscaledBoundMin.x, l_unscaledBoundMin.y, l_unscaledBoundMax.z, 1.0));
	l_VertexData_3.m_texCoord = vec2(0.0f, 0.0f);

	Vertex l_VertexData_4;
	l_VertexData_4.m_pos = (vec4(l_unscaledBoundMin.x, l_unscaledBoundMax.y, l_unscaledBoundMax.z, 1.0));
	l_VertexData_4.m_texCoord = vec2(0.0f, 1.0f);

	Vertex l_VertexData_5;
	l_VertexData_5.m_pos = (vec4(l_unscaledBoundMax.x, l_unscaledBoundMax.y, l_unscaledBoundMin.z, 1.0));
	l_VertexData_5.m_texCoord = vec2(1.0f, 1.0f);

	Vertex l_VertexData_6;
	l_VertexData_6.m_pos = (vec4(l_unscaledBoundMax.x, l_unscaledBoundMin.y, l_unscaledBoundMin.z, 1.0));
	l_VertexData_6.m_texCoord = vec2(1.0f, 0.0f);

	Vertex l_VertexData_7;
	l_VertexData_7.m_pos = (vec4(l_unscaledBoundMin.x, l_unscaledBoundMin.y, l_unscaledBoundMin.z, 1.0));
	l_VertexData_7.m_texCoord = vec2(0.0f, 0.0f);

	Vertex l_VertexData_8;
	l_VertexData_8.m_pos = (vec4(l_unscaledBoundMin.x, l_unscaledBoundMax.y, l_unscaledBoundMin.z, 1.0));
	l_VertexData_8.m_texCoord = vec2(0.0f, 1.0f);


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

meshID RenderingSystem::addAABBMesh(const AABB & AABB)
{
	auto l_AABBMeshID = addMesh(meshType::AABB);
	auto l_AABBMeshData = getMesh(meshType::AABB, l_AABBMeshID);

	std::for_each(AABB.m_vertices.begin(), AABB.m_vertices.end(), [&](Vertex val)
	{
		l_AABBMeshData->addVertices(val);
	});

	std::for_each(AABB.m_indices.begin(), AABB.m_indices.end(), [&](unsigned int val)
	{
		l_AABBMeshData->addIndices(val);
	});

	l_AABBMeshData->setup(meshType::AABB, meshDrawMethod::TRIANGLE, false, false);
	l_AABBMeshData->initialize();
	return l_AABBMeshID;
}

void RenderingSystem::loadTexture(const std::vector<std::string> &fileName, textureType textureType, VisibleComponent & visibleComponent)
{
	for (auto& i : fileName)
	{
		auto l_loadedTexturePair = m_loadedTextureMap.find(i);
		// check if this file has already loaded
		if (l_loadedTexturePair != m_loadedTextureMap.end())
		{
			assignLoadedTexture(textureAssignType::OVERWRITE, l_loadedTexturePair->second, visibleComponent);
			g_pLogSystem->printLog("innoTexture: " + i + " is already loaded, successfully assigned loaded textureID.");
		}
		else
		{
			auto l_textureID = addTexture(textureType);
			auto l_baseTexture = getTexture(textureType, l_textureID);
			g_pAssetSystem->loadTextureFromDisk({ i }, textureType, visibleComponent.m_textureWrapMethod, l_baseTexture);
			m_loadedTextureMap.emplace(i, texturePair(textureType, l_textureID));
			assignLoadedTexture(textureAssignType::OVERWRITE, texturePair(textureType, l_textureID), visibleComponent);
		}
	}
}

void RenderingSystem::loadModel(const std::string & fileName, VisibleComponent & visibleComponent)
{
	auto l_convertedFilePath = fileName.substr(0, fileName.find(".")) + ".innoModel";

	// check if this file has already been loaded once
	auto l_loadedmodelMap = m_loadedModelMap.find(l_convertedFilePath);
	if (l_loadedmodelMap != m_loadedModelMap.end())
	{
		assignloadedModel(l_loadedmodelMap->second, visibleComponent);
		g_pLogSystem->printLog("innoMesh: " + l_convertedFilePath + " is already loaded, successfully assigned loaded modelMap.");
	}
	else
	{
		modelMap l_modelMap;
		g_pAssetSystem->loadModelFromDisk(fileName, l_modelMap, visibleComponent.m_meshDrawMethod, visibleComponent.m_textureWrapMethod, visibleComponent.m_caclNormal);

		assignloadedModel(l_modelMap, visibleComponent);

		// generate AABB
		generateAABB(visibleComponent);

		//mark as loaded
		m_loadedModelMap.emplace(l_convertedFilePath, l_modelMap);
	}
}

void RenderingSystem::render()
{
	//defer render
	m_canRender = false;
	renderShadowPass(g_pGameSystem->getCameraComponents(), g_pGameSystem->getLightComponents(), g_pGameSystem->getVisibleComponents());
	renderBackgroundPass(g_pGameSystem->getCameraComponents(), g_pGameSystem->getLightComponents(), g_pGameSystem->getVisibleComponents());
	renderGeometryPass(g_pGameSystem->getCameraComponents(), g_pGameSystem->getLightComponents(), g_pGameSystem->getVisibleComponents());
	renderLightPass(g_pGameSystem->getCameraComponents(), g_pGameSystem->getLightComponents(), g_pGameSystem->getVisibleComponents());
	renderFinalPass(g_pGameSystem->getCameraComponents(), g_pGameSystem->getLightComponents(), g_pGameSystem->getVisibleComponents());
	//swap framebuffers
	if (m_window != nullptr && glfwWindowShouldClose(m_window) == 0)
	{
		glfwSwapBuffers(m_window);
		m_canRender = true;
	}
	else
	{
		g_pLogSystem->printLog("Window error!");
		g_pLogSystem->printLog("RenderingSystem is stand-by.");
		m_objectStatus = objectStatus::STANDBY;
	}
}

void RenderingSystem::initializeBackgroundPass()
{
	// environment capture pass
	// initialize shader
	auto l_environmentCapturePassVertexShaderFilePath = "GL3.3/environmentCapturePassPBSVertex.sf";
	auto l_environmentCapturePassVertexShaderData = shaderData(shaderType::VERTEX, shaderCodeContentPair(l_environmentCapturePassVertexShaderFilePath, g_pAssetSystem->loadShader(l_environmentCapturePassVertexShaderFilePath)));
	auto l_environmentCapturePassFragmentShaderFilePath = "GL3.3/environmentCapturePassPBSFragment.sf";
	auto l_environmentCapturePassFragmentShaderData = shaderData(shaderType::FRAGMENT, shaderCodeContentPair(l_environmentCapturePassFragmentShaderFilePath, g_pAssetSystem->loadShader(l_environmentCapturePassFragmentShaderFilePath)));
	m_environmentCapturePassShaderProgram->setup({ l_environmentCapturePassVertexShaderData , l_environmentCapturePassFragmentShaderData });

	// initialize texture
	m_environmentCapturePassTextureID = this->addTexture(textureType::ENVIRONMENT_CAPTURE);
	auto l_environmentCapturePassTextureData = this->getTexture(textureType::ENVIRONMENT_CAPTURE, m_environmentCapturePassTextureID);
	l_environmentCapturePassTextureData->setup(textureType::ENVIRONMENT_CAPTURE, textureColorComponentsFormat::RGB16F, texturePixelDataFormat::RGB, textureWrapMethod::REPEAT, textureFilterMethod::LINEAR_MIPMAP_LINEAR, textureFilterMethod::LINEAR, 2048, 2048, texturePixelDataType::FLOAT, std::vector<void*>{nullptr, nullptr, nullptr, nullptr, nullptr, nullptr});

	// environment convolution pass
	// initialize shader
	auto l_environmentConvolutionPassVertexShaderFilePath = "GL3.3/environmentConvolutionPassPBSVertex.sf";
	auto l_environmentConvolutionPassVertexShaderData = shaderData(shaderType::VERTEX, shaderCodeContentPair(l_environmentConvolutionPassVertexShaderFilePath, g_pAssetSystem->loadShader(l_environmentConvolutionPassVertexShaderFilePath)));
	auto l_environmentConvolutionPassFragmentShaderFilePath = "GL3.3/environmentConvolutionPassPBSFragment.sf";
	auto l_environmentConvolutionPassFragmentShaderData = shaderData(shaderType::FRAGMENT, shaderCodeContentPair(l_environmentConvolutionPassFragmentShaderFilePath, g_pAssetSystem->loadShader(l_environmentConvolutionPassFragmentShaderFilePath)));
	m_environmentConvolutionPassShaderProgram->setup({ l_environmentConvolutionPassVertexShaderData , l_environmentConvolutionPassFragmentShaderData });

	// initialize texture
	m_environmentConvolutionPassTextureID = this->addTexture(textureType::ENVIRONMENT_CONVOLUTION);
	auto l_environmentConvolutionPassTextureData = this->getTexture(textureType::ENVIRONMENT_CONVOLUTION, m_environmentConvolutionPassTextureID);
	l_environmentConvolutionPassTextureData->setup(textureType::ENVIRONMENT_CONVOLUTION, textureColorComponentsFormat::RGB16F, texturePixelDataFormat::RGB, textureWrapMethod::REPEAT, textureFilterMethod::LINEAR, textureFilterMethod::LINEAR, 128, 128, texturePixelDataType::FLOAT, std::vector<void*>{nullptr, nullptr, nullptr, nullptr, nullptr, nullptr});

	// environment pre-filter pass
	// initialize shader
	auto l_environmentPreFilterPassVertexShaderFilePath = "GL3.3/environmentPreFilterPassPBSVertex.sf";
	auto l_environmentPreFilterPassVertexShaderData = shaderData(shaderType::VERTEX, shaderCodeContentPair(l_environmentPreFilterPassVertexShaderFilePath, g_pAssetSystem->loadShader(l_environmentPreFilterPassVertexShaderFilePath)));
	auto l_environmentPreFilterPassFragmentShaderFilePath = "GL3.3/environmentPreFilterPassPBSFragment.sf";
	auto l_environmentPreFilterPassFragmentShaderData = shaderData(shaderType::FRAGMENT, shaderCodeContentPair(l_environmentPreFilterPassFragmentShaderFilePath, g_pAssetSystem->loadShader(l_environmentPreFilterPassFragmentShaderFilePath)));
	m_environmentPreFilterPassShaderProgram->setup({ l_environmentPreFilterPassVertexShaderData , l_environmentPreFilterPassFragmentShaderData });

	// initialize texture
	m_environmentPreFilterPassTextureID = this->addTexture(textureType::ENVIRONMENT_PREFILTER);
	auto l_environmentPreFilterPassTextureData = this->getTexture(textureType::ENVIRONMENT_PREFILTER, m_environmentPreFilterPassTextureID);
	l_environmentPreFilterPassTextureData->setup(textureType::ENVIRONMENT_PREFILTER, textureColorComponentsFormat::RGB16F, texturePixelDataFormat::RGB, textureWrapMethod::REPEAT, textureFilterMethod::LINEAR_MIPMAP_LINEAR, textureFilterMethod::LINEAR, 128, 128, texturePixelDataType::FLOAT, std::vector<void*>{nullptr, nullptr, nullptr, nullptr, nullptr, nullptr});

	// environment BRDF LUT pass
	// initialize shader
	auto l_environmentBRDFLUTPassVertexShaderFilePath = "GL3.3/environmentBRDFLUTPassPBSVertex.sf";
	auto l_environmentBRDFLUTPassVertexShaderData = shaderData(shaderType::VERTEX, shaderCodeContentPair(l_environmentBRDFLUTPassVertexShaderFilePath, g_pAssetSystem->loadShader(l_environmentBRDFLUTPassVertexShaderFilePath)));
	auto l_environmentBRDFLUTPassFragmentShaderFilePath = "GL3.3/environmentBRDFLUTPassPBSFragment.sf";
	auto l_environmentBRDFLUTPassFragmentShaderData = shaderData(shaderType::FRAGMENT, shaderCodeContentPair(l_environmentBRDFLUTPassFragmentShaderFilePath, g_pAssetSystem->loadShader(l_environmentBRDFLUTPassFragmentShaderFilePath)));
	m_environmentBRDFLUTPassShaderProgram->setup({ l_environmentBRDFLUTPassVertexShaderData , l_environmentBRDFLUTPassFragmentShaderData });

	// initialize texture
	m_environmentBRDFLUTTextureID = this->addTexture(textureType::RENDER_BUFFER_SAMPLER);
	auto l_environmentBRDFLUTTextureData = this->getTexture(textureType::RENDER_BUFFER_SAMPLER, m_environmentBRDFLUTTextureID);
	l_environmentBRDFLUTTextureData->setup(textureType::RENDER_BUFFER_SAMPLER, textureColorComponentsFormat::RG16F, texturePixelDataFormat::RG, textureWrapMethod::CLAMP_TO_EDGE, textureFilterMethod::LINEAR, textureFilterMethod::LINEAR, 512, 512, texturePixelDataType::FLOAT, std::vector<void*>{ nullptr });

	// initialize environment pass framebuffer
	auto l_environmentPassRenderBufferStorageSizes = std::vector<vec2>{ vec2(2048, 2048), vec2(128, 128), vec2(128, 128), vec2(512, 512) };
	auto l_environmentPassRenderTargetTextures = std::vector<BaseTexture*>{ l_environmentCapturePassTextureData, l_environmentConvolutionPassTextureData, l_environmentPreFilterPassTextureData, l_environmentBRDFLUTTextureData };
	auto l_environmentPassShaderPrograms = std::vector<BaseShaderProgram*>{ m_environmentCapturePassShaderProgram,  m_environmentConvolutionPassShaderProgram, m_environmentPreFilterPassShaderProgram, m_environmentBRDFLUTPassShaderProgram };
	m_environmentPassFrameBuffer = g_pMemorySystem->spawn<FRAMEBUFFER_CLASS>();
	m_environmentPassFrameBuffer->setup(frameBufferType::ENVIRONMENT_PASS, renderBufferType::DEPTH, l_environmentPassRenderBufferStorageSizes, l_environmentPassRenderTargetTextures, l_environmentPassShaderPrograms);
	m_environmentPassFrameBuffer->initialize();

	//assign the textures to Skybox visibleComponent
	// @TODO: multi CaptureRRRRRRRRR
	for (auto i : g_pGameSystem->getVisibleComponents())
	{
		if (i->m_visiblilityType == visiblilityType::SKYBOX)
		{
			i->overwriteTextureData(texturePair(textureType::ENVIRONMENT_CAPTURE, m_environmentCapturePassTextureID));
			i->overwriteTextureData(texturePair(textureType::ENVIRONMENT_CONVOLUTION, m_environmentConvolutionPassTextureID));
			i->overwriteTextureData(texturePair(textureType::ENVIRONMENT_PREFILTER, m_environmentPreFilterPassTextureID));
			i->overwriteTextureData(texturePair(textureType::RENDER_BUFFER_SAMPLER, m_environmentBRDFLUTTextureID));
			i->overwriteTextureData(texturePair(textureType::CUBEMAP, m_environmentCapturePassTextureID));
		}
	}
}

void RenderingSystem::renderBackgroundPass(std::vector<CameraComponent*>& cameraComponents, std::vector<LightComponent*>& lightComponents, std::vector<VisibleComponent*>& visibleComponents)
{
	if (m_shouldUpdateEnvironmentMap)
	{
		m_environmentPassFrameBuffer->update(cameraComponents, lightComponents, visibleComponents, m_meshMap, m_textureMap, true, true);

		// draw environment map BRDF LUT rectangle
		this->getMesh(meshType::TWO_DIMENSION, m_Unit2DQuadTemplate)->update();

		m_shouldUpdateEnvironmentMap = false;
	}
}


void RenderingSystem::initializeShadowPass()
{
	// shadow forward pass
	// initialize shader
	auto l_shadowForwardPassVertexShaderFilePath = "GL3.3/shadowForwardPassVertex.sf";
	auto l_shadowForwardPassVertexShaderData = shaderData(shaderType::VERTEX, shaderCodeContentPair(l_shadowForwardPassVertexShaderFilePath, g_pAssetSystem->loadShader(l_shadowForwardPassVertexShaderFilePath)));
	auto l_shadowForwardPassFragmentShaderFilePath = "GL3.3/shadowForwardPassFragment.sf";
	auto l_shadowForwardPassFragmentShaderData = shaderData(shaderType::FRAGMENT, shaderCodeContentPair(l_shadowForwardPassFragmentShaderFilePath, g_pAssetSystem->loadShader(l_shadowForwardPassFragmentShaderFilePath)));
	m_shadowForwardPassShaderProgram->setup({ l_shadowForwardPassVertexShaderData , l_shadowForwardPassFragmentShaderData });

	// initialize texture
	m_shadowForwardPassTextureID = this->addTexture(textureType::SHADOWMAP);
	auto l_shadowForwardPassTextureData = this->getTexture(textureType::SHADOWMAP, m_shadowForwardPassTextureID);
	l_shadowForwardPassTextureData->setup(textureType::SHADOWMAP, textureColorComponentsFormat::DEPTH_COMPONENT, texturePixelDataFormat::DEPTH_COMPONENT, textureWrapMethod::CLAMP_TO_BORDER, textureFilterMethod::NEAREST, textureFilterMethod::NEAREST, 4096, 4096, texturePixelDataType::FLOAT, std::vector<void*>{ nullptr });

	// initialize framebuffer
	auto l_shadowForwardPassRenderBufferStorageSizes = std::vector<vec2>{ vec2(4096, 4096) };
	auto l_shadowForwardPassRenderTargetTextures = std::vector<BaseTexture*>{ l_shadowForwardPassTextureData };
	auto l_shadowForwardPassShaderPrograms = std::vector<BaseShaderProgram*>{ m_shadowForwardPassShaderProgram };
	m_shadowForwardPassFrameBuffer = g_pMemorySystem->spawn<FRAMEBUFFER_CLASS>();
	m_shadowForwardPassFrameBuffer->setup(frameBufferType::SHADOW_PASS, renderBufferType::DEPTH, l_shadowForwardPassRenderBufferStorageSizes, l_shadowForwardPassRenderTargetTextures, l_shadowForwardPassShaderPrograms);
	m_shadowForwardPassFrameBuffer->initialize();
}

void RenderingSystem::renderShadowPass(std::vector<CameraComponent*>& cameraComponents, std::vector<LightComponent*>& lightComponents, std::vector<VisibleComponent*>& visibleComponents)
{
	// draw shadow forward pass
	m_shadowForwardPassFrameBuffer->update(cameraComponents, lightComponents, visibleComponents, m_meshMap, m_textureMap, true, true);
}

void RenderingSystem::initializeGeometryPass()
{
	// initialize shader
	auto l_vertexShaderFilePath = "GL3.3/geometryPassPBSVertex.sf";
	auto l_vertexShaderData = shaderData(shaderType::VERTEX, shaderCodeContentPair(l_vertexShaderFilePath, g_pAssetSystem->loadShader(l_vertexShaderFilePath)));
	auto l_fragmentShaderFilePath = "GL3.3/geometryPassPBSFragment.sf";
	auto l_fragmentShaderData = shaderData(shaderType::FRAGMENT, shaderCodeContentPair(l_fragmentShaderFilePath, g_pAssetSystem->loadShader(l_fragmentShaderFilePath)));
	m_geometryPassShaderProgram->setup({ l_vertexShaderData , l_fragmentShaderData });

	// initialize texture
	m_geometryPassRT0TextureID = this->addTexture(textureType::RENDER_BUFFER_SAMPLER);
	auto l_geometryPassRT0TextureData = this->getTexture(textureType::RENDER_BUFFER_SAMPLER, m_geometryPassRT0TextureID);
	l_geometryPassRT0TextureData->setup(textureType::RENDER_BUFFER_SAMPLER, textureColorComponentsFormat::RGBA16F, texturePixelDataFormat::RGBA, textureWrapMethod::CLAMP_TO_EDGE, textureFilterMethod::NEAREST, textureFilterMethod::NEAREST, m_screenResolution.x, m_screenResolution.y, texturePixelDataType::FLOAT, std::vector<void*>{ nullptr });

	m_geometryPassRT1TextureID = this->addTexture(textureType::RENDER_BUFFER_SAMPLER);
	auto l_geometryPassRT1TextureData = this->getTexture(textureType::RENDER_BUFFER_SAMPLER, m_geometryPassRT1TextureID);
	l_geometryPassRT1TextureData->setup(textureType::RENDER_BUFFER_SAMPLER, textureColorComponentsFormat::RGBA16F, texturePixelDataFormat::RGBA, textureWrapMethod::CLAMP_TO_EDGE, textureFilterMethod::NEAREST, textureFilterMethod::NEAREST, m_screenResolution.x, m_screenResolution.y, texturePixelDataType::FLOAT, std::vector<void*>{ nullptr });

	m_geometryPassRT2TextureID = this->addTexture(textureType::RENDER_BUFFER_SAMPLER);
	auto l_geometryPassRT2TextureData = this->getTexture(textureType::RENDER_BUFFER_SAMPLER, m_geometryPassRT2TextureID);
	l_geometryPassRT2TextureData->setup(textureType::RENDER_BUFFER_SAMPLER, textureColorComponentsFormat::RGBA16F, texturePixelDataFormat::RGBA, textureWrapMethod::CLAMP_TO_EDGE, textureFilterMethod::NEAREST, textureFilterMethod::NEAREST, m_screenResolution.x, m_screenResolution.y, texturePixelDataType::FLOAT, std::vector<void*>{ nullptr });

	m_geometryPassRT3TextureID = this->addTexture(textureType::RENDER_BUFFER_SAMPLER);
	auto l_geometryPassRT3TextureData = this->getTexture(textureType::RENDER_BUFFER_SAMPLER, m_geometryPassRT3TextureID);
	l_geometryPassRT3TextureData->setup(textureType::RENDER_BUFFER_SAMPLER, textureColorComponentsFormat::RGBA16F, texturePixelDataFormat::RGBA, textureWrapMethod::CLAMP_TO_EDGE, textureFilterMethod::NEAREST, textureFilterMethod::NEAREST, m_screenResolution.x, m_screenResolution.y, texturePixelDataType::FLOAT, std::vector<void*>{ nullptr });

	m_geometryPassRT4TextureID = this->addTexture(textureType::RENDER_BUFFER_SAMPLER);
	auto l_geometryPassRT4TextureData = this->getTexture(textureType::RENDER_BUFFER_SAMPLER, m_geometryPassRT4TextureID);
	l_geometryPassRT4TextureData->setup(textureType::RENDER_BUFFER_SAMPLER, textureColorComponentsFormat::RGBA16F, texturePixelDataFormat::RGBA, textureWrapMethod::CLAMP_TO_EDGE, textureFilterMethod::NEAREST, textureFilterMethod::NEAREST, m_screenResolution.x, m_screenResolution.y, texturePixelDataType::FLOAT, std::vector<void*>{ nullptr });

	// initialize frame buffer
	auto l_renderBufferStorageSizes = std::vector<vec2>{ m_screenResolution };
	auto l_renderTargetTextures = std::vector<BaseTexture*>{ l_geometryPassRT0TextureData , l_geometryPassRT1TextureData  ,l_geometryPassRT2TextureData  ,l_geometryPassRT3TextureData, l_geometryPassRT4TextureData };
	auto l_shaderPrograms = std::vector<BaseShaderProgram*>{ m_geometryPassShaderProgram };
	m_geometryPassFrameBuffer = g_pMemorySystem->spawn<FRAMEBUFFER_CLASS>();
	m_geometryPassFrameBuffer->setup(frameBufferType::FORWARD, renderBufferType::DEPTH, l_renderBufferStorageSizes, l_renderTargetTextures, l_shaderPrograms);
	m_geometryPassFrameBuffer->initialize();
}

void RenderingSystem::renderGeometryPass(std::vector<CameraComponent*>& cameraComponents, std::vector<LightComponent*>& lightComponents, std::vector<VisibleComponent*>& visibleComponents)
{
	m_geometryPassFrameBuffer->update(cameraComponents, lightComponents, visibleComponents, m_meshMap, m_textureMap, true, true);
}

void RenderingSystem::initializeLightPass()
{
	// initialize shader
	auto l_vertexShaderFilePath = "GL3.3/lightPassPBSVertex.sf";
	auto l_vertexShaderData = shaderData(shaderType::VERTEX, shaderCodeContentPair(l_vertexShaderFilePath, g_pAssetSystem->loadShader(l_vertexShaderFilePath)));
	auto l_fragmentShaderFilePath = "GL3.3/lightPassPBSFragment.sf";
	auto l_fragmentShaderData = shaderData(shaderType::FRAGMENT, shaderCodeContentPair(l_fragmentShaderFilePath, g_pAssetSystem->loadShader(l_fragmentShaderFilePath)));
	m_lightPassShaderProgram->setup({ l_vertexShaderData , l_fragmentShaderData });

	// initialize texture
	m_lightPassTextureID = this->addTexture(textureType::RENDER_BUFFER_SAMPLER);
	auto l_lightPassTextureData = this->getTexture(textureType::RENDER_BUFFER_SAMPLER, m_lightPassTextureID);
	l_lightPassTextureData->setup(textureType::RENDER_BUFFER_SAMPLER, textureColorComponentsFormat::RGBA16F, texturePixelDataFormat::RGBA, textureWrapMethod::CLAMP_TO_EDGE, textureFilterMethod::NEAREST, textureFilterMethod::NEAREST, m_screenResolution.x, m_screenResolution.y, texturePixelDataType::FLOAT, std::vector<void*>{ nullptr });

	// initialize frame buffer
	auto l_renderBufferStorageSizes = std::vector<vec2>{ m_screenResolution };
	auto l_renderTargetTextures = std::vector<BaseTexture*>{ l_lightPassTextureData };
	auto l_shaderPrograms = std::vector<BaseShaderProgram*>{ m_lightPassShaderProgram };
	m_lightPassFrameBuffer = g_pMemorySystem->spawn<FRAMEBUFFER_CLASS>();
	m_lightPassFrameBuffer->setup(frameBufferType::DEFER, renderBufferType::NONE, l_renderBufferStorageSizes, l_renderTargetTextures, l_shaderPrograms);
	m_lightPassFrameBuffer->initialize();
}

void RenderingSystem::renderLightPass(std::vector<CameraComponent*>& cameraComponents, std::vector<LightComponent*>& lightComponents, std::vector<VisibleComponent*>& visibleComponents)
{
	// world space position 
	m_geometryPassFrameBuffer->activeTexture(0, 0);
	// normal
	m_geometryPassFrameBuffer->activeTexture(1, 1);
	// albedo
	m_geometryPassFrameBuffer->activeTexture(2, 2);
	// metallic, roughness and ambient occlusion
	m_geometryPassFrameBuffer->activeTexture(3, 3);
	// light space position
	m_geometryPassFrameBuffer->activeTexture(4, 4);
	// shadow map
	m_shadowForwardPassFrameBuffer->activeTexture(0, 5);
	// irradiance environment map
	m_environmentPassFrameBuffer->activeTexture(1, 6);
	// pre-filter specular environment map
	m_environmentPassFrameBuffer->activeTexture(2, 7);
	// BRDF look-up table
	m_environmentPassFrameBuffer->activeTexture(3, 8);

	m_lightPassFrameBuffer->update(cameraComponents, lightComponents, visibleComponents, m_meshMap, m_textureMap, true, true);

	// draw light pass rectangle
	this->getMesh(meshType::TWO_DIMENSION, m_Unit2DQuadTemplate)->update();
}

void RenderingSystem::initializeFinalPass()
{
	// sky forward pass
	// initialize shader
	auto l_skyForwardPassVertexShaderFilePath = "GL3.3/skyForwardPassPBSVertex.sf";
	auto l_skyForwardPassVertexShaderData = shaderData(shaderType::VERTEX, shaderCodeContentPair(l_skyForwardPassVertexShaderFilePath, g_pAssetSystem->loadShader(l_skyForwardPassVertexShaderFilePath)));
	auto l_skyForwardPassFragmentShaderFilePath = "GL3.3/skyForwardPassPBSFragment.sf";
	auto l_skyForwardPassFragmentShaderData = shaderData(shaderType::FRAGMENT, shaderCodeContentPair(l_skyForwardPassFragmentShaderFilePath, g_pAssetSystem->loadShader(l_skyForwardPassFragmentShaderFilePath)));
	m_skyForwardPassShaderProgram->setup({ l_skyForwardPassVertexShaderData , l_skyForwardPassFragmentShaderData });

	// initialize texture
	m_skyForwardPassTextureID = this->addTexture(textureType::RENDER_BUFFER_SAMPLER);
	auto l_skyForwardPassTextureData = this->getTexture(textureType::RENDER_BUFFER_SAMPLER, m_skyForwardPassTextureID);
	l_skyForwardPassTextureData->setup(textureType::RENDER_BUFFER_SAMPLER, textureColorComponentsFormat::RGBA16F, texturePixelDataFormat::RGBA, textureWrapMethod::CLAMP_TO_EDGE, textureFilterMethod::NEAREST, textureFilterMethod::NEAREST, m_screenResolution.x, m_screenResolution.y, texturePixelDataType::FLOAT, std::vector<void*>{ nullptr });

	// initialize framebuffer
	auto l_skyForwardPassRenderBufferStorageSizes = std::vector<vec2>{ m_screenResolution };
	auto l_skyForwardPassRenderTargetTextures = std::vector<BaseTexture*>{ l_skyForwardPassTextureData };
	auto l_skyForwardPassShaderPrograms = std::vector<BaseShaderProgram*>{ m_skyForwardPassShaderProgram };
	m_skyForwardPassFrameBuffer = g_pMemorySystem->spawn<FRAMEBUFFER_CLASS>();
	m_skyForwardPassFrameBuffer->setup(frameBufferType::FORWARD, renderBufferType::NONE, l_skyForwardPassRenderBufferStorageSizes, l_skyForwardPassRenderTargetTextures, l_skyForwardPassShaderPrograms);
	m_skyForwardPassFrameBuffer->initialize();

	// debugger pass
	// initialize shader
	auto l_debuggerPassVertexShaderFilePath = "GL3.3/debuggerVertex.sf";
	auto l_debuggerPassVertexShaderData = shaderData(shaderType::VERTEX, shaderCodeContentPair(l_debuggerPassVertexShaderFilePath, g_pAssetSystem->loadShader(l_debuggerPassVertexShaderFilePath)));
	//auto l_debuggerPassGeometryShaderFilePath = "GL3.3/debuggerGeometry.sf";
	//auto l_debuggerPassGeometryShaderData = shaderData(shaderType::GEOMETRY, shaderCodeContentPair(l_debuggerPassGeometryShaderFilePath, g_pAssetSystem->loadShader(l_debuggerPassGeometryShaderFilePath)));
	auto l_debuggerPassFragmentShaderFilePath = "GL3.3/debuggerFragment.sf";
	auto l_debuggerPassFragmentShaderData = shaderData(shaderType::FRAGMENT, shaderCodeContentPair(l_debuggerPassFragmentShaderFilePath, g_pAssetSystem->loadShader(l_debuggerPassFragmentShaderFilePath)));
	//m_debuggerPassShaderProgram->setup({ l_debuggerPassVertexShaderData , l_debuggerPassGeometryShaderData , l_debuggerPassFragmentShaderData });
	m_debuggerPassShaderProgram->setup({ l_debuggerPassVertexShaderData , l_debuggerPassFragmentShaderData });

	// initialize texture
	m_debuggerPassTextureID = this->addTexture(textureType::RENDER_BUFFER_SAMPLER);
	auto l_debuggerPassTextureData = this->getTexture(textureType::RENDER_BUFFER_SAMPLER, m_debuggerPassTextureID);
	l_debuggerPassTextureData->setup(textureType::RENDER_BUFFER_SAMPLER, textureColorComponentsFormat::RGBA16F, texturePixelDataFormat::RGBA, textureWrapMethod::CLAMP_TO_EDGE, textureFilterMethod::NEAREST, textureFilterMethod::NEAREST, m_screenResolution.x, m_screenResolution.y, texturePixelDataType::FLOAT, std::vector<void*>{ nullptr });

	// initialize framebuffer
	auto l_debuggerPassRenderBufferStorageSizes = std::vector<vec2>{ m_screenResolution };
	auto l_debuggerPassRenderTargetTextures = std::vector<BaseTexture*>{ l_debuggerPassTextureData };
	auto l_debuggerPassShaderPrograms = std::vector<BaseShaderProgram*>{ m_debuggerPassShaderProgram };
	m_debuggerPassFrameBuffer = g_pMemorySystem->spawn<FRAMEBUFFER_CLASS>();
	m_debuggerPassFrameBuffer->setup(frameBufferType::DEFER, renderBufferType::DEPTH, l_debuggerPassRenderBufferStorageSizes, l_debuggerPassRenderTargetTextures, l_debuggerPassShaderPrograms);
	m_debuggerPassFrameBuffer->initialize();

	// sky defer pass
	// initialize shader
	auto l_skyDeferPassVertexShaderFilePath = "GL3.3/skyDeferPassPBSVertex.sf";
	auto l_skyDeferPassVertexShaderData = shaderData(shaderType::VERTEX, shaderCodeContentPair(l_skyDeferPassVertexShaderFilePath, g_pAssetSystem->loadShader(l_skyDeferPassVertexShaderFilePath)));
	auto l_skyDeferPassFragmentShaderFilePath = "GL3.3/skyDeferPassPBSFragment.sf";
	auto l_skyDeferPassFragmentShaderData = shaderData(shaderType::FRAGMENT, shaderCodeContentPair(l_skyDeferPassFragmentShaderFilePath, g_pAssetSystem->loadShader(l_skyDeferPassFragmentShaderFilePath)));
	m_skyDeferPassShaderProgram->setup({ l_skyDeferPassVertexShaderData , l_skyDeferPassFragmentShaderData });

	// initialize texture
	m_skyDeferPassTextureID = this->addTexture(textureType::RENDER_BUFFER_SAMPLER);
	auto l_skyDeferPassTextureData = this->getTexture(textureType::RENDER_BUFFER_SAMPLER, m_skyDeferPassTextureID);
	l_skyDeferPassTextureData->setup(textureType::RENDER_BUFFER_SAMPLER, textureColorComponentsFormat::RGBA16F, texturePixelDataFormat::RGBA, textureWrapMethod::CLAMP_TO_EDGE, textureFilterMethod::NEAREST, textureFilterMethod::NEAREST, m_screenResolution.x, m_screenResolution.y, texturePixelDataType::FLOAT, std::vector<void*>{ nullptr });

	// initialize framebuffer
	auto l_skyDeferPassRenderBufferStorageSizes = std::vector<vec2>{ m_screenResolution };
	auto l_skyDeferPassRenderTargetTextures = std::vector<BaseTexture*>{ l_skyDeferPassTextureData };
	auto l_skyDeferPassRhaderPrograms = std::vector<BaseShaderProgram*>{ m_skyDeferPassShaderProgram };
	m_skyDeferPassFrameBuffer = g_pMemorySystem->spawn<FRAMEBUFFER_CLASS>();
	m_skyDeferPassFrameBuffer->setup(frameBufferType::DEFER, renderBufferType::NONE, l_skyDeferPassRenderBufferStorageSizes, l_skyDeferPassRenderTargetTextures, l_skyDeferPassRhaderPrograms);
	m_skyDeferPassFrameBuffer->initialize();

	// billboard pass
	// initialize shader
	auto l_billboardPassVertexShaderFilePath = "GL3.3/billboardPassVertex.sf";
	auto l_billboardPassVertexShaderData = shaderData(shaderType::VERTEX, shaderCodeContentPair(l_billboardPassVertexShaderFilePath, g_pAssetSystem->loadShader(l_billboardPassVertexShaderFilePath)));
	auto l_billboardPassFragmentShaderFilePath = "GL3.3/billboardPassFragment.sf";
	auto l_billboardPassFragmentShaderData = shaderData(shaderType::FRAGMENT, shaderCodeContentPair(l_billboardPassFragmentShaderFilePath, g_pAssetSystem->loadShader(l_billboardPassFragmentShaderFilePath)));
	m_billboardPassShaderProgram->setup({ l_billboardPassVertexShaderData , l_billboardPassFragmentShaderData });

	// initialize texture
	m_billboardPassTextureID = this->addTexture(textureType::RENDER_BUFFER_SAMPLER);
	auto l_billboardPassTextureData = this->getTexture(textureType::RENDER_BUFFER_SAMPLER, m_billboardPassTextureID);
	l_billboardPassTextureData->setup(textureType::RENDER_BUFFER_SAMPLER, textureColorComponentsFormat::RGBA16F, texturePixelDataFormat::RGBA, textureWrapMethod::CLAMP_TO_EDGE, textureFilterMethod::NEAREST, textureFilterMethod::NEAREST, m_screenResolution.x, m_screenResolution.y, texturePixelDataType::FLOAT, std::vector<void*>{ nullptr });

	// initialize framebuffer
	auto l_billboardPassRenderBufferStorageSizes = std::vector<vec2>{ m_screenResolution };
	auto l_billboardPassRenderTargetTextures = std::vector<BaseTexture*>{ l_billboardPassTextureData };
	auto l_billboardPassRhaderPrograms = std::vector<BaseShaderProgram*>{ m_billboardPassShaderProgram };
	m_billboardPassFrameBuffer = g_pMemorySystem->spawn<FRAMEBUFFER_CLASS>();
	m_billboardPassFrameBuffer->setup(frameBufferType::FORWARD, renderBufferType::DEPTH, l_billboardPassRenderBufferStorageSizes, l_billboardPassRenderTargetTextures, l_billboardPassRhaderPrograms);
	m_billboardPassFrameBuffer->initialize();

	//post-process pass
	// initialize shader
	auto l_vertexShaderFilePath = "GL3.3/finalPassVertex.sf";
	auto l_vertexShaderData = shaderData(shaderType::VERTEX, shaderCodeContentPair(l_vertexShaderFilePath, g_pAssetSystem->loadShader(l_vertexShaderFilePath)));
	auto l_fragmentShaderFilePath = "GL3.3/finalPassFragment.sf";
	auto l_fragmentShaderData = shaderData(shaderType::FRAGMENT, shaderCodeContentPair(l_fragmentShaderFilePath, g_pAssetSystem->loadShader(l_fragmentShaderFilePath)));
	m_finalPassShaderProgram->setup({ l_vertexShaderData , l_fragmentShaderData });
	m_finalPassShaderProgram->initialize();
}

void RenderingSystem::renderFinalPass(std::vector<CameraComponent*>& cameraComponents, std::vector<LightComponent*>& lightComponents, std::vector<VisibleComponent*>& visibleComponents)
{
	// draw sky forward pass
	m_skyForwardPassFrameBuffer->update(cameraComponents, lightComponents, visibleComponents, m_meshMap, m_textureMap, true, true);

	// draw debugger pass
	m_geometryPassFrameBuffer->asReadBuffer();
	m_debuggerPassFrameBuffer->asWriteBuffer(m_screenResolution, m_screenResolution);
	//m_debuggerPassFrameBuffer->update(cameraComponents, lightComponents, m_selectedVisibleComponents, m_AABBMeshMap, m_textureMap, true, false);
	m_debuggerPassFrameBuffer->update(cameraComponents, lightComponents, visibleComponents, m_AABBMeshMap, m_textureMap, true, false);

	// draw background defer pass
	m_lightPassFrameBuffer->activeTexture(0, 0);
	m_skyForwardPassFrameBuffer->activeTexture(0, 1);
	m_debuggerPassFrameBuffer->activeTexture(0, 2);

	m_skyDeferPassFrameBuffer->update(cameraComponents, lightComponents, visibleComponents, m_meshMap, m_textureMap, true, true);

	// draw sky defer pass rectangle
	this->getMesh(meshType::TWO_DIMENSION, m_Unit2DQuadTemplate)->update();

	// draw billboard pass
	m_geometryPassFrameBuffer->asReadBuffer();
	m_billboardPassFrameBuffer->asWriteBuffer(m_screenResolution, m_screenResolution);
	m_billboardPassFrameBuffer->update(cameraComponents, lightComponents, visibleComponents, m_meshMap, m_textureMap, true, false);

	// draw final pass
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);

	m_skyDeferPassFrameBuffer->activeTexture(0, 0);
	m_billboardPassFrameBuffer->activeTexture(0, 1);

	m_finalPassShaderProgram->update(cameraComponents, lightComponents, visibleComponents, m_meshMap, m_textureMap);

	// draw final pass rectangle
	this->getMesh(meshType::TWO_DIMENSION, m_Unit2DQuadTemplate)->update();
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
