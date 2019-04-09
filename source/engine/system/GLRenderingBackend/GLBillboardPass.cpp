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
	void bindUniformLocations(GLShaderProgramComponent* rhs);

	EntityID m_entityID;

	GLRenderPassComponent* m_GLRPC;
	GLShaderProgramComponent* m_GLSPC;
	ShaderFilePaths m_shaderFilePaths = { "GL//billboardPassVertex.sf", "", "GL//billboardPassFragment.sf" };

	std::vector<std::string> m_uniformNames =
	{
		"uni_texture",
	};

	GLuint m_uni_p;
	GLuint m_uni_r;
	GLuint m_uni_t;
	GLuint m_uni_pos;
	GLuint m_uni_size;
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

	bindUniformLocations(rhs);

	m_GLSPC = rhs;
}

void GLBillboardPass::bindUniformLocations(GLShaderProgramComponent* rhs)
{
	updateTextureUniformLocations(rhs->m_program, m_uniformNames);

	m_uni_p = getUniformLocation(
		rhs->m_program,
		"uni_p");
	m_uni_r = getUniformLocation(
		rhs->m_program,
		"uni_r");
	m_uni_t = getUniformLocation(
		rhs->m_program,
		"uni_t");
	m_uni_pos = getUniformLocation(
		rhs->m_program,
		"uni_pos");
	m_uni_size = getUniformLocation(
		rhs->m_program,
		"uni_size");
}

bool GLBillboardPass::update()
{
	auto l_cameraDataPack = g_pCoreSystem->getVisionSystem()->getRenderingFrontend()->getCameraDataPack();

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);

	activateRenderPass(m_GLRPC);

	// copy depth buffer from G-Pass
	copyDepthBuffer(GLOpaquePass::getGLRPC(), m_GLRPC);

	activateShaderProgram(m_GLSPC);

	updateUniform(
		m_uni_p,
		l_cameraDataPack.p_original);
	updateUniform(
		m_uni_r,
		l_cameraDataPack.r);
	updateUniform(
		m_uni_t,
		l_cameraDataPack.t);

	while (GLRenderingSystemComponent::get().m_billboardPassDataQueue.size() > 0)
	{
		auto l_renderPack = GLRenderingSystemComponent::get().m_billboardPassDataQueue.front();

		auto l_GlobalPos = l_renderPack.globalPos;

		updateUniform(
			m_uni_pos,
			l_GlobalPos.x, l_GlobalPos.y, l_GlobalPos.z);

		auto l_distanceToCamera = l_renderPack.distanceToCamera;

		if (l_distanceToCamera > 1.0f)
		{
			updateUniform(
				m_uni_size,
				(1.0f / (l_distanceToCamera * l_cameraDataPack.WHRatio)), (1.0f / l_distanceToCamera));
		}
		else
		{
			updateUniform(
				m_uni_size,
				(1.0f / l_cameraDataPack.WHRatio), 1.0f);
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

bool GLBillboardPass::resize()
{
	resizeGLRenderPassComponent(m_GLRPC, GLRenderingSystemComponent::get().deferredPassFBDesc);

	return true;
}

bool GLBillboardPass::reloadShader()
{
	deleteShaderProgram(m_GLSPC);

	initializeGLShaderProgramComponent(m_GLSPC, m_shaderFilePaths);

	bindUniformLocations(m_GLSPC);

	return true;
}

GLRenderPassComponent * GLBillboardPass::getGLRPC()
{
	return m_GLRPC;
}