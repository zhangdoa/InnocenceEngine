#include "GeometryPass.h"

void GeometryPass::setup()
{
	//@TODO: add a switch for different shader model
	//m_geometryPassShaderProgram = g_pMemorySystem->spawn<GeometryPassBlinnPhongShaderProgram>();
	m_geometryPassShaderProgram = g_pMemorySystem->spawn<GeometryPassPBSShaderProgram>();
}

void GeometryPass::initialize()
{
	//// initialize shader
	////auto l_vertexShaderFilePath = "GL3.3/geometryPassBlinnPhongVertex.sf";
	//auto l_vertexShaderFilePath = "GL3.3/geometryPassPBSVertex.sf";
	//auto l_vertexShaderData = shaderData(shaderType::VERTEX, shaderCodeContentPair(l_vertexShaderFilePath, g_pAssetSystem->loadShader(l_vertexShaderFilePath)));
	////auto l_fragmentShaderFilePath = "GL3.3/geometryPassBlinnPhongFragment.sf";
	//auto l_fragmentShaderFilePath = "GL3.3/geometryPassPBSFragment.sf";
	//auto l_fragmentShaderData = shaderData(shaderType::FRAGMENT, shaderCodeContentPair(l_fragmentShaderFilePath, g_pAssetSystem->loadShader(l_fragmentShaderFilePath)));
	//m_geometryPassShaderProgram->setup({ l_vertexShaderData , l_fragmentShaderData });
	//m_geometryPassShaderProgram->initialize();

	//// initialize texture
	//m_geometryPassRT0TextureID = this->addTexture(textureType::RENDER_BUFFER_SAMPLER);
	//auto l_geometryPassRT0TextureData = this->getTexture(textureType::RENDER_BUFFER_SAMPLER, m_geometryPassRT0TextureID);
	//l_geometryPassRT0TextureData->setup(textureType::RENDER_BUFFER_SAMPLER, textureColorComponentsFormat::RGBA16F, texturePixelDataFormat::RGBA, textureWrapMethod::CLAMP_TO_EDGE, textureFilterMethod::NEAREST, textureFilterMethod::NEAREST, (int)m_screenResolution.x, (int)m_screenResolution.y, texturePixelDataType::FLOAT, std::vector<void*>{ nullptr });

	//m_geometryPassRT1TextureID = this->addTexture(textureType::RENDER_BUFFER_SAMPLER);
	//auto l_geometryPassRT1TextureData = this->getTexture(textureType::RENDER_BUFFER_SAMPLER, m_geometryPassRT1TextureID);
	//l_geometryPassRT1TextureData->setup(textureType::RENDER_BUFFER_SAMPLER, textureColorComponentsFormat::RGBA16F, texturePixelDataFormat::RGBA, textureWrapMethod::CLAMP_TO_EDGE, textureFilterMethod::NEAREST, textureFilterMethod::NEAREST, (int)m_screenResolution.x, (int)m_screenResolution.y, texturePixelDataType::FLOAT, std::vector<void*>{ nullptr });

	//m_geometryPassRT2TextureID = this->addTexture(textureType::RENDER_BUFFER_SAMPLER);
	//auto l_geometryPassRT2TextureData = this->getTexture(textureType::RENDER_BUFFER_SAMPLER, m_geometryPassRT2TextureID);
	//l_geometryPassRT2TextureData->setup(textureType::RENDER_BUFFER_SAMPLER, textureColorComponentsFormat::RGBA16F, texturePixelDataFormat::RGBA, textureWrapMethod::CLAMP_TO_EDGE, textureFilterMethod::NEAREST, textureFilterMethod::NEAREST, (int)m_screenResolution.x, (int)m_screenResolution.y, texturePixelDataType::FLOAT, std::vector<void*>{ nullptr });

	//m_geometryPassRT3TextureID = this->addTexture(textureType::RENDER_BUFFER_SAMPLER);
	//auto l_geometryPassRT3TextureData = this->getTexture(textureType::RENDER_BUFFER_SAMPLER, m_geometryPassRT3TextureID);
	//l_geometryPassRT3TextureData->setup(textureType::RENDER_BUFFER_SAMPLER, textureColorComponentsFormat::RGBA16F, texturePixelDataFormat::RGBA, textureWrapMethod::CLAMP_TO_EDGE, textureFilterMethod::NEAREST, textureFilterMethod::NEAREST, (int)m_screenResolution.x, (int)m_screenResolution.y, texturePixelDataType::FLOAT, std::vector<void*>{ nullptr });

	//// initialize frame buffer
	//auto l_renderBufferStorageSizes = std::vector<vec2>{ m_screenResolution };
	//auto l_renderTargetTextures = std::vector<BaseTexture*>{ l_geometryPassRT0TextureData , l_geometryPassRT1TextureData , l_geometryPassRT2TextureData , l_geometryPassRT3TextureData };
	//m_geometryPassFrameBuffer = g_pMemorySystem->spawn<FRAMEBUFFER_CLASS>();
	//m_geometryPassFrameBuffer->setup(frameBufferType::FORWARD, renderBufferType::DEPTH_AND_STENCIL, l_renderBufferStorageSizes, l_renderTargetTextures);
	//m_geometryPassFrameBuffer->initialize();
}

void GeometryPass::draw()
{
	//// bind to framebuffer
	//m_geometryPassFrameBuffer->update(true, true);
	//m_geometryPassFrameBuffer->setRenderBufferStorageSize(0);

	//m_geometryPassShaderProgram->update();

	//mat4 p = cameraComponents[0]->m_projectionMatrix;
	//mat4 r = cameraComponents[0]->getInvertRotationMatrix();
	//mat4 t = cameraComponents[0]->getInvertTranslationMatrix();

	//updateUniform(m_uni_p, p);
	//updateUniform(m_uni_r, r);
	//updateUniform(m_uni_t, t);

	///////////////////Blinn-Phong
	//// draw each visibleComponent
	//for (auto& l_visibleComponent : visibleComponents)
	//{
	//	if (l_visibleComponent->m_visiblilityType == visiblilityType::STATIC_MESH)
	//	{
	//		updateUniform(m_uni_m, l_visibleComponent->getParentEntity()->caclTransformationMatrix());

	//		// draw each graphic data of visibleComponent
	//		for (auto& l_graphicData : l_visibleComponent->getModelMap())
	//		{
	//			//active and bind textures
	//			// is there any texture?
	//			auto l_textureMap = &l_graphicData.second;
	//			if (l_textureMap != nullptr)
	//			{
	//				// any normal?
	//				auto l_normalTextureID = l_textureMap->find(textureType::NORMAL);
	//				if (l_normalTextureID != l_textureMap->end())
	//				{
	//					auto l_textureData = textureMap.find(l_normalTextureID->second)->second;
	//					l_textureData->update(0);
	//				}
	//				// any diffuse?
	//				auto l_diffuseTextureID = l_textureMap->find(textureType::ALBEDO);
	//				if (l_diffuseTextureID != l_textureMap->end())
	//				{
	//					auto l_textureData = textureMap.find(l_diffuseTextureID->second)->second;
	//					l_textureData->update(1);
	//				}
	//				// any specular?
	//				auto l_specularTextureID = l_textureMap->find(textureType::METALLIC);
	//				if (l_specularTextureID != l_textureMap->end())
	//				{
	//					auto l_textureData = textureMap.find(l_specularTextureID->second)->second;
	//					l_textureData->update(2);
	//				}
	//			}
	//			// draw meshes
	//			meshMap.find(l_graphicData.first)->second->update();
	//		}
	//	}
	//}
	//glDisable(GL_DEPTH_TEST);
	//glDisable(GL_DEPTH_CLAMP);

	/////////////////Cook-Torrance
	//if (cameraComponents.size() > 0)
	//{
	//	mat4 p = cameraComponents[0]->m_projectionMatrix;
	//	mat4 r = cameraComponents[0]->getInvertRotationMatrix();
	//	mat4 t = cameraComponents[0]->getInvertTranslationMatrix();
	//	updateUniform(m_uni_p, p);
	//	updateUniform(m_uni_r, r);
	//	updateUniform(m_uni_t, t);
	//}

	//if (lightComponents.size() > 0)
	//{
	//	for (auto& l_lightComponent : lightComponents)
	//	{
	//		// update light space transformation matrices
	//		if (l_lightComponent->getLightType() == lightType::DIRECTIONAL)
	//		{
	//			updateUniform(m_uni_p_light, l_lightComponent->getProjectionMatrix(0));
	//			updateUniform(m_uni_v_light, l_lightComponent->getViewMatrix());

	//			// draw each visibleComponent
	//			for (auto& l_visibleComponent : visibleComponents)
	//			{
	//				if (l_visibleComponent->m_visiblilityType == visiblilityType::STATIC_MESH)
	//				{
	//					glStencilFunc(GL_ALWAYS, 0x01, 0xFF);

	//					updateUniform(m_uni_m, l_visibleComponent->getParentEntity()->caclTransformationMatrix());

	//					// draw each graphic data of visibleComponent
	//					for (auto& l_graphicData : l_visibleComponent->getModelMap())
	//					{
	//						//active and bind textures
	//						// is there any texture?
	//						auto l_textureMap = &l_graphicData.second;
	//						if (l_textureMap != nullptr)
	//						{
	//							// any normal?
	//							auto l_normalTextureID = l_textureMap->find(textureType::NORMAL);
	//							if (l_normalTextureID != l_textureMap->end())
	//							{
	//								auto l_textureData = textureMap.find(l_normalTextureID->second)->second;
	//								l_textureData->update(0);
	//							}
	//							// any albedo?
	//							auto l_albedoTextureID = l_textureMap->find(textureType::ALBEDO);
	//							if (l_albedoTextureID != l_textureMap->end())
	//							{
	//								auto l_textureData = textureMap.find(l_albedoTextureID->second)->second;
	//								l_textureData->update(1);
	//							}
	//							// any metallic?
	//							auto l_metallicTextureID = l_textureMap->find(textureType::METALLIC);
	//							if (l_metallicTextureID != l_textureMap->end())
	//							{
	//								auto l_textureData = textureMap.find(l_metallicTextureID->second)->second;
	//								l_textureData->update(2);
	//							}
	//							// any roughness?
	//							auto l_roughnessTextureID = l_textureMap->find(textureType::ROUGHNESS);
	//							if (l_roughnessTextureID != l_textureMap->end())
	//							{
	//								auto l_textureData = textureMap.find(l_roughnessTextureID->second)->second;
	//								l_textureData->update(3);
	//							}
	//							// any ao?
	//							auto l_aoTextureID = l_textureMap->find(textureType::AMBIENT_OCCLUSION);
	//							if (l_aoTextureID != l_textureMap->end())
	//							{
	//								auto l_textureData = textureMap.find(l_aoTextureID->second)->second;
	//								l_textureData->update(4);
	//							}
	//						}
	//						updateUniform(m_uni_useTexture, l_visibleComponent->m_useTexture);
	//						updateUniform(m_uni_albedo, l_visibleComponent->m_albedo.x, l_visibleComponent->m_albedo.y, l_visibleComponent->m_albedo.z);
	//						updateUniform(m_uni_MRA, l_visibleComponent->m_MRA.x, l_visibleComponent->m_MRA.y, l_visibleComponent->m_MRA.z);
	//						// draw meshes
	//						meshMap.find(l_graphicData.first)->second->update();
	//					}
	//				}
	//				else if (l_visibleComponent->m_visiblilityType == visiblilityType::EMISSIVE)
	//				{
	//					glStencilFunc(GL_ALWAYS, 0x02, 0xFF);

	//					updateUniform(m_uni_m, l_visibleComponent->getParentEntity()->caclTransformationMatrix());

	//					// draw each graphic data of visibleComponent
	//					for (auto& l_graphicData : l_visibleComponent->getModelMap())
	//					{
	//						updateUniform(m_uni_useTexture, l_visibleComponent->m_useTexture);
	//						updateUniform(m_uni_albedo, l_visibleComponent->m_albedo.x, l_visibleComponent->m_albedo.y, l_visibleComponent->m_albedo.z);
	//						// draw meshes
	//						meshMap.find(l_graphicData.first)->second->update();
	//					}
	//				}
	//				else
	//				{
	//					glStencilFunc(GL_ALWAYS, 0x00, 0xFF);
	//				}
	//			}
	//		}
	//	}
	//}

	//glDisable(GL_DEPTH_TEST);
	//glDisable(GL_DEPTH_CLAMP);
	//glDisable(GL_STENCIL_TEST);
	//glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

void GeometryPass::shutdown()
{
}

const objectStatus & GeometryPass::getStatus() const
{
	return m_objectStatus;
}
