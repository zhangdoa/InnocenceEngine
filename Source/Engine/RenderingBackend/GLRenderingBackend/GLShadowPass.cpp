#include "GLRenderingBackendUtilities.h"
#include "GLShadowPass.h"
#include "../../Component/GLRenderingBackendComponent.h"

#include "../../ModuleManager/IModuleManager.h"

extern IModuleManager* g_pModuleManager;

using namespace GLRenderingBackendNS;

INNO_PRIVATE_SCOPE GLShadowPass
{
	EntityID m_entityID;

	GLRenderPassComponent* m_DirLight_GLRPC;
	GLRenderPassComponent* m_PointLight_GLRPC;
	GLTextureDataComponent* m_PointLightDepth_GLTDC;

	GLShaderProgramComponent* m_DirLight_GLSPC;
	GLShaderProgramComponent* m_PointLight_GLSPC;

	ShaderFilePaths m_DirLightShaderFilePaths = { "GL//dirLightShadowPass.vert/" , "", "", "", "GL//dirLightShadowPass.frag/" };
	ShaderFilePaths m_PointLightShaderFilePaths = { "GL/pointLightShadowPass.vert/" , "", "", "GL/pointLightShadowPass.geom/", "GL//pointLightShadowPass.frag/" };
}

void GLShadowPass::initialize()
{
	m_entityID = InnoMath::createEntityID();

	auto l_renderPassDesc = GLRenderingBackendComponent::get().m_deferredRenderPassDesc;
	l_renderPassDesc.RTDesc.samplerType = TextureSamplerType::SAMPLER_2D;
	l_renderPassDesc.RTDesc.usageType = TextureUsageType::COLOR_ATTACHMENT;
	l_renderPassDesc.RTDesc.pixelDataFormat = TexturePixelDataFormat::RG;
	l_renderPassDesc.RTDesc.minFilterMethod = TextureFilterMethod::LINEAR;
	l_renderPassDesc.RTDesc.magFilterMethod = TextureFilterMethod::LINEAR;
	l_renderPassDesc.RTDesc.wrapMethod = TextureWrapMethod::CLAMP_TO_BORDER;
	l_renderPassDesc.RTDesc.width = 4096;
	l_renderPassDesc.RTDesc.height = 4096;
	l_renderPassDesc.RTDesc.pixelDataType = TexturePixelDataType::FLOAT32;
	l_renderPassDesc.RTDesc.borderColor[0] = 1.0f;
	l_renderPassDesc.RTDesc.borderColor[1] = 1.0f;
	l_renderPassDesc.RTDesc.borderColor[2] = 1.0f;
	l_renderPassDesc.RTDesc.borderColor[3] = 1.0f;

	m_DirLight_GLRPC = addGLRenderPassComponent(m_entityID, "DirectionalLightShadowPassGLRPC/");
	m_DirLight_GLRPC->m_renderPassDesc = l_renderPassDesc;
	m_DirLight_GLRPC->m_renderPassDesc.useDepthAttachment = true;
	m_DirLight_GLRPC->m_drawColorBuffers = true;
	initializeGLRenderPassComponent(m_DirLight_GLRPC);

	l_renderPassDesc.RTDesc.samplerType = TextureSamplerType::SAMPLER_CUBEMAP;
	l_renderPassDesc.RTDesc.usageType = TextureUsageType::COLOR_ATTACHMENT;
	l_renderPassDesc.RTDesc.pixelDataFormat = TexturePixelDataFormat::RG;
	l_renderPassDesc.RTDesc.minFilterMethod = TextureFilterMethod::LINEAR;
	l_renderPassDesc.RTDesc.magFilterMethod = TextureFilterMethod::LINEAR;
	l_renderPassDesc.RTDesc.wrapMethod = TextureWrapMethod::CLAMP_TO_BORDER;
	l_renderPassDesc.RTDesc.width = 1024;
	l_renderPassDesc.RTDesc.height = 1024;
	l_renderPassDesc.RTDesc.pixelDataType = TexturePixelDataType::FLOAT32;
	l_renderPassDesc.RTDesc.borderColor[0] = 1.0f;
	l_renderPassDesc.RTDesc.borderColor[1] = 1.0f;
	l_renderPassDesc.RTDesc.borderColor[2] = 1.0f;
	l_renderPassDesc.RTDesc.borderColor[3] = 1.0f;

	m_PointLight_GLRPC = addGLRenderPassComponent(m_entityID, "PointLightShadowPassGLRPC/");
	m_PointLight_GLRPC->m_renderPassDesc = l_renderPassDesc;
	m_PointLight_GLRPC->m_renderPassDesc.useDepthAttachment = true;
	m_PointLight_GLRPC->m_drawColorBuffers = true;
	initializeGLRenderPassComponent(m_PointLight_GLRPC);

	l_renderPassDesc.RTDesc.usageType = TextureUsageType::DEPTH_ATTACHMENT;
	l_renderPassDesc.RTDesc.pixelDataFormat = TexturePixelDataFormat::DEPTH_COMPONENT;
	m_PointLightDepth_GLTDC = addGLTextureDataComponent();
	m_PointLightDepth_GLTDC->m_textureDataDesc = l_renderPassDesc.RTDesc;
	m_PointLightDepth_GLTDC->m_textureData = nullptr;
	initializeGLTextureDataComponent(m_PointLightDepth_GLTDC);

	m_DirLight_GLSPC = addGLShaderProgramComponent(m_entityID);
	initializeGLShaderProgramComponent(m_DirLight_GLSPC, m_DirLightShaderFilePaths);

	m_PointLight_GLSPC = addGLShaderProgramComponent(m_entityID);
	initializeGLShaderProgramComponent(m_PointLight_GLSPC, m_PointLightShaderFilePaths);
}

void GLShadowPass::update()
{
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glDepthMask(GL_TRUE);

	glEnable(GL_CULL_FACE);
	glCullFace(GL_FRONT);

	activateShaderProgram(m_DirLight_GLSPC);

	activateRenderPass(m_DirLight_GLRPC);

	auto sizeX = m_DirLight_GLRPC->m_renderPassDesc.RTDesc.width;
	auto sizeY = m_DirLight_GLRPC->m_renderPassDesc.RTDesc.height;

	unsigned int splitCount = 0;

	for (unsigned int i = 0; i < 2; i++)
	{
		for (unsigned int j = 0; j < 2; j++)
		{
			glViewport(i * sizeX / 2, j * sizeY / 2, sizeX / 2, sizeY / 2);
			// uni_p
			updateUniform(
				0,
				g_pModuleManager->getRenderingFrontend()->getCSMGPUData()[splitCount].p);
			//uni_v
			updateUniform(
				1,
				g_pModuleManager->getRenderingFrontend()->getCSMGPUData()[splitCount].v);

			splitCount++;

			auto l_totalDrawCallCount = g_pModuleManager->getRenderingFrontend()->getOpaquePassDrawCallCount();
			for (unsigned int i = 0; i < l_totalDrawCallCount; i++)
			{
				auto l_opaquePassGPUData = g_pModuleManager->getRenderingFrontend()->getOpaquePassGPUData()[i];
				auto l_meshGPUData = g_pModuleManager->getRenderingFrontend()->getOpaquePassMeshGPUData()[i];

				//uni_m
				updateUniform(2, l_meshGPUData.m);

				drawMesh(reinterpret_cast<GLMeshDataComponent*>(l_opaquePassGPUData.MDC));
			}
		}
	}

	//////
	auto l_pointLightData = g_pModuleManager->getRenderingFrontend()->getPointLightGPUData()[0];
	auto l_capturePos = l_pointLightData.pos;
	auto l_attenuationRange = l_pointLightData.luminance.w;
	auto l_t = InnoMath::getInvertTranslationMatrix(l_capturePos);

	activateShaderProgram(m_PointLight_GLSPC);

	glBindFramebuffer(GL_FRAMEBUFFER, m_PointLight_GLRPC->m_FBO);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, m_PointLightDepth_GLTDC->m_TO, 0);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, m_PointLight_GLRPC->m_GLTDCs[0]->m_TO, 0);
	glDrawBuffer(GL_COLOR_ATTACHMENT0);

	activateRenderPass(m_PointLight_GLRPC);

	auto l_p = InnoMath::generatePerspectiveMatrix((90.0f / 180.0f) * PI<float>, 1.0f, 0.1f, l_attenuationRange);

	auto l_rPX = InnoMath::toRotationMatrix(InnoMath::getQuatRotator(vec4(0.0f, 1.0f, 0.0f, 0.0f), -90.0f)).inverse();
	auto l_rNX = InnoMath::toRotationMatrix(InnoMath::getQuatRotator(vec4(0.0f, 1.0f, 0.0f, 0.0f), 90.0f)).inverse();
	auto l_rPY = InnoMath::toRotationMatrix(InnoMath::getQuatRotator(vec4(1.0f, 0.0f, 0.0f, 0.0f), -90.0f)).inverse();
	auto l_rNY = InnoMath::toRotationMatrix(InnoMath::getQuatRotator(vec4(1.0f, 0.0f, 0.0f, 0.0f), 90.0f)).inverse();
	auto l_rPZ = InnoMath::toRotationMatrix(InnoMath::getQuatRotator(vec4(0.0f, 1.0f, 0.0f, 0.0f), 0.0f)).inverse();
	auto l_rNZ = InnoMath::toRotationMatrix(InnoMath::getQuatRotator(vec4(0.0f, 1.0f, 0.0f, 0.0f), 180.0f)).inverse();

	std::vector<mat4> l_r =
	{
		l_rPX, l_rNX, l_rPY, l_rNY, l_rPZ, l_rNZ
	};

	std::vector<mat4> l_prt(6);
	for (size_t i = 0; i < l_r.size(); i++)
	{
		l_prt[i] = l_p * l_r[i] * l_t;
	}

	updateUniform(1, l_prt);

	auto l_totalDrawCallCount = g_pModuleManager->getRenderingFrontend()->getOpaquePassDrawCallCount();
	for (unsigned int i = 0; i < l_totalDrawCallCount; i++)
	{
		auto l_opaquePassGPUData = g_pModuleManager->getRenderingFrontend()->getOpaquePassGPUData()[i];
		auto l_meshGPUData = g_pModuleManager->getRenderingFrontend()->getOpaquePassMeshGPUData()[i];

		//uni_m
		updateUniform(0, l_meshGPUData.m);

		drawMesh(reinterpret_cast<GLMeshDataComponent*>(l_opaquePassGPUData.MDC));
	}

	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);
}

bool GLShadowPass::reloadShader()
{
	deleteShaderProgram(m_DirLight_GLSPC);
	deleteShaderProgram(m_PointLight_GLSPC);

	initializeGLShaderProgramComponent(m_DirLight_GLSPC, m_DirLightShaderFilePaths);
	initializeGLShaderProgramComponent(m_PointLight_GLSPC, m_PointLightShaderFilePaths);

	return true;
}

GLRenderPassComponent * GLShadowPass::getGLRPC(unsigned int index)
{
	if (index == 0)
	{
		return m_DirLight_GLRPC;
	}
	else if (index == 1)
	{
		return m_PointLight_GLRPC;
	}
	else
	{
		return nullptr;
	}
}