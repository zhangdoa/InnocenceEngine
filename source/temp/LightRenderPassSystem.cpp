#include "LightPass.h"

void LightPass::setup()
{
	//m_lightPassShaderProgram = g_pMemorySystem->spawn<LightPassBlinnPhongShaderProgram>();
	m_lightPassShaderProgram = g_pMemorySystem->spawn<LightPassPBSShaderProgram>();
}

void LightPass::initialize()
{
//	// initialize shader
//	//auto l_vertexShaderFilePath = "GL3.3/lightPassBlinnPhongVertex.sf";
//	auto l_vertexShaderFilePath = "GL3.3/lightPassPBSVertex.sf";
//	auto l_vertexShaderData = shaderData(shaderType::VERTEX, shaderCodeContentPair(l_vertexShaderFilePath, g_pAssetSystem->loadShader(l_vertexShaderFilePath)));
//	//auto l_fragmentShaderFilePath = "GL3.3/lightPassNormalizedBlinnPhongFragment.sf";
//	//auto l_fragmentShaderFilePath = "GL3.3/lightPassBlinnPhongFragment.sf";
//	auto l_fragmentShaderFilePath = "GL3.3/lightPassPBSFragment.sf";
//	auto l_fragmentShaderData = shaderData(shaderType::FRAGMENT, shaderCodeContentPair(l_fragmentShaderFilePath, g_pAssetSystem->loadShader(l_fragmentShaderFilePath)));
//	m_lightPassShaderProgram->setup({ l_vertexShaderData , l_fragmentShaderData });
//	m_lightPassShaderProgram->initialize();
//
//	// initialize texture
//	m_lightPassTextureID = this->addTexture(textureType::RENDER_BUFFER_SAMPLER);
//	auto l_lightPassTextureData = this->getTexture(textureType::RENDER_BUFFER_SAMPLER, m_lightPassTextureID);
//	l_lightPassTextureData->setup(textureType::RENDER_BUFFER_SAMPLER, textureColorComponentsFormat::RGBA16F, texturePixelDataFormat::RGBA, textureWrapMethod::CLAMP_TO_EDGE, textureFilterMethod::NEAREST, textureFilterMethod::NEAREST, (int)m_screenResolution.x, (int)m_screenResolution.y, texturePixelDataType::FLOAT, std::vector<void*>{ nullptr });
//
//	// initialize frame buffer
//	auto l_renderBufferStorageSizes = std::vector<vec2>{ m_screenResolution };
//	auto l_renderTargetTextures = std::vector<BaseTexture*>{ l_lightPassTextureData };
//	m_lightPassFrameBuffer = g_pMemorySystem->spawn<FRAMEBUFFER_CLASS>();
//	m_lightPassFrameBuffer->setup(frameBufferType::DEFER, renderBufferType::DEPTH_AND_STENCIL, l_renderBufferStorageSizes, l_renderTargetTextures);
//	m_lightPassFrameBuffer->initialize();
}

void LightPass::draw()
{
//	// bind to framebuffer
//
//	/////////Blinn-Phong
//	if (!isPointLightUniformAdded)
//	{
//		int l_pointLightIndexOffset = 0;
//		for (auto i = (unsigned int)0; i < lightComponents.size(); i++)
//		{
//			if (lightComponents[i]->getLightType() == lightType::DIRECTIONAL)
//			{
//				l_pointLightIndexOffset -= 1;
//			}
//			if (lightComponents[i]->getLightType() == lightType::POINT)
//			{
//				std::stringstream ss;
//				ss << i + l_pointLightIndexOffset;
//				m_uni_pointLights_position.emplace_back(getUniformLocation("uni_pointLights[" + ss.str() + "].position"));
//				m_uni_pointLights_radius.emplace_back(getUniformLocation("uni_pointLights[" + ss.str() + "].radius"));
//				m_uni_pointLights_color.emplace_back(getUniformLocation("uni_pointLights[" + ss.str() + "].color"));
//			}
//		}
//		isPointLightUniformAdded = true;
//	}
//
//	int l_pointLightIndexOffset = 0;
//	for (auto i = (unsigned int)0; i < lightComponents.size(); i++)
//	{
//		//@TODO: generalization
//
//		updateUniform(m_uni_viewPos, cameraComponents[0]->getParentEntity()->getTransform()->getPos().x, cameraComponents[0]->getParentEntity()->getTransform()->getPos().y, cameraComponents[0]->getParentEntity()->getTransform()->getPos().z);
//
//		if (lightComponents[i]->getLightType() == lightType::DIRECTIONAL)
//		{
//			l_pointLightIndexOffset -= 1;
//			updateUniform(m_uni_dirLight_direction, lightComponents[i]->getDirection().x, lightComponents[i]->getDirection().y, lightComponents[i]->getDirection().z);
//			updateUniform(m_uni_dirLight_color, lightComponents[i]->getColor().x, lightComponents[i]->getColor().y, lightComponents[i]->getColor().z);
//		}
//		else if (lightComponents[i]->getLightType() == lightType::POINT)
//		{
//			updateUniform(m_uni_pointLights_position[i + l_pointLightIndexOffset], lightComponents[i]->getParentEntity()->getTransform()->getPos().x, lightComponents[i]->getParentEntity()->getTransform()->getPos().y, lightComponents[i]->getParentEntity()->getTransform()->getPos().z);
//			updateUniform(m_uni_pointLights_radius[i + l_pointLightIndexOffset], lightComponents[i]->getRadius());
//			updateUniform(m_uni_pointLights_color[i + l_pointLightIndexOffset], lightComponents[i]->getColor().x, lightComponents[i]->getColor().y, lightComponents[i]->getColor().z);
//		}
//	}
//
//
//	//////////////////// Cook-Torrance
//	// world space position + metallic
//	m_geometryPassFrameBuffer->activeRenderTargetTexture(0, 0);
//	// normal + roughness
//	m_geometryPassFrameBuffer->activeRenderTargetTexture(1, 1);
//	// albedo + ambient occlusion
//	m_geometryPassFrameBuffer->activeRenderTargetTexture(2, 2);
//	// light space position
//	m_geometryPassFrameBuffer->activeRenderTargetTexture(3, 3);
//	// shadow map
//	m_shadowForwardPassFrameBuffer->activeRenderTargetTexture(0, 4);
//	// irradiance environment map
//	m_environmentPassFrameBuffer->activeRenderTargetTexture(1, 5);
//	// pre-filter specular environment map
//	m_environmentPassFrameBuffer->activeRenderTargetTexture(2, 6);
//	// BRDF look-up table
//	m_environmentPassFrameBuffer->activeRenderTargetTexture(3, 7);
//
//	m_geometryPassFrameBuffer->bindAsReadBuffer();
//	m_lightPassFrameBuffer->bindAsWriteBuffer(m_screenResolution, m_screenResolution);
//
//	m_lightPassFrameBuffer->update(true, true);
//	m_lightPassFrameBuffer->setRenderBufferStorageSize(0);
//
//	m_lightPassShaderProgram->update();
//
//	if (lightComponents.size() > 0)
//	{
//		if (!isPointLightUniformAdded)
//		{
//			int l_pointLightIndexOffset = 0;
//			for (auto i = (unsigned int)0; i < lightComponents.size(); i++)
//			{
//				if (lightComponents[i]->getLightType() == lightType::DIRECTIONAL)
//				{
//					l_pointLightIndexOffset -= 1;
//				}
//				if (lightComponents[i]->getLightType() == lightType::POINT)
//				{
//					std::stringstream ss;
//					ss << i + l_pointLightIndexOffset;
//					m_uni_pointLights_position.emplace_back(getUniformLocation("uni_pointLights[" + ss.str() + "].position"));
//					m_uni_pointLights_radius.emplace_back(getUniformLocation("uni_pointLights[" + ss.str() + "].radius"));
//					m_uni_pointLights_color.emplace_back(getUniformLocation("uni_pointLights[" + ss.str() + "].color"));
//				}
//			}
//			isPointLightUniformAdded = true;
//		}
//
//		int l_pointLightIndexOffset = 0;
//		for (auto i = (unsigned int)0; i < lightComponents.size(); i++)
//		{
//			//@TODO: generalization
//
//			updateUniform(m_uni_viewPos, cameraComponents[0]->getParentEntity()->getTransform()->getPos().x, cameraComponents[0]->getParentEntity()->getTransform()->getPos().y, cameraComponents[0]->getParentEntity()->getTransform()->getPos().z);
//			updateUniform(m_uni_textureMode, (int)in_shaderDrawPair.second);
//
//			if (lightComponents[i]->getLightType() == lightType::DIRECTIONAL)
//			{
//				l_pointLightIndexOffset -= 1;
//				updateUniform(m_uni_dirLight_position, lightComponents[i]->getParentEntity()->getTransform()->getPos().x, lightComponents[i]->getParentEntity()->getTransform()->getPos().y, lightComponents[i]->getParentEntity()->getTransform()->getPos().z);
//				updateUniform(m_uni_dirLight_direction, lightComponents[i]->getDirection().x, lightComponents[i]->getDirection().y, lightComponents[i]->getDirection().z);
//				updateUniform(m_uni_dirLight_color, lightComponents[i]->getColor().x, lightComponents[i]->getColor().y, lightComponents[i]->getColor().z);
//			}
//			else if (lightComponents[i]->getLightType() == lightType::POINT)
//			{
//				updateUniform(m_uni_pointLights_position[i + l_pointLightIndexOffset], lightComponents[i]->getParentEntity()->getTransform()->getPos().x, lightComponents[i]->getParentEntity()->getTransform()->getPos().y, lightComponents[i]->getParentEntity()->getTransform()->getPos().z);
//				updateUniform(m_uni_pointLights_radius[i + l_pointLightIndexOffset], lightComponents[i]->getRadius());
//				updateUniform(m_uni_pointLights_color[i + l_pointLightIndexOffset], lightComponents[i]->getColor().x, lightComponents[i]->getColor().y, lightComponents[i]->getColor().z);
//			}
//		}
//	}
//
//	// draw light pass rectangle
//	this->getMesh(meshType::TWO_DIMENSION, m_Unit2DQuadTemplate)->update();
//
//	glDisable(GL_STENCIL_TEST);
}

void LightPass::shutdown()
{
}

const objectStatus & LightPass::getStatus() const
{
	return m_objectStatus;
}
