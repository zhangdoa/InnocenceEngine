#include "GLDebuggerPass.h"

#include "GLOpaquePass.h"

#include "GLRenderingSystemUtilities.h"
#include "../../component/GLRenderingSystemComponent.h"

using namespace GLRenderingSystemNS;

#include "../ICoreSystem.h"

extern ICoreSystem* g_pCoreSystem;

INNO_PRIVATE_SCOPE GLDebuggerPass
{
	void initializeShaders();

	EntityID m_entityID;

	std::function<void()> f_mouseSelect;
	unsigned int m_pickedID;
	VisibleComponent* m_pickedVisibleComponent;

	GLRenderPassComponent* m_GLRPC;
	GLShaderProgramComponent* m_GLSPC;
	ShaderFilePaths m_shaderFilePaths = { "GL//wireframeOverlayPassVertex.sf", "", "GL//wireframeOverlayPassFragment.sf" };
	//ShaderFilePaths m_shaderFilePaths = { "GL//debuggerPassVertex.sf", "GL//debuggerPassGeometry.sf", "GL//debuggerPassFragment.sf" };

	CameraDataPack m_cameraDataPack;
}

bool GLDebuggerPass::initialize()
{
	f_mouseSelect = [&]() {
		auto l_mousePos = g_pCoreSystem->getInputSystem()->getMousePositionInScreenSpace();
		l_mousePos.y = g_pCoreSystem->getVisionSystem()->getRenderingFrontend()->getScreenResolution().y - l_mousePos.y;

		auto l_pixelValue = readPixel(GLOpaquePass::getGLRPC(), 3, (GLint)l_mousePos.x, (GLint)l_mousePos.y);

		m_pickedID = (unsigned int)l_pixelValue.z;

		auto l_visibleCompoents = g_pCoreSystem->getGameSystem()->get<VisibleComponent>();
		auto l_findResult =
			std::find_if(l_visibleCompoents.begin(), l_visibleCompoents.end(), [&](auto& val) -> bool {
			return val->m_UUID == m_pickedID;
		}
		);
		if (l_findResult != l_visibleCompoents.end())
		{
			m_pickedVisibleComponent = *l_findResult;
		}
	};

	g_pCoreSystem->getInputSystem()->addButtonStatusCallback(ButtonData{ INNO_MOUSE_BUTTON_LEFT, ButtonStatus::PRESSED }, &f_mouseSelect);

	m_entityID = InnoMath::createEntityID();

	m_GLRPC = addGLRenderPassComponent(1, GLRenderingSystemComponent::get().deferredPassFBDesc, GLRenderingSystemComponent::get().deferredPassTextureDesc);

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

bool GLDebuggerPass::update()
{
	// copy camera data pack for local scope
	auto l_cameraDataPack = g_pCoreSystem->getVisionSystem()->getRenderingFrontend()->getCameraDataPack();
	if (l_cameraDataPack.has_value())
	{
		m_cameraDataPack = l_cameraDataPack.value();
	}

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);

	activateRenderPass(m_GLRPC);

	// copy depth buffer from G-Pass
	copyDepthBuffer(GLOpaquePass::getGLRPC(), m_GLRPC);

	activateShaderProgram(m_GLSPC);

	updateUniform(
		0,
		m_cameraDataPack.p_original);
	updateUniform(
		1,
		m_cameraDataPack.r);
	updateUniform(
		2,
		m_cameraDataPack.t);

	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	while (GLRenderingSystemComponent::get().m_debuggerPassDataQueue.size() > 0)
	{
		auto l_renderPack = GLRenderingSystemComponent::get().m_debuggerPassDataQueue.front();

		auto l_m = l_renderPack.m;

		updateUniform(
			3,
			l_m);

		drawMesh(l_renderPack.indiceSize, l_renderPack.meshPrimitiveTopology, l_renderPack.GLMDC);

		GLRenderingSystemComponent::get().m_debuggerPassDataQueue.pop();
	}

	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glDisable(GL_DEPTH_TEST);

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

GLRenderPassComponent * GLDebuggerPass::getGLRPC()
{
	return m_GLRPC;
}