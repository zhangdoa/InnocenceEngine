#include "GLDebuggerPass.h"
#include "../../Common/CommonMacro.inl"
#include "../../ComponentManager/ITransformComponentManager.h"
#include "../../ComponentManager/IVisibleComponentManager.h"
#include "../../ComponentManager/IDirectionalLightComponentManager.h"

#include "GLOpaquePass.h"

#include "GLRenderingBackendUtilities.h"
#include "../../Component/GLRenderingBackendComponent.h"

using namespace GLRenderingBackendNS;

#include "../../ModuleManager/IModuleManager.h"

extern IModuleManager* g_pModuleManager;

INNO_PRIVATE_SCOPE GLDebuggerPass
{
	void initializeShaders();

	bool drawCoordinateAxis();
	bool drawMainCamera();
	bool drawDebugObjects();

	bool drawRightView();
	bool drawTopView();
	bool drawFrontView();

	EntityID m_entityID;

	std::function<void()> f_mouseSelect;
	unsigned int m_pickedID;
	VisibleComponent* m_pickedVisibleComponent;

	GLRenderPassComponent* m_GLRPC;
	GLRenderPassComponent* m_rightViewGLRPC;
	GLRenderPassComponent* m_topViewGLRPC;
	GLRenderPassComponent* m_frontViewGLRPC;

	GLShaderProgramComponent* m_GLSPC;
	ShaderFilePaths m_shaderFilePaths = { "GL//wireframeOverlayPass.vert/", "", "", "", "GL//wireframeOverlayPass.frag/" };
	//ShaderFilePaths m_shaderFilePaths = { "GL//debuggerPass.vert/", "", "", "GL//debuggerPass.geom/", "GL//debuggerPass.frag/" };
}

bool GLDebuggerPass::initialize()
{
	f_mouseSelect = [&]() {
		auto l_mousePos = g_pModuleManager->getInputSystem()->getMousePositionInScreenSpace();
		l_mousePos.y = g_pModuleManager->getRenderingFrontend()->getScreenResolution().y - l_mousePos.y;

		auto l_pixelValue = readPixel(GLOpaquePass::getGLRPC(), 3, (GLint)l_mousePos.x, (GLint)l_mousePos.y);

		m_pickedID = (unsigned int)l_pixelValue.z;

		auto l_visibleComponents = GetComponentManager(VisibleComponent)->GetAllComponents();
		auto l_findResult =
			std::find_if(l_visibleComponents.begin(), l_visibleComponents.end(), [&](auto& val) -> bool {
			return val->m_UUID == m_pickedID;
		}
		);
		if (l_findResult != l_visibleComponents.end())
		{
			m_pickedVisibleComponent = *l_findResult;
		}
	};

	g_pModuleManager->getInputSystem()->addButtonStatusCallback(ButtonData{ INNO_MOUSE_BUTTON_LEFT, ButtonStatus::PRESSED }, &f_mouseSelect);

	m_entityID = InnoMath::createEntityID();

	auto l_renderPassDesc = GLRenderingBackendComponent::get().m_deferredRenderPassDesc;
	m_GLRPC = addGLRenderPassComponent(m_entityID, "DebugPassGLRPC/");
	m_GLRPC->m_renderPassDesc = l_renderPassDesc;
	m_GLRPC->m_renderPassDesc.useDepthAttachment = true;
	m_GLRPC->m_renderPassDesc.useStencilAttachment = true;
	initializeGLRenderPassComponent(m_GLRPC);

	l_renderPassDesc.RTDesc.width /= 2;
	l_renderPassDesc.RTDesc.height /= 2;

	m_rightViewGLRPC = addGLRenderPassComponent(m_entityID, "DebugRightViewPassGLRPC/");
	m_rightViewGLRPC->m_renderPassDesc = l_renderPassDesc;
	initializeGLRenderPassComponent(m_rightViewGLRPC);

	m_topViewGLRPC = addGLRenderPassComponent(m_entityID, "DebugTopViewPassGLRPC/");
	m_topViewGLRPC->m_renderPassDesc = l_renderPassDesc;
	initializeGLRenderPassComponent(m_topViewGLRPC);

	m_frontViewGLRPC = addGLRenderPassComponent(m_entityID, "DebugFrontViewPassGLRPC/");
	m_frontViewGLRPC->m_renderPassDesc = l_renderPassDesc;
	initializeGLRenderPassComponent(m_frontViewGLRPC);

	initializeShaders();

	return true;
}

void GLDebuggerPass::initializeShaders()
{
	// shader programs and shaders
	auto rhs = addGLShaderProgramComponent(m_entityID);

	initializeGLShaderProgramComponent(rhs, m_shaderFilePaths);

	m_GLSPC = rhs;
}

bool GLDebuggerPass::drawCoordinateAxis()
{
	auto l_MDC = getGLMeshDataComponent(MeshShapeType::CUBE);

	auto l_p = g_pModuleManager->getRenderingFrontend()->getCameraGPUData().p_original;
	auto l_r = g_pModuleManager->getRenderingFrontend()->getCameraGPUData().r;

	auto l_pos = vec4(-70.0f, -60.0f, -1.0f, 1.0f);
	l_pos = InnoMath::clipToViewSpace(l_pos, l_p);
	l_pos.z = 0.0f;
	l_pos.w = 1.0f;

	auto l_tX = InnoMath::toTranslationMatrix(l_pos);
	auto l_tY = InnoMath::toTranslationMatrix(l_pos);
	auto l_tZ = InnoMath::toTranslationMatrix(l_pos);

	auto l_sX = InnoMath::toScaleMatrix(vec4(0.5f, 0.02f, 0.02f, 1.0f));
	auto l_sY = InnoMath::toScaleMatrix(vec4(0.02f, 0.5f, 0.02f, 1.0f));
	auto l_sZ = InnoMath::toScaleMatrix(vec4(0.02f, 0.02f, 0.5f, 1.0f));

	auto l_mX = l_tX * l_r *l_sX;
	auto l_mY = l_tY * l_r *l_sY;
	auto l_mZ = l_tZ * l_r *l_sZ;

	auto l_camPos = vec4(0.0f, 0.0f, 8.0f, 1.0f);

	auto l_cam_r = InnoMath::generateIdentityMatrix<float>();
	auto l_cam_t = InnoMath::toTranslationMatrix(l_camPos).inverse();

	updateUniform(0, l_p);
	updateUniform(1, l_cam_r);
	updateUniform(2, l_cam_t);

	updateUniform(3, l_mX);
	updateUniform(4, vec4(1.0f, 0.0f, 0.0f, 1.0f));
	drawMesh(l_MDC);

	updateUniform(3, l_mY);
	updateUniform(4, vec4(0.0f, 1.0f, 0.0f, 1.0f));
	drawMesh(l_MDC);

	updateUniform(3, l_mZ);
	updateUniform(4, vec4(0.0f, 0.0f, 1.0f, 1.0f));
	drawMesh(l_MDC);

	return true;
}

bool GLDebuggerPass::drawMainCamera()
{
	auto l_MDC = getGLMeshDataComponent(MeshShapeType::SPHERE);

	auto l_r = InnoMath::generateIdentityMatrix<float>();
	auto l_pos = g_pModuleManager->getRenderingFrontend()->getCameraGPUData().globalPos;
	auto l_t = InnoMath::toTranslationMatrix(l_pos);
	auto l_s = InnoMath::toScaleMatrix(vec4(12.8f, 12.8f, 12.8f, 1.0f));
	auto l_m = l_r * l_t * l_s;

	updateUniform(3, l_m);
	// albedo
	updateUniform(4, vec4(0.6f, 0.6f, 0.1f, 1.0f));

	drawMesh(l_MDC);

	return true;
}

bool GLDebuggerPass::drawDebugObjects()
{
	auto l_MDC = getGLMeshDataComponent(MeshShapeType::SPHERE);

	static bool l_drawPointLightRange = false;

	if (l_drawPointLightRange)
	{
		for (auto i : g_pModuleManager->getRenderingFrontend()->getPointLightGPUData())
		{
			if (i.luminance.w > 0.0f)
			{
				auto l_t = InnoMath::toTranslationMatrix(i.pos);
				auto l_s = InnoMath::toScaleMatrix(vec4(i.luminance.w, i.luminance.w, i.luminance.w, 1.0f));
				auto l_m = l_t * l_s;
				updateUniform(3, l_m);
				drawMesh(l_MDC);
			}
		}
	}

	static bool l_drawSphereLightShape = false;

	if (l_drawSphereLightShape)
	{
		for (auto i : g_pModuleManager->getRenderingFrontend()->getSphereLightGPUData())
		{
			if (i.luminance.w > 0.0f)
			{
				auto l_t = InnoMath::toTranslationMatrix(i.pos);
				auto l_s = InnoMath::toScaleMatrix(vec4(i.luminance.w, i.luminance.w, i.luminance.w, 1.0f));
				auto l_m = l_t * l_s;
				updateUniform(3, l_m);
				drawMesh(l_MDC);
			}
		}
	}

	static bool l_drawCSMAABBRange = false;
	l_MDC = getGLMeshDataComponent(MeshShapeType::CUBE);

	if (l_drawCSMAABBRange)
	{
		auto l_directionalLightComponents = GetComponentManager(DirectionalLightComponent)->GetAllComponents();
		auto l_directionalLight = l_directionalLightComponents[0];

		for (auto i : l_directionalLight->m_AABBsInWorldSpace)
		{
			auto l_t = InnoMath::toTranslationMatrix(i.m_center);
			auto l_extend = i.m_extend;
			auto l_s = InnoMath::toScaleMatrix(l_extend);
			auto l_m = l_t * l_s;
			updateUniform(3, l_m);
			drawMesh(l_MDC);
		}
	}

	static bool l_drawSkeleton = true;
	if (l_drawSkeleton)
	{
		auto l_visibleComponents = GetComponentManager(VisibleComponent)->GetAllComponents();

		for (auto i : l_visibleComponents)
		{
			if (i->m_meshUsageType == MeshUsageType::SKELETAL)
			{
				auto l_transformComponent = GetComponent(TransformComponent, i->m_parentEntity);
				auto l_m = l_transformComponent->m_globalTransformMatrix.m_transformationMat;
				for (auto j : i->m_modelMap)
				{
					auto l_SDC = j.first->m_SDC;
					auto l_rootOffsetMat = l_SDC->m_RootOffsetMatrix;

					for (auto k : l_SDC->m_Bones)
					{
						auto l_t = InnoMath::toTranslationMatrix(k.m_Pos);
						auto l_r = InnoMath::toRotationMatrix(k.m_Rot);
						auto l_bm = l_t * l_r;
						// Inverse-Joint-Matrix
						l_bm = l_bm.inverse();
						auto l_s = InnoMath::toScaleMatrix(vec4(0.01f, 0.01f, 0.01f, 1.0f));
						l_bm = l_bm * l_s;
						l_bm = l_m * l_bm;
						updateUniform(3, l_bm);
						drawMesh(reinterpret_cast<GLMeshDataComponent*>(l_MDC));
					}
				}
			}
		}
	}

	static bool l_drawDebugMesh = false;

	if (l_drawDebugMesh)
	{
		for (unsigned int i = 0; i < g_pModuleManager->getRenderingFrontend()->getDebuggerPassDrawCallCount(); i++)
		{
			auto l_debuggerPassGPUData = g_pModuleManager->getRenderingFrontend()->getDebuggerPassGPUData()[i];
			updateUniform(3, l_debuggerPassGPUData.m);
			drawMesh(reinterpret_cast<GLMeshDataComponent*>(l_debuggerPassGPUData.MDC));
		}
	}

	if (m_pickedVisibleComponent)
	{
		auto l_transformComponent = GetComponent(TransformComponent, m_pickedVisibleComponent->m_parentEntity);

		for (auto i : m_pickedVisibleComponent->m_modelMap)
		{
			updateUniform(3, l_transformComponent->m_globalTransformMatrix.m_transformationMat);
			drawMesh(reinterpret_cast<GLMeshDataComponent*>(i.first));
		}
	}

	return true;
}

bool GLDebuggerPass::drawRightView()
{
	auto l_p = g_pModuleManager->getRenderingFrontend()->getCameraGPUData().p_original;
	auto l_cam_r = InnoMath::toRotationMatrix(InnoMath::getQuatRotator(vec4(0.0f, 1.0f, 0.0f, 0.0f), 90.0f)).inverse();
	auto l_pos = vec4(1024.0f, 0.0f, 0.0f, 1.0f);
	auto l_cam_t = InnoMath::toTranslationMatrix(l_pos).inverse();

	updateUniform(0, l_p);
	updateUniform(1, l_cam_r);
	updateUniform(2, l_cam_t);
	// albedo
	updateUniform(4, vec4(1.0f, 1.0f, 1.0f, 1.0f));

	activateRenderPass(m_rightViewGLRPC);

	drawDebugObjects();

	drawMainCamera();

	return true;
}

bool GLDebuggerPass::drawTopView()
{
	auto l_p = g_pModuleManager->getRenderingFrontend()->getCameraGPUData().p_original;
	auto l_qX = InnoMath::getQuatRotator(vec4(1.0f, 0.0f, 0.0f, 0.0f), -90.0f);
	auto l_qY = InnoMath::getQuatRotator(vec4(0.0f, 1.0f, 0.0f, 0.0f), 0.0f);
	auto l_cam_r = InnoMath::toRotationMatrix(l_qX.quatMul(l_qY)).inverse();
	auto l_pos = vec4(0.0f, 1024.0f, 0.0f, 1.0f);
	auto l_cam_t = InnoMath::toTranslationMatrix(l_pos).inverse();

	updateUniform(0, l_p);
	updateUniform(1, l_cam_r);
	updateUniform(2, l_cam_t);
	// albedo
	updateUniform(4, vec4(1.0f, 1.0f, 1.0f, 1.0f));

	activateRenderPass(m_topViewGLRPC);

	drawDebugObjects();

	drawMainCamera();

	return true;
}

bool GLDebuggerPass::drawFrontView()
{
	auto l_p = g_pModuleManager->getRenderingFrontend()->getCameraGPUData().p_original;
	auto l_cam_r = InnoMath::toRotationMatrix(InnoMath::getQuatRotator(vec4(1.0f, 0.0f, 0.0f, 0.0f), 0.0f)).inverse();
	auto l_pos = vec4(0.0f, 0.0f, 1024.0f, 1.0f);
	auto l_cam_t = InnoMath::toTranslationMatrix(l_pos).inverse();

	updateUniform(0, l_p);
	updateUniform(1, l_cam_r);
	updateUniform(2, l_cam_t);
	// albedo
	updateUniform(4, vec4(1.0f, 1.0f, 1.0f, 1.0f));

	activateRenderPass(m_frontViewGLRPC);

	drawDebugObjects();

	drawMainCamera();

	return true;
}

bool GLDebuggerPass::update()
{
	activateShaderProgram(m_GLSPC);

	activateRenderPass(m_GLRPC);

	drawCoordinateAxis();

	//glEnable(GL_DEPTH_TEST);
	//glDepthFunc(GL_LESS);

	// copy depth buffer from G-Pass
	//copyDepthBuffer(GLOpaquePass::getGLRPC(), m_GLRPC);

	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	updateUniform(0, g_pModuleManager->getRenderingFrontend()->getCameraGPUData().p_original);
	updateUniform(1, g_pModuleManager->getRenderingFrontend()->getCameraGPUData().r);
	updateUniform(2, g_pModuleManager->getRenderingFrontend()->getCameraGPUData().t);

	// albedo
	updateUniform(4, vec4(0.5f, 0.2f, 0.1f, 1.0f));

	drawDebugObjects();

	glDisable(GL_DEPTH_TEST);

	drawRightView();
	drawTopView();
	drawFrontView();

	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	return true;
}

bool GLDebuggerPass::resize(unsigned int newSizeX, unsigned int newSizeY)
{
	resizeGLRenderPassComponent(m_GLRPC, newSizeX, newSizeY);

	return true;
}

bool GLDebuggerPass::reloadShader()
{
	deleteShaderProgram(m_GLSPC);

	initializeGLShaderProgramComponent(m_GLSPC, m_shaderFilePaths);

	return true;
}

GLRenderPassComponent * GLDebuggerPass::getGLRPC(unsigned int index)
{
	if (index == 0)
	{
		return m_GLRPC;
	}
	else if (index == 1)
	{
		return m_rightViewGLRPC;
	}
	else if (index == 2)
	{
		return m_topViewGLRPC;
	}
	else
	{
		return m_frontViewGLRPC;
	}
}