#include "GLEnvironmentConvolutionPass.h"
#include "GLRenderingBackendUtilities.h"
#include "../../Component/GLRenderingBackendComponent.h"

#include "GLEnvironmentCapturePass.h"

#include "../../ModuleManager/IModuleManager.h"

extern IModuleManager* g_pModuleManager;

using namespace GLRenderingBackendNS;

INNO_PRIVATE_SCOPE GLEnvironmentConvolutionPass
{
	EntityID m_entityID;

	GLRenderPassComponent* m_GLRPC;

	GLShaderProgramComponent* m_GLSPC;

	ShaderFilePaths m_shaderFilePaths = { "environmentConvolutionPass.vert/" , "", "", "", "environmentConvolutionPass.frag/" };
}

bool GLEnvironmentConvolutionPass::initialize()
{
	m_entityID = InnoMath::createEntityID();

	auto l_renderPassDesc = GLRenderingBackendComponent::get().m_deferredRenderPassDesc;

	l_renderPassDesc.RTDesc.samplerType = TextureSamplerType::SAMPLER_CUBEMAP;
	l_renderPassDesc.RTDesc.usageType = TextureUsageType::COLOR_ATTACHMENT;
	l_renderPassDesc.RTDesc.pixelDataFormat = TexturePixelDataFormat::RGBA;
	l_renderPassDesc.RTDesc.minFilterMethod = TextureFilterMethod::LINEAR;
	l_renderPassDesc.RTDesc.magFilterMethod = TextureFilterMethod::LINEAR;
	l_renderPassDesc.RTDesc.wrapMethod = TextureWrapMethod::REPEAT;
	l_renderPassDesc.RTDesc.width = 128;
	l_renderPassDesc.RTDesc.height = 128;
	l_renderPassDesc.RTDesc.pixelDataType = TexturePixelDataType::FLOAT16;
	l_renderPassDesc.useDepthAttachment = true;

	m_GLRPC = addGLRenderPassComponent(m_entityID, "EnvironmentConvolutionPassGLRPC/");
	m_GLRPC->m_renderPassDesc = l_renderPassDesc;
	m_GLRPC->m_drawColorBuffers = true;
	initializeGLRenderPassComponent(m_GLRPC);

	m_GLSPC = addGLShaderProgramComponent(m_entityID);
	initializeGLShaderProgramComponent(m_GLSPC, m_shaderFilePaths);

	return true;
}

bool GLEnvironmentConvolutionPass::update(GLTextureDataComponent* GLTDC)
{
	auto l_p = InnoMath::generatePerspectiveMatrix((90.0f / 180.0f) * PI<float>, 1.0f, 0.1f, 10.0f);

	auto l_rPX = InnoMath::toRotationMatrix(InnoMath::getQuatRotator(vec4(0.0f, 1.0f, 0.0f, 0.0f), -90.0f)).inverse();
	auto l_rNX = InnoMath::toRotationMatrix(InnoMath::getQuatRotator(vec4(0.0f, 1.0f, 0.0f, 0.0f), 90.0f)).inverse();
	auto l_rPY = InnoMath::toRotationMatrix(InnoMath::getQuatRotator(vec4(1.0f, 0.0f, 0.0f, 0.0f), -90.0f)).inverse();
	auto l_rNY = InnoMath::toRotationMatrix(InnoMath::getQuatRotator(vec4(1.0f, 0.0f, 0.0f, 0.0f), 90.0f)).inverse();
	auto l_rPZ = InnoMath::toRotationMatrix(InnoMath::getQuatRotator(vec4(0.0f, 1.0f, 0.0f, 0.0f), 0.0f)).inverse();
	auto l_rNZ = InnoMath::toRotationMatrix(InnoMath::getQuatRotator(vec4(0.0f, 1.0f, 0.0f, 0.0f), 180.0f)).inverse();
	std::vector<mat4> l_v =
	{
		l_rPX, l_rNX, l_rPY, l_rNY, l_rPZ, l_rNZ
	};

	auto l_renderingConfig = g_pModuleManager->getRenderingFrontend()->getRenderingConfig();

	auto l_MDC = getGLMeshDataComponent(MeshShapeType::CUBE);

	activateRenderPass(m_GLRPC);

	activateShaderProgram(m_GLSPC);

	// uni_p
	updateUniform(0, l_p);

	activateTexture(GLTDC, 0);

	for (unsigned int i = 0; i < 6; ++i)
	{
		// uni_v
		updateUniform(1, l_v[i]);
		bindCubemapTextureForWrite(m_GLRPC->m_GLTDCs[0], m_GLRPC, 0, i, 0);

		drawMesh(l_MDC);

		unbindCubemapTextureForWrite(m_GLRPC, 0, i, 0);
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