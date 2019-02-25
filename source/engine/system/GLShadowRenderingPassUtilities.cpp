#include "GLRenderingSystemUtilities.h"
#include "GLShadowRenderingPassUtilities.h"
#include "../component/GLShadowRenderPassComponent.h"
#include "../component/GameSystemComponent.h"
#include "../component/RenderingSystemComponent.h"
#include "ICoreSystem.h"

extern ICoreSystem* g_pCoreSystem;

using namespace GLRenderingSystemNS;

INNO_PRIVATE_SCOPE GLShadowRenderingPassUtilities
{
	GLFrameBufferDesc DirLightShadowPassFBDesc = GLFrameBufferDesc();
	TextureDataDesc DirLightShadowPassTextureDesc = TextureDataDesc();

	GLFrameBufferDesc PointLightShadowPassFBDesc = GLFrameBufferDesc();
	TextureDataDesc PointLightShadowPassTextureDesc = TextureDataDesc();

	EntityID m_entityID;

	void drawAllMeshDataComponents();
}

void GLShadowRenderingPassUtilities::initialize()
{
	m_entityID = InnoMath::createEntityID();

	DirLightShadowPassFBDesc.renderBufferAttachmentType = GL_DEPTH_ATTACHMENT;
	DirLightShadowPassFBDesc.renderBufferInternalFormat = GL_DEPTH_COMPONENT32;
	DirLightShadowPassFBDesc.sizeX = 2048;
	DirLightShadowPassFBDesc.sizeY = 2048;
	DirLightShadowPassFBDesc.drawColorBuffers = false;

	DirLightShadowPassTextureDesc.textureUsageType = TextureUsageType::RENDER_TARGET;
	DirLightShadowPassTextureDesc.textureColorComponentsFormat = TextureColorComponentsFormat::DEPTH_COMPONENT;
	DirLightShadowPassTextureDesc.texturePixelDataFormat = TexturePixelDataFormat::DEPTH_COMPONENT;
	DirLightShadowPassTextureDesc.textureMinFilterMethod = TextureFilterMethod::NEAREST;
	DirLightShadowPassTextureDesc.textureMagFilterMethod = TextureFilterMethod::NEAREST;
	DirLightShadowPassTextureDesc.textureWrapMethod = TextureWrapMethod::CLAMP_TO_BORDER;
	DirLightShadowPassTextureDesc.textureWidth = DirLightShadowPassFBDesc.sizeX;
	DirLightShadowPassTextureDesc.textureHeight = DirLightShadowPassFBDesc.sizeY;
	DirLightShadowPassTextureDesc.texturePixelDataType = TexturePixelDataType::FLOAT;
	DirLightShadowPassTextureDesc.borderColor = vec4(1.0f, 1.0f, 1.0f, 1.0f);

	GLShadowRenderPassComponent::get().m_DirLight_GLRPC = addGLRenderPassComponent(1, DirLightShadowPassFBDesc, DirLightShadowPassTextureDesc);

	PointLightShadowPassFBDesc.renderBufferAttachmentType = GL_DEPTH_ATTACHMENT;
	PointLightShadowPassFBDesc.renderBufferInternalFormat = GL_DEPTH_COMPONENT32;
	PointLightShadowPassFBDesc.sizeX = 4096;
	PointLightShadowPassFBDesc.sizeY = 4096;
	PointLightShadowPassFBDesc.drawColorBuffers = false;

	PointLightShadowPassTextureDesc.textureUsageType = TextureUsageType::CUBEMAP;
	PointLightShadowPassTextureDesc.textureColorComponentsFormat = TextureColorComponentsFormat::DEPTH_COMPONENT;
	PointLightShadowPassTextureDesc.texturePixelDataFormat = TexturePixelDataFormat::DEPTH_COMPONENT;
	PointLightShadowPassTextureDesc.textureMinFilterMethod = TextureFilterMethod::NEAREST;
	PointLightShadowPassTextureDesc.textureMagFilterMethod = TextureFilterMethod::NEAREST;
	PointLightShadowPassTextureDesc.textureWrapMethod = TextureWrapMethod::CLAMP_TO_BORDER;
	PointLightShadowPassTextureDesc.textureWidth = PointLightShadowPassFBDesc.sizeX;
	PointLightShadowPassTextureDesc.textureHeight = PointLightShadowPassFBDesc.sizeY;
	PointLightShadowPassTextureDesc.texturePixelDataType = TexturePixelDataType::FLOAT;
	PointLightShadowPassTextureDesc.borderColor = vec4(1.0f, 1.0f, 1.0f, 1.0f);

	GLShadowRenderPassComponent::get().m_PointLight_GLRPC = addGLRenderPassComponent(1, PointLightShadowPassFBDesc, PointLightShadowPassTextureDesc);

	// shader programs and shaders
	auto rhs = addGLShaderProgramComponent(m_entityID);
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

void GLShadowRenderingPassUtilities::drawAllMeshDataComponents()
{
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

void GLShadowRenderingPassUtilities::update()
{
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);

	glEnable(GL_CULL_FACE);
	glFrontFace(GL_CCW);
	glCullFace(GL_FRONT);

	activateShaderProgram(GLShadowRenderPassComponent::get().m_SPC);

	auto l_GLFBC = GLShadowRenderPassComponent::get().m_DirLight_GLRPC->m_GLFBC;
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
				RenderingSystemComponent::get().m_CSMProjs[splitCount]);

			updateUniform(
				GLShadowRenderPassComponent::get().m_shadowPass_uni_v,
				RenderingSystemComponent::get().m_CSMViews[splitCount]);

			splitCount++;

			drawAllMeshDataComponents();
		}
	}

	mat4 l_p = InnoMath::generatePerspectiveMatrix((90.0f / 180.0f) * PI<float>, 1.0f, 0.1f, 10.0f);
	auto l_capturePos = vec4(0.0f, 0.0f, 0.0f, 1.0f);
	std::vector<mat4> l_v =
	{
		InnoMath::lookAt(l_capturePos, l_capturePos + vec4(1.0f,  0.0f,  0.0f, 0.0f), vec4(0.0f, -1.0f,  0.0f, 0.0f)),
		InnoMath::lookAt(l_capturePos, l_capturePos + vec4(-1.0f,  0.0f,  0.0f, 0.0f), vec4(0.0f, -1.0f,  0.0f, 0.0f)),
		InnoMath::lookAt(l_capturePos, l_capturePos + vec4(0.0f,  1.0f,  0.0f, 0.0f), vec4(0.0f,  0.0f,  1.0f, 0.0f)),
		InnoMath::lookAt(l_capturePos, l_capturePos + vec4(0.0f, -1.0f,  0.0f, 0.0f), vec4(0.0f,  0.0f, -1.0f, 0.0f)),
		InnoMath::lookAt(l_capturePos, l_capturePos + vec4(0.0f,  0.0f,  1.0f, 0.0f), vec4(0.0f, -1.0f,  0.0f, 0.0f)),
		InnoMath::lookAt(l_capturePos, l_capturePos + vec4(0.0f,  0.0f, -1.0f, 0.0f), vec4(0.0f, -1.0f,  0.0f, 0.0f))
	};

	l_GLFBC = GLShadowRenderPassComponent::get().m_PointLight_GLRPC->m_GLFBC;

	bindFBC(l_GLFBC);

	updateUniform(
		GLShadowRenderPassComponent::get().m_shadowPass_uni_p,
		l_p);

	//for (unsigned int i = 0; i < 6; ++i)
	//{
	//	updateUniform(GLShadowRenderPassComponent::get().m_shadowPass_uni_v, l_v[i]);
	//	attachTextureToFramebuffer(GLShadowRenderPassComponent::get().m_PointLight_GLRPC->m_TDCs[0], GLShadowRenderPassComponent::get().m_PointLight_GLRPC->m_GLTDCs[0], l_GLFBC, 0, i, 0);

	//	glClear(GL_DEPTH_BUFFER_BIT);

	//	drawAllMeshDataComponents();
	//}

	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);
}