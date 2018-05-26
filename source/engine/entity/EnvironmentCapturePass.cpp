#include "EnvironmentCapturePass.h"

void EnvironmentCapturePass::setup()
{
	m_environmentCapturePassShaderProgram = g_pMemorySystem->spawn<EnvironmentCapturePassPBSShaderProgram>();
	m_environmentConvolutionPassShaderProgram = g_pMemorySystem->spawn<EnvironmentConvolutionPassPBSShaderProgram>();
	m_environmentPreFilterPassShaderProgram = g_pMemorySystem->spawn<EnvironmentPreFilterPassPBSShaderProgram>();
	m_environmentBRDFLUTPassShaderProgram = g_pMemorySystem->spawn<EnvironmentBRDFLUTPassPBSShaderProgram>();
}

void EnvironmentCapturePass::initialize()
{
	// initialize shader
	auto l_environmentCapturePassVertexShader = g_pMemorySystem->spawn<SHADER_CLASS>();
	l_environmentCapturePassVertexShader->setup(shaderData(shaderType::VERTEX, "GL3.3/environmentCapturePassPBSVertex.sf", g_pAssetSystem->loadShader("GL3.3/environmentCapturePassPBSVertex.sf")));
	auto l_environmentCapturePassFragmentShader = g_pMemorySystem->spawn<SHADER_CLASS>();
	l_environmentCapturePassFragmentShader->setup(shaderData(shaderType::FRAGMENT, "GL3.3/environmentCapturePassPBSFragment.sf", g_pAssetSystem->loadShader("GL3.3/environmentCapturePassPBSFragment.sf")));
	m_environmentCapturePassShaderProgram->setup({ l_environmentCapturePassVertexShader , l_environmentCapturePassFragmentShader });
	m_environmentCapturePassShaderProgram->initialize();

	// initialize texture
	m_environmentCapturePassTextureID = g_pRenderingSystem->addTexture(textureType::ENVIRONMENT_CAPTURE);
	auto l_environmentCapturePassTextureData = g_pRenderingSystem->getTexture(textureType::ENVIRONMENT_CAPTURE, m_environmentCapturePassTextureID);
	l_environmentCapturePassTextureData->setup(textureType::ENVIRONMENT_CAPTURE, textureColorComponentsFormat::RGB16F, texturePixelDataFormat::RGB, textureWrapMethod::REPEAT, textureFilterMethod::LINEAR_MIPMAP_LINEAR, textureFilterMethod::LINEAR, 2048, 2048, texturePixelDataType::FLOAT, std::vector<void*>{nullptr, nullptr, nullptr, nullptr, nullptr, nullptr});

	// environment convolution pass
	// initialize shader
	auto l_environmentConvolutionPassVertexShader = g_pMemorySystem->spawn<SHADER_CLASS>(); 
	l_environmentConvolutionPassVertexShader->setup(shaderData(shaderType::VERTEX, "GL3.3/environmentConvolutionPassPBSVertex.sf", g_pAssetSystem->loadShader("GL3.3/environmentConvolutionPassPBSVertex.sf")));
	auto l_environmentConvolutionPassFragmentShader = g_pMemorySystem->spawn<SHADER_CLASS>(); ;
	l_environmentConvolutionPassFragmentShader->setup(shaderData(shaderType::FRAGMENT, "GL3.3/environmentConvolutionPassPBSFragment.sf", g_pAssetSystem->loadShader("GL3.3/environmentConvolutionPassPBSFragment.sf")));
	m_environmentConvolutionPassShaderProgram->setup({ l_environmentConvolutionPassVertexShader , l_environmentConvolutionPassFragmentShader });
	m_environmentConvolutionPassShaderProgram->initialize();

	// initialize texture
	m_environmentConvolutionPassTextureID = g_pRenderingSystem->addTexture(textureType::ENVIRONMENT_CONVOLUTION);
	auto l_environmentConvolutionPassTextureData = g_pRenderingSystem->getTexture(textureType::ENVIRONMENT_CONVOLUTION, m_environmentConvolutionPassTextureID);
	l_environmentConvolutionPassTextureData->setup(textureType::ENVIRONMENT_CONVOLUTION, textureColorComponentsFormat::RGB16F, texturePixelDataFormat::RGB, textureWrapMethod::REPEAT, textureFilterMethod::LINEAR, textureFilterMethod::LINEAR, 128, 128, texturePixelDataType::FLOAT, std::vector<void*>{nullptr, nullptr, nullptr, nullptr, nullptr, nullptr});

	// environment pre-filter pass
	// initialize shader
	auto l_environmentPreFilterPassVertexShader = g_pMemorySystem->spawn<SHADER_CLASS>();
	l_environmentPreFilterPassVertexShader->setup(shaderData(shaderType::VERTEX, "GL3.3/environmentPreFilterPassPBSVertex.sf", g_pAssetSystem->loadShader("GL3.3/environmentPreFilterPassPBSVertex.sf")));
	auto l_environmentPreFilterPassFragmentShader = g_pMemorySystem->spawn<SHADER_CLASS>();
	l_environmentPreFilterPassFragmentShader->setup(shaderData(shaderType::FRAGMENT, "GL3.3/environmentPreFilterPassPBSFragment.sf", g_pAssetSystem->loadShader("GL3.3/environmentPreFilterPassPBSFragment.sf")));
	m_environmentPreFilterPassShaderProgram->setup({ l_environmentPreFilterPassVertexShader , l_environmentPreFilterPassFragmentShader });
	m_environmentPreFilterPassShaderProgram->initialize();

	// initialize texture
	m_environmentPreFilterPassTextureID = g_pRenderingSystem->addTexture(textureType::ENVIRONMENT_PREFILTER);
	auto l_environmentPreFilterPassTextureData = g_pRenderingSystem->getTexture(textureType::ENVIRONMENT_PREFILTER, m_environmentPreFilterPassTextureID);
	l_environmentPreFilterPassTextureData->setup(textureType::ENVIRONMENT_PREFILTER, textureColorComponentsFormat::RGB16F, texturePixelDataFormat::RGB, textureWrapMethod::REPEAT, textureFilterMethod::LINEAR_MIPMAP_LINEAR, textureFilterMethod::LINEAR, 128, 128, texturePixelDataType::FLOAT, std::vector<void*>{nullptr, nullptr, nullptr, nullptr, nullptr, nullptr});

	// environment BRDF LUT pass
	// initialize shader
	auto l_environmentBRDFLUTPassVertexShader = g_pMemorySystem->spawn<SHADER_CLASS>();;
	l_environmentBRDFLUTPassVertexShader->setup(shaderData(shaderType::VERTEX, "GL3.3/environmentBRDFLUTPassPBSVertex.sf", g_pAssetSystem->loadShader("GL3.3/environmentBRDFLUTPassPBSVertex.sf")));
	auto l_environmentBRDFLUTPassFragmentShader = g_pMemorySystem->spawn<SHADER_CLASS>(); 
	l_environmentBRDFLUTPassFragmentShader->setup(shaderData(shaderType::FRAGMENT, "GL3.3/environmentBRDFLUTPassPBSFragment.sf", g_pAssetSystem->loadShader("GL3.3/environmentBRDFLUTPassPBSFragment.sf")));
	m_environmentBRDFLUTPassShaderProgram->setup({ l_environmentBRDFLUTPassVertexShader , l_environmentBRDFLUTPassFragmentShader });
	m_environmentBRDFLUTPassShaderProgram->initialize();

	// initialize texture
	m_environmentBRDFLUTTextureID = g_pRenderingSystem->addTexture(textureType::RENDER_BUFFER_SAMPLER);
	auto l_environmentBRDFLUTTextureData = g_pRenderingSystem->getTexture(textureType::RENDER_BUFFER_SAMPLER, m_environmentBRDFLUTTextureID);
	l_environmentBRDFLUTTextureData->setup(textureType::RENDER_BUFFER_SAMPLER, textureColorComponentsFormat::RG16F, texturePixelDataFormat::RG, textureWrapMethod::CLAMP_TO_EDGE, textureFilterMethod::LINEAR, textureFilterMethod::LINEAR, 512, 512, texturePixelDataType::FLOAT, std::vector<void*>{ nullptr });

	// initialize environment pass framebuffer
	auto l_environmentPassRenderBufferStorageSizes = std::vector<vec2>{ vec2(2048, 2048), vec2(128, 128), vec2(128, 128), vec2(512, 512) };
	auto l_environmentPassRenderTargetTextures = std::vector<BaseTexture*>{ l_environmentCapturePassTextureData, l_environmentConvolutionPassTextureData, l_environmentPreFilterPassTextureData, l_environmentBRDFLUTTextureData };
	m_environmentPassFrameBuffer = g_pMemorySystem->spawn<FRAMEBUFFER_CLASS>();
	m_environmentPassFrameBuffer->setup(frameBufferType::ENVIRONMENT_PASS, renderBufferType::DEPTH, l_environmentPassRenderBufferStorageSizes, l_environmentPassRenderTargetTextures);
	m_environmentPassFrameBuffer->initialize();
}

void EnvironmentCapturePass::draw()
{
	//// bind to framebuffer
	//m_environmentPassFrameBuffer->update(true, true);

	//// draw environment capture texture
	//m_environmentPassFrameBuffer->setRenderBufferStorageSize(0);

	//mat4 captureProjection;
	//captureProjection.initializeToPerspectiveMatrix((90.0 / 180.0) * PI, 1.0, 0.1, 10.0);
	//std::vector<mat4> captureViews =
	//{
	//	mat4().lookAt(vec4(0.0, 0.0, 0.0, 1.0), vec4(1.0,  0.0,  0.0, 1.0), vec4(0.0, -1.0,  0.0, 0.0)),
	//	mat4().lookAt(vec4(0.0, 0.0, 0.0, 1.0), vec4(-1.0,  0.0,  0.0, 1.0), vec4(0.0, -1.0,  0.0, 0.0)),
	//	mat4().lookAt(vec4(0.0, 0.0, 0.0, 1.0), vec4(0.0,  1.0,  0.0, 1.0), vec4(0.0,  0.0,  1.0, 0.0)),
	//	mat4().lookAt(vec4(0.0, 0.0, 0.0, 1.0), vec4(0.0, -1.0,  0.0, 1.0), vec4(0.0,  0.0, -1.0, 0.0)),
	//	mat4().lookAt(vec4(0.0, 0.0, 0.0, 1.0), vec4(0.0,  0.0,  1.0, 1.0), vec4(0.0, -1.0,  0.0, 0.0)),
	//	mat4().lookAt(vec4(0.0, 0.0, 0.0, 1.0), vec4(0.0,  0.0, -1.0, 1.0), vec4(0.0, -1.0,  0.0, 0.0))
	//};

	//m_environmentCapturePassShaderProgram->activate();
	//m_environmentCapturePassShaderProgram->updateUniform(m_uni_p, captureProjection);

	//if (visibleComponents.size() > 0)
	//{
	//	for (auto& l_visibleComponent : visibleComponents)
	//	{
	//		if (l_visibleComponent->m_visiblilityType == visiblilityType::SKYBOX)
	//		{

	//			for (auto& l_graphicData : l_visibleComponent->getModelMap())
	//			{
	//				// activate equiretangular texture and remap equiretangular texture to cubemap
	//				auto l_equiretangularTexture = textureMap.find(l_graphicData.second.find(textureType::EQUIRETANGULAR)->second);
	//				auto l_environmentCaptureTexture = textureMap.find(l_graphicData.second.find(textureType::ENVIRONMENT_CAPTURE)->second);
	//				if (l_equiretangularTexture != textureMap.end() && l_environmentCaptureTexture != textureMap.end())
	//				{
	//					l_equiretangularTexture->second->activate(0);
	//					for (unsigned int i = 0; i < 6; ++i)
	//					{
	//						updateUniform(m_uni_r, captureViews[i]);
	//						l_environmentCaptureTexture->second->attachToFramebuffer(0, i, 0);
	//						glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	//						meshMap.find(l_graphicData.first)->second->update();
	//					}
	//					l_environmentCaptureTexture->second->activate(0);
	//					glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
	//				}
	//			}
	//		}
	//	}
	//}

	//// draw environment convolution texture
	//m_environmentPassFrameBuffer->setRenderBufferStorageSize(1);
	//m_environmentConvolutionPassShaderProgram->activate();

	//m_environmentConvolutionPassShaderProgram->updateUniform(m_uni_p, captureProjection);

	//if (visibleComponents.size() > 0)
	//{
	//	for (auto& l_visibleComponent : visibleComponents)
	//	{
	//		if (l_visibleComponent->m_visiblilityType == visiblilityType::SKYBOX)
	//		{
	//			for (auto& l_graphicData : l_visibleComponent->getModelMap())
	//			{
	//				auto l_environmentCaptureTexture = textureMap.find(l_graphicData.second.find(textureType::ENVIRONMENT_CAPTURE)->second);
	//				auto l_environmentConvolutionTexture = textureMap.find(l_graphicData.second.find(textureType::ENVIRONMENT_CONVOLUTION)->second);
	//				if (l_environmentCaptureTexture != textureMap.end() && l_environmentConvolutionTexture != textureMap.end())
	//				{
	//					// @TODO: it should be update(0)?
	//					l_environmentCaptureTexture->second->update(1);
	//					for (unsigned int i = 0; i < 6; ++i)
	//					{
	//						updateUniform(m_uni_r, captureViews[i]);
	//						l_environmentConvolutionTexture->second->attachToFramebuffer(0, i, 0);
	//						glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	//						meshMap.find(l_graphicData.first)->second->update();
	//					}
	//				}
	//			}
	//		}
	//	}
	//}

	//// draw environment pre-filter texture
	//m_environmentPassFrameBuffer->setRenderBufferStorageSize(2);
	//m_environmentPreFilterPassShaderProgram->activate();

	//m_environmentPreFilterPassShaderProgram->updateUniform(m_uni_p, captureProjection);

	//if (visibleComponents.size() > 0)
	//{
	//	for (auto& l_visibleComponent : visibleComponents)
	//	{
	//		if (l_visibleComponent->m_visiblilityType == visiblilityType::SKYBOX)
	//		{
	//			for (auto& l_graphicData : l_visibleComponent->getModelMap())
	//			{
	//				auto l_environmentCaptureTexture = textureMap.find(l_graphicData.second.find(textureType::ENVIRONMENT_CAPTURE)->second);
	//				auto l_environmentPrefilterTexture = textureMap.find(l_graphicData.second.find(textureType::ENVIRONMENT_PREFILTER)->second);
	//				if (l_environmentCaptureTexture != textureMap.end() && l_environmentPrefilterTexture != textureMap.end())
	//				{
	//					// @TODO: it should be update(0)?
	//					l_environmentCaptureTexture->second->update(2);
	//					unsigned int maxMipLevels = 5;
	//					for (unsigned int mip = 0; mip < maxMipLevels; ++mip)
	//					{
	//						// resize framebuffer according to mip-level size.
	//						unsigned int mipWidth = (int)(128 * std::pow(0.5, mip));
	//						unsigned int mipHeight = (int)(128 * std::pow(0.5, mip));

	//						glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT32, mipWidth, mipHeight);
	//						glViewport(0, 0, mipWidth, mipHeight);

	//						double roughness = (double)mip / (double)(maxMipLevels - 1);
	//						updateUniform(m_uni_roughness, roughness);
	//						for (unsigned int i = 0; i < 6; ++i)
	//						{
	//							updateUniform(m_uni_r, captureViews[i]);
	//							l_environmentPrefilterTexture->second->attachToFramebuffer(0, i, mip);
	//							glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	//							meshMap.find(l_graphicData.first)->second->update();
	//						}
	//					}
	//				}
	//			}
	//		}
	//	}
	//}

	//// draw environment BRDF look-up table texture
	//m_environmentPassFrameBuffer->setRenderBufferStorageSize(3);
	//m_environmentBRDFLUTPassShaderProgram->update(cameraComponents, lightComponents, visibleComponents, m_meshMap, m_textureMap);

	//if (visibleComponents.size() > 0)
	//{
	//	for (auto& l_visibleComponent : visibleComponents)
	//	{
	//		if (l_visibleComponent->m_visiblilityType == visiblilityType::SKYBOX)
	//		{
	//			for (auto& l_graphicData : l_visibleComponent->getModelMap())
	//			{
	//				auto l_environmentBRDFLUTTexture = textureMap.find(l_graphicData.second.find(textureType::RENDER_BUFFER_SAMPLER)->second);
	//				if (l_environmentBRDFLUTTexture != textureMap.end())
	//				{
	//					l_environmentBRDFLUTTexture->second->attachToFramebuffer(0, 0, 0);
	//				}
	//			}
	//		}
	//	}
	//}

	//// draw environment map BRDF LUT rectangle
	//g_pRenderingSystem->getMesh(meshType::TWO_DIMENSION, m_Unit2DQuadTemplate)->update();
}

void EnvironmentCapturePass::shutdown()
{
}

const objectStatus & EnvironmentCapturePass::getStatus() const
{
	return m_objectStatus;
}
