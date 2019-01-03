#include "GLRenderingSystemUtilities.h"
#include "GLShadowRenderingPassUtilities.h"
#include "../component/GLShadowRenderPassComponent.h"
#include "../component/GameSystemComponent.h"
#include "../component/GLRenderingSystemComponent.h"
#include "ICoreSystem.h"

extern ICoreSystem* g_pCoreSystem;

INNO_PRIVATE_SCOPE GLRenderingSystemNS
{
	GLFrameBufferDesc shadowPassFBDesc = GLFrameBufferDesc();
	TextureDataDesc shadowPassTextureDesc = TextureDataDesc();
}

void GLRenderingSystemNS::initializeShadowPass()
{
	shadowPassFBDesc.renderBufferAttachmentType = GL_DEPTH_ATTACHMENT;
	shadowPassFBDesc.renderBufferInternalFormat = GL_DEPTH_COMPONENT32;
	shadowPassFBDesc.sizeX = 2048;
	shadowPassFBDesc.sizeY = 2048;

	shadowPassTextureDesc.textureUsageType = TextureUsageType::SHADOWMAP;
	shadowPassTextureDesc.textureColorComponentsFormat = TextureColorComponentsFormat::DEPTH_COMPONENT;
	shadowPassTextureDesc.texturePixelDataFormat = TexturePixelDataFormat::DEPTH_COMPONENT;
	shadowPassTextureDesc.textureMinFilterMethod = TextureFilterMethod::NEAREST;
	shadowPassTextureDesc.textureMagFilterMethod = TextureFilterMethod::NEAREST;
	shadowPassTextureDesc.textureWrapMethod = TextureWrapMethod::CLAMP_TO_BORDER;
	shadowPassTextureDesc.textureWidth = shadowPassFBDesc.sizeX;
	shadowPassTextureDesc.textureHeight = shadowPassFBDesc.sizeY;
	shadowPassTextureDesc.texturePixelDataType = TexturePixelDataType::FLOAT;

	GLShadowRenderPassComponent::get().m_GLRPC = addGLRenderPassComponent(1, shadowPassFBDesc, shadowPassTextureDesc);

	// shader programs and shaders
	auto rhs = addGLShaderProgramComponent(0);
	initializeGLShaderProgramComponent(rhs, GLShadowRenderPassComponent::get().m_shaderFilePaths);

	GLShadowRenderPassComponent::get().m_shadowPass_uni_p = getUniformLocation(
		rhs->m_program,
		"uni_p");
	GLShadowRenderPassComponent::get().m_shadowPass_uni_v = getUniformLocation(
		rhs->m_program,
		"uni_v");
	GLShadowRenderPassComponent::get().m_shadowPass_uni_m = getUniformLocation(
		rhs->m_program,
		"uni_m");

	GLShadowRenderPassComponent::get().m_SPC = rhs;
}

void GLRenderingSystemNS::updateShadowPass()
{
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);

	glEnable(GL_CULL_FACE);
	glFrontFace(GL_CCW);
	glCullFace(GL_FRONT);

	activateShaderProgram(GLShadowRenderPassComponent::get().m_SPC);

	auto l_GLFBC = GLShadowRenderPassComponent::get().m_GLRPC->m_GLFBC;
	auto sizeX = l_GLFBC->m_GLFrameBufferDesc.sizeX;
	auto sizeY = l_GLFBC->m_GLFrameBufferDesc.sizeY;

	cleanFBC(l_GLFBC);
	glRenderbufferStorage(GL_RENDERBUFFER, l_GLFBC->m_GLFrameBufferDesc.renderBufferInternalFormat, sizeX, sizeY);

	unsigned int splitCount = 0;

	for (unsigned int i = 0; i < 2; i++)
	{
		for (unsigned int j = 0; j < 2; j++)
		{
			glViewport(i * sizeX / 2, j * sizeY / 2, sizeX / 2, sizeY / 2);
			updateUniform(
				GLShadowRenderPassComponent::get().m_shadowPass_uni_p,
				GLRenderingSystemComponent::get().m_CSMProjs[splitCount]);

			updateUniform(
				GLShadowRenderPassComponent::get().m_shadowPass_uni_v,
				GLRenderingSystemComponent::get().m_CSMViews[splitCount]);

			splitCount++;

			// draw each visibleComponent
			for (auto& l_visibleComponent : GameSystemComponent::get().m_VisibleComponents)
			{
				if (l_visibleComponent->m_visiblilityType == VisiblilityType::INNO_OPAQUE)
				{
					updateUniform(
						GLShadowRenderPassComponent::get().m_shadowPass_uni_m,
						g_pCoreSystem->getGameSystem()->get<TransformComponent>(l_visibleComponent->m_parentEntity)->m_globalTransformMatrix.m_transformationMat);

					// draw each graphic data of visibleComponent
					for (auto& l_modelPair : l_visibleComponent->m_modelMap)
					{
						// draw meshes
						auto l_MDC = l_modelPair.first;
						if (l_MDC)
						{
							// draw meshes
							drawMesh(l_MDC);
						}
					}
				}
			}
		}
	}

	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);
}