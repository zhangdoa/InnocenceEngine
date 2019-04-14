#include "GLOpaquePass.h"
#include "GLEarlyZPass.h"
#include "GLRenderingSystemUtilities.h"
#include "../../component/GLRenderingSystemComponent.h"

using namespace GLRenderingSystemNS;

#include "../ICoreSystem.h"

extern ICoreSystem* g_pCoreSystem;

INNO_PRIVATE_SCOPE GLOpaquePass
{
	void initializeShaders();
	void bindUniformLocations(GLShaderProgramComponent* rhs);

	EntityID m_entityID;

	GLRenderPassComponent* m_GLRPC;

	GLShaderProgramComponent* m_GLSPC;

#ifdef CookTorrance
	ShaderFilePaths m_shaderFilePaths = { "GL//opaquePassCookTorrance.vert" , "", "GL//opaquePassCookTorrance.frag" };
#elif BlinnPhong
	ShaderFilePaths m_shaderFilePaths = { "GL//opaquePassBlinnPhong.vert" , "", "GL//opaquePassBlinnPhong.frag" };
#endif

	GLuint m_uni_id;

	std::vector<std::string> m_TextureUniformNames =
	{
		"uni_normalTexture",
		"uni_albedoTexture",
		"uni_metallicTexture",
		"uni_roughnessTexture",
		"uni_aoTexture",
	};
}

bool GLOpaquePass::initialize()
{
	m_entityID = InnoMath::createEntityID();

	m_GLRPC = addGLRenderPassComponent(4, GLRenderingSystemComponent::get().deferredPassFBDesc, GLRenderingSystemComponent::get().deferredPassTextureDesc);

	initializeShaders();

	return true;
}

void GLOpaquePass::initializeShaders()
{
	// shader programs and shaders
	auto rhs = addGLShaderProgramComponent(m_entityID);

	initializeGLShaderProgramComponent(rhs, m_shaderFilePaths);

	bindUniformLocations(rhs);

	m_GLSPC = rhs;
}

void GLOpaquePass::bindUniformLocations(GLShaderProgramComponent* rhs)
{
	bindUniformBlock(GLRenderingSystemComponent::get().m_cameraUBO, sizeof(GPassCameraUBOData), rhs->m_program, "cameraUBO", 0);

	bindUniformBlock(GLRenderingSystemComponent::get().m_meshUBO, sizeof(GPassMeshUBOData), rhs->m_program, "meshUBO", 1);

	bindUniformBlock(GLRenderingSystemComponent::get().m_textureUBO, sizeof(GPassTextureUBOData), rhs->m_program, "textureUBO", 2);

	m_uni_id = getUniformLocation(rhs->m_program, "uni_id");

#ifdef CookTorrance
	updateTextureUniformLocations(rhs->m_program, m_TextureUniformNames);
#elif BlinnPhong
	// @TODO: texture uniforms
#endif
}

bool GLOpaquePass::update()
{
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glEnable(GL_DEPTH_CLAMP);
	glDepthMask(GL_FALSE);

	glEnable(GL_STENCIL_TEST);
	glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
	glStencilMask(0xFF);

	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);

	activateRenderPass(m_GLRPC);

	copyDepthBuffer(GLEarlyZPass::getGLRPC(), m_GLRPC);

	activateShaderProgram(m_GLSPC);

	updateUBO(GLRenderingSystemComponent::get().m_cameraUBO, GLRenderingSystemComponent::get().m_GPassCameraUBOData);

#ifdef CookTorrance
	while (GLRenderingSystemComponent::get().m_opaquePassDataQueue.size() > 0)
	{
		auto l_renderPack = GLRenderingSystemComponent::get().m_opaquePassDataQueue.front();
		if (l_renderPack.meshShapeType != MeshShapeType::CUSTOM)
		{
			glFrontFace(GL_CW);
		}
		else
		{
			glFrontFace(GL_CCW);
		}
		if (l_renderPack.visiblilityType == VisiblilityType::INNO_OPAQUE)
		{
			glStencilFunc(GL_ALWAYS, 0x01, 0xFF);

			// any normal?
			if (l_renderPack.textureUBOData.useNormalTexture)
			{
				activateTexture(l_renderPack.normalGLTDC, 0);
			}
			// any albedo?
			if (l_renderPack.textureUBOData.useAlbedoTexture)
			{
				activateTexture(l_renderPack.albedoGLTDC, 1);
			}
			// any metallic?
			if (l_renderPack.textureUBOData.useMetallicTexture)
			{
				activateTexture(l_renderPack.metallicGLTDC, 2);
			}
			// any roughness?
			if (l_renderPack.textureUBOData.useRoughnessTexture)
			{
				activateTexture(l_renderPack.roughnessGLTDC, 3);
			}
			// any ao?
			if (l_renderPack.textureUBOData.useAOTexture)
			{
				activateTexture(l_renderPack.AOGLTDC, 4);
			}

			updateUBO(GLRenderingSystemComponent::get().m_meshUBO, l_renderPack.meshUBOData);
			updateUBO(GLRenderingSystemComponent::get().m_textureUBO, l_renderPack.textureUBOData);
			updateUniform(m_uni_id, l_renderPack.UUID);

			drawMesh(l_renderPack.indiceSize, l_renderPack.meshPrimitiveTopology, l_renderPack.GLMDC);
		}
		else if (l_renderPack.visiblilityType == VisiblilityType::INNO_EMISSIVE)
		{
			glStencilFunc(GL_ALWAYS, 0x02, 0xFF);

			updateUBO(GLRenderingSystemComponent::get().m_meshUBO, l_renderPack.meshUBOData);
			updateUBO(GLRenderingSystemComponent::get().m_textureUBO, l_renderPack.textureUBOData);

			drawMesh(l_renderPack.indiceSize, l_renderPack.meshPrimitiveTopology, l_renderPack.GLMDC);
		}
		else
		{
			glStencilFunc(GL_ALWAYS, 0x00, 0xFF);
		}
		GLRenderingSystemComponent::get().m_opaquePassDataQueue.pop();
	}

	glDisable(GL_CULL_FACE);
	glDisable(GL_STENCIL_TEST);
	glDepthMask(GL_TRUE);
	glDisable(GL_DEPTH_CLAMP);
	glDisable(GL_DEPTH_TEST);

#elif BlinnPhong
#endif

	return true;
}

bool GLOpaquePass::resize(unsigned int newSizeX, unsigned int newSizeY)
{
	resizeGLRenderPassComponent(m_GLRPC, newSizeX, newSizeY);

	return true;
}

bool GLOpaquePass::reloadShader()
{
	deleteShaderProgram(m_GLSPC);

	initializeGLShaderProgramComponent(m_GLSPC, m_shaderFilePaths);

	bindUniformLocations(m_GLSPC);

	return true;
}

GLRenderPassComponent * GLOpaquePass::getGLRPC()
{
	return m_GLRPC;
}