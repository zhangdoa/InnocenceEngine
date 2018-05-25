#include "ShadowPass.h"

void ShadowPass::setup()
{
	m_shadowForwardPassShaderProgram = g_pMemorySystem->spawn<ShadowForwardPassShaderProgram>();
}

void ShadowPass::initialize()
{
	// initialize shader
	auto l_shadowForwardPassVertexShader = g_pMemorySystem->spawn<SHADER_CLASS>();
	l_shadowForwardPassVertexShader->setup(shaderData(shaderType::VERTEX, "GL3.3/shadowForwardPassVertex.sf", g_pAssetSystem->loadShader("GL3.3/shadowForwardPassVertex.sf")));
	auto l_shadowForwardPassFragmentShader = g_pMemorySystem->spawn<SHADER_CLASS>();
	l_shadowForwardPassFragmentShader->setup(shaderData(shaderType::FRAGMENT, "GL3.3/shadowForwardPassFragment.sf", g_pAssetSystem->loadShader("GL3.3/shadowForwardPassFragment.sf")));
	m_shadowForwardPassShaderProgram->setup({ l_shadowForwardPassVertexShader , l_shadowForwardPassFragmentShader });
	m_shadowForwardPassShaderProgram->initialize();

	// initialize texture
	m_shadowForwardPassTextureID_L0 = g_pRenderingSystem->addTexture(textureType::SHADOWMAP);
	auto l_shadowForwardPassTextureData_L0 = g_pRenderingSystem->getTexture(textureType::SHADOWMAP, m_shadowForwardPassTextureID_L0);
	l_shadowForwardPassTextureData_L0->setup(textureType::SHADOWMAP, textureColorComponentsFormat::DEPTH_COMPONENT, texturePixelDataFormat::DEPTH_COMPONENT, textureWrapMethod::CLAMP_TO_BORDER, textureFilterMethod::NEAREST, textureFilterMethod::NEAREST, 2048, 2048, texturePixelDataType::FLOAT, std::vector<void*>{ nullptr });

	m_shadowForwardPassTextureID_L1 = g_pRenderingSystem->addTexture(textureType::SHADOWMAP);
	auto l_shadowForwardPassTextureData_L1 = g_pRenderingSystem->getTexture(textureType::SHADOWMAP, m_shadowForwardPassTextureID_L1);
	l_shadowForwardPassTextureData_L1->setup(textureType::SHADOWMAP, textureColorComponentsFormat::DEPTH_COMPONENT, texturePixelDataFormat::DEPTH_COMPONENT, textureWrapMethod::CLAMP_TO_BORDER, textureFilterMethod::NEAREST, textureFilterMethod::NEAREST, 2048, 2048, texturePixelDataType::FLOAT, std::vector<void*>{ nullptr });

	m_shadowForwardPassTextureID_L2 = g_pRenderingSystem->addTexture(textureType::SHADOWMAP);
	auto l_shadowForwardPassTextureData_L2 = g_pRenderingSystem->getTexture(textureType::SHADOWMAP, m_shadowForwardPassTextureID_L2);
	l_shadowForwardPassTextureData_L2->setup(textureType::SHADOWMAP, textureColorComponentsFormat::DEPTH_COMPONENT, texturePixelDataFormat::DEPTH_COMPONENT, textureWrapMethod::CLAMP_TO_BORDER, textureFilterMethod::NEAREST, textureFilterMethod::NEAREST, 2048, 2048, texturePixelDataType::FLOAT, std::vector<void*>{ nullptr });

	m_shadowForwardPassTextureID_L3 = g_pRenderingSystem->addTexture(textureType::SHADOWMAP);
	auto l_shadowForwardPassTextureData_L3 = g_pRenderingSystem->getTexture(textureType::SHADOWMAP, m_shadowForwardPassTextureID_L3);
	l_shadowForwardPassTextureData_L3->setup(textureType::SHADOWMAP, textureColorComponentsFormat::DEPTH_COMPONENT, texturePixelDataFormat::DEPTH_COMPONENT, textureWrapMethod::CLAMP_TO_BORDER, textureFilterMethod::NEAREST, textureFilterMethod::NEAREST, 2048, 2048, texturePixelDataType::FLOAT, std::vector<void*>{ nullptr });

	// initialize framebuffer
	auto l_shadowForwardPassRenderBufferStorageSizes = std::vector<vec2>{ vec2(2048, 2048) };
	auto l_shadowForwardPassRenderTargetTextures = std::vector<BaseTexture*>{ l_shadowForwardPassTextureData_L0, l_shadowForwardPassTextureData_L1, l_shadowForwardPassTextureData_L2, l_shadowForwardPassTextureData_L3 };
	m_shadowForwardPassFrameBuffer = g_pMemorySystem->spawn<FRAMEBUFFER_CLASS>();
	m_shadowForwardPassFrameBuffer->setup(frameBufferType::SHADOW_PASS, renderBufferType::DEPTH, l_shadowForwardPassRenderBufferStorageSizes, l_shadowForwardPassRenderTargetTextures);
	m_shadowForwardPassFrameBuffer->initialize();
}

void ShadowPass::update()
{
	//// bind to framebuffer
	//m_shadowForwardPassFrameBuffer->update(true, true);
	//m_shadowForwardPassFrameBuffer->setRenderBufferStorageSize(0);

	//m_shadowForwardPassShaderProgram->update();

	//// draw each lightComponent's shadowmap
	//for (size_t i = 0; i < 4; i++)
	//{
	//	for (auto& l_lightComponent : lightComponents)
	//	{
	//		if (l_lightComponent->getLightType() == lightType::DIRECTIONAL)
	//		{
	//			updateUniform(m_uni_p, l_lightComponent->getProjectionMatrix(i));
	//			updateUniform(m_uni_v, l_lightComponent->getViewMatrix());

	//			// draw each visibleComponent
	//			for (auto& l_visibleComponent : visibleComponents)
	//			{
	//				if (l_visibleComponent->m_visiblilityType == visiblilityType::STATIC_MESH)
	//				{
	//					updateUniform(m_uni_m, l_visibleComponent->getParentEntity()->caclTransformationMatrix());

	//					// draw each graphic data of visibleComponent
	//					for (auto& l_graphicData : l_visibleComponent->getModelMap())
	//					{
	//						// draw meshes
	//						meshMap.find(l_graphicData.first)->second->update();
	//					}
	//				}
	//			}
	//		}
	//	}
	//}

	//glDisable(GL_DEPTH_TEST);
	//glDisable(GL_CULL_FACE);
}

void ShadowPass::shutdown()
{
}

const objectStatus & ShadowPass::getStatus() const
{
	return m_objectStatus;
}
