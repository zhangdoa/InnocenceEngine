#include "GLBillboardPass.h"

#include "GLOpaquePass.h"

#include "GLRenderingSystemUtilities.h"
#include "../../component/GLRenderingSystemComponent.h"

using namespace GLRenderingSystemNS;

#include "../ICoreSystem.h"

extern ICoreSystem* g_pCoreSystem;

INNO_PRIVATE_SCOPE GLBillboardPass
{
	void initializeShaders();

	EntityID m_entityID;

	GLRenderPassComponent* m_GLRPC;
	GLShaderProgramComponent* m_GLSPC;
	ShaderFilePaths m_shaderFilePaths = { "GL//billboardPass.vert", "", "GL//billboardPass.frag" };

	CameraDataPack m_cameraDataPack;
}

bool GLBillboardPass::initialize()
{
	m_entityID = InnoMath::createEntityID();

	m_GLRPC = addGLRenderPassComponent(1, GLRenderingSystemComponent::get().deferredPassFBDesc, GLRenderingSystemComponent::get().deferredPassTextureDesc);

	initializeShaders();

	return true;
}

void GLBillboardPass::initializeShaders()
{
	// shader programs and shaders
	auto rhs = addGLShaderProgramComponent(m_entityID);

	initializeGLShaderProgramComponent(rhs, m_shaderFilePaths);

	m_GLSPC = rhs;
}

bool GLBillboardPass::update()
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

	while (GLRenderingSystemComponent::get().m_billboardPassDataQueue.size() > 0)
	{
		auto l_renderPack = GLRenderingSystemComponent::get().m_billboardPassDataQueue.front();

		auto l_GlobalPos = l_renderPack.globalPos;

		updateUniform(
			3,
			l_GlobalPos.x, l_GlobalPos.y, l_GlobalPos.z);

		auto l_distanceToCamera = l_renderPack.distanceToCamera;

		if (l_distanceToCamera > 1.0f)
		{
			updateUniform(
				4,
				(1.0f / (l_distanceToCamera * m_cameraDataPack.WHRatio)), (1.0f / l_distanceToCamera));
		}
		else
		{
			updateUniform(
				4,
				(1.0f / m_cameraDataPack.WHRatio), 1.0f);
		}

		GLTextureDataComponent* l_iconTexture = 0;

		switch (l_renderPack.iconType)
		{
		case WorldEditorIconType::DIRECTIONAL_LIGHT: l_iconTexture = GLRenderingSystemComponent::get().m_iconTemplate_DirectionalLight; break;
		case WorldEditorIconType::POINT_LIGHT: l_iconTexture = GLRenderingSystemComponent::get().m_iconTemplate_PointLight; break;
		case WorldEditorIconType::SPHERE_LIGHT: l_iconTexture = GLRenderingSystemComponent::get().m_iconTemplate_SphereLight; break;
		default:
			break;
		}

		activateTexture(l_iconTexture, 0);

		drawMesh(6, MeshPrimitiveTopology::TRIANGLE_STRIP, GLRenderingSystemComponent::get().m_UnitQuadGLMDC);

		GLRenderingSystemComponent::get().m_billboardPassDataQueue.pop();
	}

	glDisable(GL_DEPTH_TEST);

	return true;
}

bool GLBillboardPass::resize(unsigned int newSizeX, unsigned int newSizeY)
{
	resizeGLRenderPassComponent(m_GLRPC, newSizeX, newSizeY);

	return true;
}

bool GLBillboardPass::reloadShader()
{
	deleteShaderProgram(m_GLSPC);

	initializeGLShaderProgramComponent(m_GLSPC, m_shaderFilePaths);

	return true;
}

GLRenderPassComponent * GLBillboardPass::getGLRPC()
{
	return m_GLRPC;
}