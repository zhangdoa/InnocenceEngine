#include "FinalPass.h"

void FinalPass::setup()
{
	m_skyPassShaderProgram = g_pMemorySystem->spawn<SkyPassShaderProgram>();
	m_bloomExtractPassShaderProgram = g_pMemorySystem->spawn<BloomExtractPassShaderProgram>();
	m_bloomBlurPassShaderProgram = g_pMemorySystem->spawn<BloomBlurPassShaderProgram>();
	m_billboardPassShaderProgram = g_pMemorySystem->spawn<BillboardPassShaderProgram>();
	m_debuggerPassShaderProgram = g_pMemorySystem->spawn<DebuggerShaderProgram>();
	m_finalPassShaderProgram = g_pMemorySystem->spawn<FinalPassShaderProgram>();
}

void FinalPass::initialize()
{
	//// sky pass
	//// initialize shader
	//auto l_skyPassVertexShaderFilePath = "GL3.3/skyPassVertex.sf";
	//auto l_skyPassVertexShaderData = shaderData(shaderType::VERTEX, shaderCodeContentPair(l_skyPassVertexShaderFilePath, g_pAssetSystem->loadShader(l_skyPassVertexShaderFilePath)));
	//auto l_skyPassFragmentShaderFilePath = "GL3.3/skyPassFragment.sf";
	//auto l_skyPassFragmentShaderData = shaderData(shaderType::FRAGMENT, shaderCodeContentPair(l_skyPassFragmentShaderFilePath, g_pAssetSystem->loadShader(l_skyPassFragmentShaderFilePath)));
	//m_skyPassShaderProgram->setup({ l_skyPassVertexShaderData , l_skyPassFragmentShaderData });
	//m_skyPassShaderProgram->initialize();

	//// initialize texture
	//m_skyPassTextureID = g_pRenderingSystem->addTexture(textureType::RENDER_BUFFER_SAMPLER);
	//auto l_skyPassTextureData = g_pRenderingSystem->getTexture(textureType::RENDER_BUFFER_SAMPLER, m_skyPassTextureID);
	//l_skyPassTextureData->setup(textureType::RENDER_BUFFER_SAMPLER, textureColorComponentsFormat::RGBA16F, texturePixelDataFormat::RGBA, textureWrapMethod::CLAMP_TO_EDGE, textureFilterMethod::NEAREST, textureFilterMethod::NEAREST, (int)m_screenResolution.x, (int)m_screenResolution.y, texturePixelDataType::FLOAT, std::vector<void*>{ nullptr });

	//// initialize framebuffer
	//auto l_skyPassRenderBufferStorageSizes = std::vector<vec2>{ m_screenResolution };
	//auto l_skyPassRenderTargetTextures = std::vector<BaseTexture*>{ l_skyPassTextureData };
	//m_skyPassFrameBuffer = g_pMemorySystem->spawn<FRAMEBUFFER_CLASS>();
	//m_skyPassFrameBuffer->setup(frameBufferType::FORWARD, renderBufferType::NONE, l_skyPassRenderBufferStorageSizes, l_skyPassRenderTargetTextures);
	//m_skyPassFrameBuffer->initialize();

	//// bloom pass
	//// initialize bloom extract pass shader
	//auto l_bloomExtractPassVertexShaderFilePath = "GL3.3/bloomExtractPassVertex.sf";
	//auto l_bloomExtractPassVertexShaderData = shaderData(shaderType::VERTEX, shaderCodeContentPair(l_bloomExtractPassVertexShaderFilePath, g_pAssetSystem->loadShader(l_bloomExtractPassVertexShaderFilePath)));
	//auto l_bloomExtractPassFragmentShaderFilePath = "GL3.3/bloomExtractPassFragment.sf";
	//auto l_bloomExtractPassFragmentShaderData = shaderData(shaderType::FRAGMENT, shaderCodeContentPair(l_bloomExtractPassFragmentShaderFilePath, g_pAssetSystem->loadShader(l_bloomExtractPassFragmentShaderFilePath)));
	//m_bloomExtractPassShaderProgram->setup({ l_bloomExtractPassVertexShaderData , l_bloomExtractPassFragmentShaderData });
	//m_bloomExtractPassShaderProgram->initialize();

	//// initialize texture
	//m_bloomExtractPassTextureID = g_pRenderingSystem->addTexture(textureType::RENDER_BUFFER_SAMPLER);
	//auto l_bloomExtractPassTextureData = g_pRenderingSystem->getTexture(textureType::RENDER_BUFFER_SAMPLER, m_bloomExtractPassTextureID);
	//l_bloomExtractPassTextureData->setup(textureType::RENDER_BUFFER_SAMPLER, textureColorComponentsFormat::RGBA16F, texturePixelDataFormat::RGBA, textureWrapMethod::CLAMP_TO_EDGE, textureFilterMethod::NEAREST, textureFilterMethod::NEAREST, (int)m_screenResolution.x, (int)m_screenResolution.y, texturePixelDataType::FLOAT, std::vector<void*>{ nullptr });

	//// initialize framebuffer
	//auto l_bloomExtractPassRenderBufferStorageSizes = std::vector<vec2>{ m_screenResolution };
	//auto l_bloomExtractPassRenderTargetTextures = std::vector<BaseTexture*>{ l_bloomExtractPassTextureData };
	//m_bloomExtractPassFrameBuffer = g_pMemorySystem->spawn<FRAMEBUFFER_CLASS>();
	//m_bloomExtractPassFrameBuffer->setup(frameBufferType::DEFER, renderBufferType::DEPTH_AND_STENCIL, l_bloomExtractPassRenderBufferStorageSizes, l_bloomExtractPassRenderTargetTextures);
	//m_bloomExtractPassFrameBuffer->initialize();

	//// initialize bloom blur pass shader
	//auto l_bloomBlurPassVertexShaderFilePath = "GL3.3/bloomBlurPassVertex.sf";
	//auto l_bloomBlurPassVertexShaderData = shaderData(shaderType::VERTEX, shaderCodeContentPair(l_bloomBlurPassVertexShaderFilePath, g_pAssetSystem->loadShader(l_bloomBlurPassVertexShaderFilePath)));
	//auto l_bloomBlurPassFragmentShaderFilePath = "GL3.3/bloomBlurPassFragment.sf";
	//auto l_bloomBlurPassFragmentShaderData = shaderData(shaderType::FRAGMENT, shaderCodeContentPair(l_bloomBlurPassFragmentShaderFilePath, g_pAssetSystem->loadShader(l_bloomBlurPassFragmentShaderFilePath)));
	//m_bloomBlurPassShaderProgram->setup({ l_bloomBlurPassVertexShaderData , l_bloomBlurPassFragmentShaderData });
	//m_bloomBlurPassShaderProgram->initialize();

	//// initialize ping texture
	//m_bloomBlurPassPingTextureID = g_pRenderingSystem->addTexture(textureType::RENDER_BUFFER_SAMPLER);
	//auto l_bloomBlurPassPingTextureData = g_pRenderingSystem->getTexture(textureType::RENDER_BUFFER_SAMPLER, m_bloomBlurPassPingTextureID);
	//l_bloomBlurPassPingTextureData->setup(textureType::RENDER_BUFFER_SAMPLER, textureColorComponentsFormat::RGBA16F, texturePixelDataFormat::RGBA, textureWrapMethod::CLAMP_TO_EDGE, textureFilterMethod::NEAREST, textureFilterMethod::NEAREST, (int)m_screenResolution.x, (int)m_screenResolution.y, texturePixelDataType::FLOAT, std::vector<void*>{ nullptr });

	//// initialize ping framebuffer
	//auto l_bloomBlurPassRenderBufferStorageSizes = std::vector<vec2>{ m_screenResolution };
	//auto l_bloomBlurPassPingRenderTargetTextures = std::vector<BaseTexture*>{ l_bloomBlurPassPingTextureData };
	//m_bloomBlurPassPingFrameBuffer = g_pMemorySystem->spawn<FRAMEBUFFER_CLASS>();
	//m_bloomBlurPassPingFrameBuffer->setup(frameBufferType::PINGPONG, renderBufferType::DEPTH_AND_STENCIL, l_bloomBlurPassRenderBufferStorageSizes, l_bloomBlurPassPingRenderTargetTextures);
	//m_bloomBlurPassPingFrameBuffer->initialize();

	//// initialize pong texture
	//m_bloomBlurPassPongTextureID = g_pRenderingSystem->addTexture(textureType::RENDER_BUFFER_SAMPLER);
	//auto l_bloomBlurPassPongTextureData = g_pRenderingSystem->getTexture(textureType::RENDER_BUFFER_SAMPLER, m_bloomBlurPassPongTextureID);
	//l_bloomBlurPassPongTextureData->setup(textureType::RENDER_BUFFER_SAMPLER, textureColorComponentsFormat::RGBA16F, texturePixelDataFormat::RGBA, textureWrapMethod::CLAMP_TO_EDGE, textureFilterMethod::NEAREST, textureFilterMethod::NEAREST, (int)m_screenResolution.x, (int)m_screenResolution.y, texturePixelDataType::FLOAT, std::vector<void*>{ nullptr });

	//// initialize pong framebuffer
	//auto l_bloomBlurPassPongRenderBufferStorageSizes = std::vector<vec2>{ m_screenResolution };
	//auto l_bloomBlurPassPongRenderTargetTextures = std::vector<BaseTexture*>{ l_bloomBlurPassPongTextureData };
	//m_bloomBlurPassPongFrameBuffer = g_pMemorySystem->spawn<FRAMEBUFFER_CLASS>();
	//m_bloomBlurPassPongFrameBuffer->setup(frameBufferType::PINGPONG, renderBufferType::DEPTH_AND_STENCIL, l_bloomBlurPassPongRenderBufferStorageSizes, l_bloomBlurPassPongRenderTargetTextures);
	//m_bloomBlurPassPongFrameBuffer->initialize();

	//// billboard pass
	//// initialize shader
	//auto l_billboardPassVertexShaderFilePath = "GL3.3/billboardPassVertex.sf";
	//auto l_billboardPassVertexShaderData = shaderData(shaderType::VERTEX, shaderCodeContentPair(l_billboardPassVertexShaderFilePath, g_pAssetSystem->loadShader(l_billboardPassVertexShaderFilePath)));
	//auto l_billboardPassFragmentShaderFilePath = "GL3.3/billboardPassFragment.sf";
	//auto l_billboardPassFragmentShaderData = shaderData(shaderType::FRAGMENT, shaderCodeContentPair(l_billboardPassFragmentShaderFilePath, g_pAssetSystem->loadShader(l_billboardPassFragmentShaderFilePath)));
	//m_billboardPassShaderProgram->setup({ l_billboardPassVertexShaderData , l_billboardPassFragmentShaderData });
	//m_billboardPassShaderProgram->initialize();

	//// initialize texture
	//m_billboardPassTextureID = g_pRenderingSystem->addTexture(textureType::RENDER_BUFFER_SAMPLER);
	//auto l_billboardPassTextureData = g_pRenderingSystem->getTexture(textureType::RENDER_BUFFER_SAMPLER, m_billboardPassTextureID);
	//l_billboardPassTextureData->setup(textureType::RENDER_BUFFER_SAMPLER, textureColorComponentsFormat::RGBA16F, texturePixelDataFormat::RGBA, textureWrapMethod::CLAMP_TO_EDGE, textureFilterMethod::NEAREST, textureFilterMethod::NEAREST, (int)m_screenResolution.x, (int)m_screenResolution.y, texturePixelDataType::FLOAT, std::vector<void*>{ nullptr });

	//// initialize framebuffer
	//auto l_billboardPassRenderBufferStorageSizes = std::vector<vec2>{ m_screenResolution };
	//auto l_billboardPassRenderTargetTextures = std::vector<BaseTexture*>{ l_billboardPassTextureData };
	//m_billboardPassFrameBuffer = g_pMemorySystem->spawn<FRAMEBUFFER_CLASS>();
	//m_billboardPassFrameBuffer->setup(frameBufferType::FORWARD, renderBufferType::DEPTH, l_billboardPassRenderBufferStorageSizes, l_billboardPassRenderTargetTextures);
	//m_billboardPassFrameBuffer->initialize();

	//// debugger pass
	//// initialize shader
	//auto l_debuggerPassVertexShaderFilePath = "GL3.3/debuggerVertex.sf";
	//auto l_debuggerPassVertexShaderData = shaderData(shaderType::VERTEX, shaderCodeContentPair(l_debuggerPassVertexShaderFilePath, g_pAssetSystem->loadShader(l_debuggerPassVertexShaderFilePath)));
	////auto l_debuggerPassGeometryShaderFilePath = "GL3.3/debuggerGeometry.sf";
	////auto l_debuggerPassGeometryShaderData = shaderData(shaderType::GEOMETRY, shaderCodeContentPair(l_debuggerPassGeometryShaderFilePath, g_pAssetSystem->loadShader(l_debuggerPassGeometryShaderFilePath)));
	//auto l_debuggerPassFragmentShaderFilePath = "GL3.3/debuggerFragment.sf";
	//auto l_debuggerPassFragmentShaderData = shaderData(shaderType::FRAGMENT, shaderCodeContentPair(l_debuggerPassFragmentShaderFilePath, g_pAssetSystem->loadShader(l_debuggerPassFragmentShaderFilePath)));
	////m_debuggerPassShaderProgram->setup({ l_debuggerPassVertexShaderData , l_debuggerPassGeometryShaderData , l_debuggerPassFragmentShaderData });
	//m_debuggerPassShaderProgram->setup({ l_debuggerPassVertexShaderData , l_debuggerPassFragmentShaderData });
	//m_debuggerPassShaderProgram->initialize();

	//// initialize texture
	//m_debuggerPassTextureID = g_pRenderingSystem->addTexture(textureType::RENDER_BUFFER_SAMPLER);
	//auto l_debuggerPassTextureData = g_pRenderingSystem->getTexture(textureType::RENDER_BUFFER_SAMPLER, m_debuggerPassTextureID);
	//l_debuggerPassTextureData->setup(textureType::RENDER_BUFFER_SAMPLER, textureColorComponentsFormat::RGBA16F, texturePixelDataFormat::RGBA, textureWrapMethod::CLAMP_TO_EDGE, textureFilterMethod::NEAREST, textureFilterMethod::NEAREST, (int)m_screenResolution.x, (int)m_screenResolution.y, texturePixelDataType::FLOAT, std::vector<void*>{ nullptr });

	//// initialize framebuffer
	//auto l_debuggerPassRenderBufferStorageSizes = std::vector<vec2>{ m_screenResolution };
	//auto l_debuggerPassRenderTargetTextures = std::vector<BaseTexture*>{ l_debuggerPassTextureData };
	//m_debuggerPassFrameBuffer = g_pMemorySystem->spawn<FRAMEBUFFER_CLASS>();
	//m_debuggerPassFrameBuffer->setup(frameBufferType::DEFER, renderBufferType::DEPTH, l_debuggerPassRenderBufferStorageSizes, l_debuggerPassRenderTargetTextures);
	//m_debuggerPassFrameBuffer->initialize();

	////post-process and final pass
	//// initialize shader
	//auto l_vertexShaderFilePath = "GL3.3/finalPassVertex.sf";
	//auto l_vertexShaderData = shaderData(shaderType::VERTEX, shaderCodeContentPair(l_vertexShaderFilePath, g_pAssetSystem->loadShader(l_vertexShaderFilePath)));
	//auto l_fragmentShaderFilePath = "GL3.3/finalPassFragment.sf";
	//auto l_fragmentShaderData = shaderData(shaderType::FRAGMENT, shaderCodeContentPair(l_fragmentShaderFilePath, g_pAssetSystem->loadShader(l_fragmentShaderFilePath)));
	//m_finalPassShaderProgram->setup({ l_vertexShaderData , l_fragmentShaderData });
	//m_finalPassShaderProgram->initialize();
}

void FinalPass::draw()
{
	//// bind to framebuffer
	//// draw sky pass
	//m_skyPassFrameBuffer->update(true, true);
	//m_skyPassFrameBuffer->setRenderBufferStorageSize(0);
	//m_skyPassShaderProgram->update();

	//if (cameraComponents.size() > 0)
	//{
	//	mat4 p = cameraComponents[0]->m_projectionMatrix;
	//	mat4 r = cameraComponents[0]->getInvertRotationMatrix();

	//	updateUniform(m_uni_p, p);
	//	updateUniform(m_uni_r, r);
	//}

	//if (visibleComponents.size() > 0)
	//{
	//	for (auto& l_visibleComponent : visibleComponents)
	//	{
	//		if (l_visibleComponent->m_visiblilityType == visiblilityType::SKYBOX)
	//		{
	//			for (auto& l_graphicData : l_visibleComponent->getModelMap())
	//			{
	//				auto l_cubeMapTexture = textureMap.find(l_graphicData.second.find(textureType::CUBEMAP)->second);
	//				if (l_cubeMapTexture != textureMap.end())
	//				{
	//					l_cubeMapTexture->second->update(0);
	//					meshMap.find(l_graphicData.first)->second->update();
	//				}
	//			}
	//		}
	//	}
	//}
	//glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	//glDisable(GL_DEPTH_TEST);
	//glDisable(GL_CULL_FACE);

	//// draw bloom extract pass
	//m_lightPassFrameBuffer->activeRenderTargetTexture(0, 0);
	//m_bloomExtractPassFrameBuffer->update(true, true);
	//m_bloomExtractPassFrameBuffer->setRenderBufferStorageSize(0);
	//m_bloomExtractPassShaderProgram->update(cameraComponents, lightComponents, visibleComponents, m_meshMap, m_textureMap);
	//this->getMesh(meshType::TWO_DIMENSION, m_Unit2DQuadTemplate)->update();

	//// draw bloom blur pass
	//bool l_isPing = true;
	//bool l_isFirstIteration = true;
	//for (size_t i = 0; i < 5; i++)
	//{
	//	if (l_isPing)
	//	{
	//		if (l_isFirstIteration)
	//		{
	//			m_bloomExtractPassFrameBuffer->activeRenderTargetTexture(0, 0);
	//			l_isFirstIteration = false;
	//		}
	//		else
	//		{
	//			m_bloomBlurPassPongFrameBuffer->activeRenderTargetTexture(0, 0);
	//		}

	//		m_bloomBlurPassPingFrameBuffer->update(true, true);
	//		m_bloomBlurPassPingFrameBuffer->setRenderBufferStorageSize(0);
	//		m_bloomBlurPassShaderProgram->update();

	//		m_bloomBlurPassShaderProgram->updateUniform(m_uni_horizontal, m_isHorizontal);

	//		if (m_isHorizontal)
	//		{
	//			m_isHorizontal = false;
	//		}
	//		else
	//		{
	//			m_isHorizontal = true;
	//		}
	//		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	//		this->getMesh(meshType::TWO_DIMENSION, m_Unit2DQuadTemplate)->update();
	//		l_isPing = false;
	//	}
	//	else
	//	{
	//		m_bloomBlurPassPingFrameBuffer->activeRenderTargetTexture(0, 0);

	//		m_bloomBlurPassPongFrameBuffer->update(true, true);
	//		m_bloomBlurPassPongFrameBuffer->setRenderBufferStorageSize(0);
	//		m_bloomBlurPassShaderProgram->update();

	//		updateUniform(m_uni_horizontal, m_isHorizontal);

	//		if (m_isHorizontal)
	//		{
	//			m_isHorizontal = false;
	//		}
	//		else
	//		{
	//			m_isHorizontal = true;
	//		}
	//		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	//		this->getMesh(meshType::TWO_DIMENSION, m_Unit2DQuadTemplate)->update();
	//		l_isPing = true;
	//	}
	//}

	//// draw billboard pass
	//m_geometryPassFrameBuffer->bindAsReadBuffer();
	//m_billboardPassFrameBuffer->bindAsWriteBuffer(m_screenResolution, m_screenResolution);
	//m_billboardPassFrameBuffer->update(true, false);
	//m_billboardPassFrameBuffer->setRenderBufferStorageSize(0);
	//m_billboardPassShaderProgram->update();

	//if (cameraComponents.size() > 0)
	//{
	//	mat4 p = cameraComponents[0]->m_projectionMatrix;
	//	mat4 r = cameraComponents[0]->getInvertRotationMatrix();
	//	mat4 t = cameraComponents[0]->getInvertTranslationMatrix();

	//	updateUniform(m_uni_p, p);
	//	updateUniform(m_uni_r, r);
	//	updateUniform(m_uni_t, t);
	//}

	//if (visibleComponents.size() > 0)
	//{
	//	// draw each visibleComponent
	//	for (auto& l_visibleComponent : visibleComponents)
	//	{
	//		if (l_visibleComponent->m_visiblilityType == visiblilityType::BILLBOARD)
	//		{
	//			updateUniform(m_uni_pos, l_visibleComponent->getParentEntity()->getTransform()->getPos().x, l_visibleComponent->getParentEntity()->getTransform()->getPos().y, l_visibleComponent->getParentEntity()->getTransform()->getPos().z);
	//			updateUniform(m_uni_albedo, l_visibleComponent->m_albedo.x, l_visibleComponent->m_albedo.y, l_visibleComponent->m_albedo.z);
	//			auto l_distanceToCamera = (cameraComponents[0]->getParentEntity()->getTransform()->getPos() - l_visibleComponent->getParentEntity()->getTransform()->getPos()).length();
	//			if (l_distanceToCamera > 1)
	//			{
	//				updateUniform(m_uni_size, (vec2(1.0, 1.0) * (1.0 / l_distanceToCamera)).x, (vec2(1.0, 1.0) * (1.0 / l_distanceToCamera)).y);
	//			}
	//			else
	//			{
	//				updateUniform(m_uni_size, vec2(1.0, 1.0).x, vec2(1.0, 1.0).y);
	//			}

	//			// draw each graphic data of visibleComponent
	//			for (auto& l_graphicData : l_visibleComponent->getModelMap())
	//			{
	//				//active and bind textures
	//				// is there any texture?
	//				auto l_textureMap = &l_graphicData.second;
	//				if (l_textureMap != nullptr)
	//				{
	//					// any albedo?
	//					auto l_diffuseTextureID = l_textureMap->find(textureType::ALBEDO);
	//					if (l_diffuseTextureID != l_textureMap->end())
	//					{
	//						auto l_textureData = textureMap.find(l_diffuseTextureID->second)->second;
	//						l_textureData->update(0);
	//					}
	//				}
	//				// draw meshes
	//				meshMap.find(l_graphicData.first)->second->update();
	//			}
	//		}
	//	}
	//}
	//glDisable(GL_DEPTH_TEST);

	//// draw debugger pass
	//m_geometryPassFrameBuffer->bindAsReadBuffer();
	//m_debuggerPassFrameBuffer->bindAsWriteBuffer(m_screenResolution, m_screenResolution);
	//m_debuggerPassFrameBuffer->update(true, false);
	//m_debuggerPassFrameBuffer->setRenderBufferStorageSize(0);
	//m_debuggerPassShaderProgram->update();


	//if (cameraComponents.size() > 0)
	//{
	//	mat4 p = cameraComponents[0]->m_projectionMatrix;
	//	mat4 r = cameraComponents[0]->getInvertRotationMatrix();
	//	mat4 t = cameraComponents[0]->getInvertTranslationMatrix();

	//	updateUniform(m_uni_p, p);
	//	updateUniform(m_uni_r, r);
	//	updateUniform(m_uni_t, t);
	//}

	//if (cameraComponents.size() > 0)
	//{
	//	for (auto& l_cameraComponent : cameraComponents)
	//	{
	//		// draw frustum for cameraComponent
	//		if (l_cameraComponent->m_drawFrustum)
	//		{
	//			auto l_cameraLocalMat = mat4();
	//			l_cameraLocalMat.initializeToIdentityMatrix();
	//			updateUniform(m_uni_m, l_cameraLocalMat);
	//			meshMap.find(l_cameraComponent->m_FrustumMeshID)->second->update();
	//		}
	//		// draw AABB of frustum for cameraComponent
	//		if (l_cameraComponent->m_drawAABB)
	//		{
	//			auto l_cameraLocalMat = mat4();
	//			l_cameraLocalMat.initializeToIdentityMatrix();
	//			updateUniform(m_uni_m, l_cameraLocalMat);
	//			meshMap.find(l_cameraComponent->m_AABBMeshID)->second->update();
	//		}
	//	}
	//}

	//if (lightComponents.size() > 0)
	//{
	//	// draw AABB for lightComponent
	//	for (auto& l_lightComponent : lightComponents)
	//	{
	//		if (l_lightComponent->m_drawAABB)
	//		{
	//			auto l_lightLocalMat = l_lightComponent->getParentEntity()->caclWorldRotMatrix();
	//			updateUniform(m_uni_m, l_lightLocalMat);
	//			for (auto l_AABBMeshID : l_lightComponent->m_AABBMeshIDs)
	//			{
	//				meshMap.find(l_AABBMeshID)->second->update();
	//			}
	//		}
	//	}
	//}

	//if (visibleComponents.size() > 0)
	//{
	//	// draw AABB for visibleComponent
	//	for (auto& l_visibleComponent : visibleComponents)
	//	{
	//		if (l_visibleComponent->m_visiblilityType == visiblilityType::STATIC_MESH && l_visibleComponent->m_drawAABB)
	//		{
	//			auto l_m = mat4();
	//			l_m.initializeToIdentityMatrix();
	//			updateUniform(m_uni_m, l_m);

	//			// draw each graphic data of visibleComponent
	//			for (auto& l_graphicData : l_visibleComponent->getModelMap())
	//			{
	//				//active and bind textures
	//				// is there any texture?
	//				auto l_textureMap = &l_graphicData.second;
	//				if (l_textureMap != nullptr)
	//				{
	//					// any normal?
	//					auto l_normalTextureID = l_textureMap->find(textureType::NORMAL);
	//					if (l_normalTextureID != l_textureMap->end())
	//					{
	//						auto l_textureData = textureMap.find(l_normalTextureID->second)->second;
	//						l_textureData->update(0);
	//					}
	//				}
	//				// draw meshes
	//				meshMap.find(l_visibleComponent->m_AABBMeshID)->second->update();
	//			}
	//		}
	//	}
	//}
	//glDisable(GL_DEPTH_TEST);
	//glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	//// draw final pass
	//glBindFramebuffer(GL_FRAMEBUFFER, 0);
	//glBindRenderbuffer(GL_RENDERBUFFER, 0);

	//m_lightPassFrameBuffer->activeRenderTargetTexture(0, 0);
	//m_skyPassFrameBuffer->activeRenderTargetTexture(0, 1);
	//m_bloomBlurPassPongFrameBuffer->activeRenderTargetTexture(0, 2);
	//m_billboardPassFrameBuffer->activeRenderTargetTexture(0, 3);
	//m_debuggerPassFrameBuffer->activeRenderTargetTexture(0, 4);
	//m_finalPassShaderProgram->update(cameraComponents, lightComponents, visibleComponents, m_meshMap, m_textureMap);

	//// draw final pass rectangle
	//this->getMesh(meshType::TWO_DIMENSION, m_Unit2DQuadTemplate)->update();
}

void FinalPass::shutdown()
{
}

const objectStatus & FinalPass::getStatus() const
{
	return m_objectStatus;
}
