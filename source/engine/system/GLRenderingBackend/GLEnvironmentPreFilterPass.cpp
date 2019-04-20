#include "GLEnvironmentPreFilterPass.h"
#include "GLRenderingSystemUtilities.h"
#include "../../component/GLRenderingSystemComponent.h"
#include "../../component/RenderingFrontendSystemComponent.h"

#include "GLEnvironmentCapturePass.h"

#include "../ICoreSystem.h"

extern ICoreSystem* g_pCoreSystem;

using namespace GLRenderingSystemNS;

INNO_PRIVATE_SCOPE GLEnvironmentPreFilterPass
{
	void initializeShaders();

	EntityID m_entityID;

	GLRenderPassComponent* m_GLRPC;

	GLShaderProgramComponent* m_GLSPC;

	GLFrameBufferDesc m_frameBufferDesc = GLFrameBufferDesc();
	TextureDataDesc m_textureDesc = TextureDataDesc();

	ShaderFilePaths m_shaderFilePaths = { "GL//environmentPreFilterPass.vert" , "", "GL//environmentPreFilterPass.frag" };
}

bool GLEnvironmentPreFilterPass::initialize()
{
	m_entityID = InnoMath::createEntityID();

	m_textureDesc.samplerType = TextureSamplerType::CUBEMAP;
	m_textureDesc.usageType = TextureUsageType::RENDER_TARGET;
	m_textureDesc.colorComponentsFormat = TextureColorComponentsFormat::RGB16F;
	m_textureDesc.pixelDataFormat = TexturePixelDataFormat::RGB;
	m_textureDesc.minFilterMethod = TextureFilterMethod::LINEAR_MIPMAP_LINEAR;
	m_textureDesc.magFilterMethod = TextureFilterMethod::LINEAR;
	m_textureDesc.wrapMethod = TextureWrapMethod::REPEAT;
	m_textureDesc.width = 128;
	m_textureDesc.height = 128;
	m_textureDesc.pixelDataType = TexturePixelDataType::FLOAT;

	m_frameBufferDesc.renderBufferAttachmentType = GL_DEPTH_ATTACHMENT;
	m_frameBufferDesc.renderBufferInternalFormat = GL_DEPTH_COMPONENT24;
	m_frameBufferDesc.sizeX = m_textureDesc.width;
	m_frameBufferDesc.sizeY = m_textureDesc.height;
	m_frameBufferDesc.drawColorBuffers = true;

	m_GLRPC = addGLRenderPassComponent(1, m_frameBufferDesc, m_textureDesc);

	auto rhs = addGLShaderProgramComponent(m_entityID);
	initializeGLShaderProgramComponent(rhs, m_shaderFilePaths);

	m_GLSPC = rhs;

	return true;
}

bool GLEnvironmentPreFilterPass::update()
{
	auto l_mainPreFilter = g_pCoreSystem->getGameSystem()->get<EnvironmentCaptureComponent>()[0];
	auto l_mainPreFilterTransformComponent = g_pCoreSystem->getGameSystem()->get<TransformComponent>(l_mainPreFilter->m_parentEntity);
	//auto l_capturePos = l_mainPreFilterTransformComponent->m_localTransformVector.m_pos;
	auto l_capturePos = vec4(0.0f, 0.0f, 0.0f, 1.0f);
	mat4 l_p = InnoMath::generatePerspectiveMatrix((90.0f / 180.0f) * PI<float>, 1.0f, 0.1f, 10.0f);
	std::vector<mat4> l_v =
	{
		InnoMath::lookAt(l_capturePos, l_capturePos + vec4(1.0f,  0.0f,  0.0f, 0.0f), vec4(0.0f, -1.0f,  0.0f, 0.0f)),
		InnoMath::lookAt(l_capturePos, l_capturePos + vec4(-1.0f,  0.0f,  0.0f, 0.0f), vec4(0.0f, -1.0f,  0.0f, 0.0f)),
		InnoMath::lookAt(l_capturePos, l_capturePos + vec4(0.0f,  1.0f,  0.0f, 0.0f), vec4(0.0f,  0.0f,  1.0f, 0.0f)),
		InnoMath::lookAt(l_capturePos, l_capturePos + vec4(0.0f, -1.0f,  0.0f, 0.0f), vec4(0.0f,  0.0f, -1.0f, 0.0f)),
		InnoMath::lookAt(l_capturePos, l_capturePos + vec4(0.0f,  0.0f,  1.0f, 0.0f), vec4(0.0f, -1.0f,  0.0f, 0.0f)),
		InnoMath::lookAt(l_capturePos, l_capturePos + vec4(0.0f,  0.0f, -1.0f, 0.0f), vec4(0.0f, -1.0f,  0.0f, 0.0f))
	};

	auto l_renderingConfig = g_pCoreSystem->getVisionSystem()->getRenderingFrontend()->getRenderingConfig();

	auto l_MDC = getGLMeshDataComponent(MeshShapeType::CUBE);

	// draw pre-filter pass
	activateRenderPass(m_GLRPC);

	activateShaderProgram(m_GLSPC);

	// uni_p
	updateUniform(0, l_p);

	auto l_GLTDC = m_GLRPC->m_GLTDCs[0];

	activateTexture(GLEnvironmentCapturePass::getGLRPC()->m_GLTDCs[0], 0);

	unsigned int l_maxMipLevels = 5;
	for (unsigned int mip = 0; mip < l_maxMipLevels; ++mip)
	{
		// resize framebuffer according to mip-level size.
		unsigned int mipWidth = (int)(128 * std::pow(0.5, mip));
		unsigned int mipHeight = (int)(128 * std::pow(0.5, mip));

		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT32, mipWidth, mipHeight);
		glViewport(0, 0, mipWidth, mipHeight);

		float roughness = (float)mip / (float)(l_maxMipLevels - 1);

		// uni_roughness
		updateUniform(3, roughness);
		for (unsigned int i = 0; i < 6; ++i)
		{
			// uni_v
			updateUniform(1, l_v[i]);
			attachCubemapColorRT(l_GLTDC, m_GLRPC, 0, i, mip);
			drawMesh(l_MDC);
		}
	}

	return true;
}

bool GLEnvironmentPreFilterPass::reloadShader()
{
	deleteShaderProgram(m_GLSPC);

	initializeGLShaderProgramComponent(m_GLSPC, m_shaderFilePaths);

	return true;
}

GLRenderPassComponent * GLEnvironmentPreFilterPass::getGLRPC()
{
	return m_GLRPC;
}