#include "GLEnvironmentPreFilterPass.h"
#include "GLRenderingBackendUtilities.h"
#include "../../Component/GLRenderingBackendComponent.h"

#include "GLEnvironmentCapturePass.h"

#include "../../ModuleManager/IModuleManager.h"

extern IModuleManager* g_pModuleManager;

using namespace GLRenderingBackendNS;

INNO_PRIVATE_SCOPE GLEnvironmentPreFilterPass
{
	void initializeShaders();

	EntityID m_entityID;

	GLRenderPassComponent* m_GLRPC;

	GLShaderProgramComponent* m_GLSPC;

	ShaderFilePaths m_shaderFilePaths = { "environmentPreFilterPass.vert/" , "", "", "", "environmentPreFilterPass.frag/" };

	const unsigned int m_subDivideDimension = 2;
	const unsigned int m_totalCubemaps = m_subDivideDimension * m_subDivideDimension * m_subDivideDimension;
	std::vector<GLTextureDataComponent*> m_preFiltedCubemaps;
}

bool GLEnvironmentPreFilterPass::initialize()
{
	m_entityID = InnoMath::createEntityID();

	auto l_renderPassDesc = GLRenderingBackendComponent::get().m_deferredRenderPassDesc;

	l_renderPassDesc.RTDesc.samplerType = TextureSamplerType::SAMPLER_CUBEMAP;
	l_renderPassDesc.RTDesc.usageType = TextureUsageType::COLOR_ATTACHMENT;
	l_renderPassDesc.RTDesc.pixelDataFormat = TexturePixelDataFormat::RGBA;
	l_renderPassDesc.RTDesc.minFilterMethod = TextureFilterMethod::LINEAR_MIPMAP_LINEAR;
	l_renderPassDesc.RTDesc.magFilterMethod = TextureFilterMethod::LINEAR;
	l_renderPassDesc.RTDesc.wrapMethod = TextureWrapMethod::REPEAT;
	l_renderPassDesc.RTDesc.width = 128;
	l_renderPassDesc.RTDesc.height = 128;
	l_renderPassDesc.RTDesc.pixelDataType = TexturePixelDataType::FLOAT16;
	l_renderPassDesc.useDepthAttachment = true;

	m_GLRPC = addGLRenderPassComponent(m_entityID, "EnvironmentPreFilterPassGLRPC/");
	m_GLRPC->m_renderPassDesc = l_renderPassDesc;
	m_GLRPC->m_drawColorBuffers = true;
	initializeGLRenderPassComponent(m_GLRPC);

	m_GLSPC = addGLShaderProgramComponent(m_entityID);
	initializeGLShaderProgramComponent(m_GLSPC, m_shaderFilePaths);

	m_preFiltedCubemaps.reserve(m_totalCubemaps);

	for (size_t i = 0; i < m_totalCubemaps; i++)
	{
		auto l_preFiltedCubemap = addGLTextureDataComponent();
		l_preFiltedCubemap->m_textureDataDesc = l_renderPassDesc.RTDesc;
		l_preFiltedCubemap->m_textureData = nullptr;
		initializeGLTextureDataComponent(l_preFiltedCubemap);

		m_preFiltedCubemaps.emplace_back(l_preFiltedCubemap);
	}

	return true;
}

bool GLEnvironmentPreFilterPass::update()
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

	// draw pre-filter pass
	activateRenderPass(m_GLRPC);

	activateShaderProgram(m_GLSPC);

	// uni_p
	updateUniform(0, l_p);

	auto l_capturedCubemaps = GLEnvironmentCapturePass::getCapturedCubemaps();

	for (size_t i = 0; i < l_capturedCubemaps.size(); i++)
	{
		activateTexture(l_capturedCubemaps[i], 0);

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
			for (unsigned int j = 0; j < 6; ++j)
			{
				// uni_v
				updateUniform(1, l_v[j]);
				bindCubemapTextureForWrite(m_preFiltedCubemaps[i], m_GLRPC, 0, j, mip);
				drawMesh(l_MDC);
				unbindCubemapTextureForWrite(m_GLRPC, 0, j, mip);
			}
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

const std::vector<GLTextureDataComponent*>& GLEnvironmentPreFilterPass::getPreFiltedCubemaps()
{
	return m_preFiltedCubemaps;
}