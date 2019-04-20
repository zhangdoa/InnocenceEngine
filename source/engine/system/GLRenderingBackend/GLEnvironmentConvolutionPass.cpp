#include "GLEnvironmentConvolutionPass.h"
#include "GLRenderingSystemUtilities.h"
#include "../../component/GLRenderingSystemComponent.h"
#include "../../component/RenderingFrontendSystemComponent.h"

#include "GLEnvironmentCapturePass.h"

#include "../ICoreSystem.h"

extern ICoreSystem* g_pCoreSystem;

using namespace GLRenderingSystemNS;

INNO_PRIVATE_SCOPE GLEnvironmentConvolutionPass
{
	EntityID m_entityID;

	GLRenderPassComponent* m_GLRPC;

	GLShaderProgramComponent* m_GLSPC;

	GLFrameBufferDesc m_frameBufferDesc = GLFrameBufferDesc();
	TextureDataDesc m_textureDesc = TextureDataDesc();

	ShaderFilePaths m_shaderFilePaths = { "GL//environmentConvolutionPass.vert" , "", "GL//environmentConvolutionPass.frag" };
}

bool GLEnvironmentConvolutionPass::initialize()
{
	m_entityID = InnoMath::createEntityID();

	m_textureDesc.samplerType = TextureSamplerType::CUBEMAP;
	m_textureDesc.usageType = TextureUsageType::RENDER_TARGET;
	m_textureDesc.colorComponentsFormat = TextureColorComponentsFormat::RGB16F;
	m_textureDesc.pixelDataFormat = TexturePixelDataFormat::RGB;
	m_textureDesc.minFilterMethod = TextureFilterMethod::LINEAR;
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

bool GLEnvironmentConvolutionPass::update()
{
	auto l_mainConvolution = g_pCoreSystem->getGameSystem()->get<EnvironmentCaptureComponent>()[0];
	auto l_mainConvolutionTransformComponent = g_pCoreSystem->getGameSystem()->get<TransformComponent>(l_mainConvolution->m_parentEntity);
	//auto l_capturePos = l_mainConvolutionTransformComponent->m_localTransformVector.m_pos;
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

	activateRenderPass(m_GLRPC);

	activateShaderProgram(m_GLSPC);

	// uni_p
	updateUniform(0, l_p);

	auto l_GLTDC = m_GLRPC->m_GLTDCs[0];
	activateTexture(GLEnvironmentCapturePass::getGLRPC()->m_GLTDCs[0], 0);

	for (unsigned int i = 0; i < 6; ++i)
	{
		// uni_v
		updateUniform(1, l_v[i]);
		attachCubemapColorRT(l_GLTDC, m_GLRPC, 0, i, 0);

		drawMesh(l_MDC);
	}

	return true;
}

bool GLEnvironmentConvolutionPass::reloadShader()
{
	deleteShaderProgram(m_GLSPC);

	initializeGLShaderProgramComponent(m_GLSPC, m_shaderFilePaths);

	return true;
}

GLRenderPassComponent * GLEnvironmentConvolutionPass::getGLRPC()
{
	return m_GLRPC;
}