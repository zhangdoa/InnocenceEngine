#include "GLHelper.h"
#include "../../Core/InnoLogger.h"

#include "../../ModuleManager/IModuleManager.h"

extern IModuleManager* g_pModuleManager;

namespace GLHelper
{
#ifdef _DEBUG
	const char* m_shaderRelativePath = "Res//Shaders//GL//";
#else
	const char* m_shaderRelativePath = "Res//Shaders//SPIRV//";
#endif
}

GLTextureDataDesc GLHelper::GetGLTextureDataDesc(TextureDataDesc textureDataDesc)
{
	GLTextureDataDesc l_result;

	l_result.TextureSamplerType = GetTextureSamplerType(textureDataDesc.SamplerType);
	l_result.TextureWrapMethod = GetTextureWrapMethod(textureDataDesc.WrapMethod);
	l_result.MinFilterParam = GetTextureFilterParam(textureDataDesc.MinFilterMethod, textureDataDesc.UseMipMap);
	l_result.MagFilterParam = GetTextureFilterParam(textureDataDesc.MagFilterMethod, false);
	l_result.InternalFormat = GetTextureInternalFormat(textureDataDesc);
	l_result.PixelDataFormat = GetTexturePixelDataFormat(textureDataDesc);
	l_result.PixelDataType = GetTexturePixelDataType(textureDataDesc);
	l_result.PixelDataSize = GetTexturePixelDataSize(textureDataDesc);
	l_result.Width = textureDataDesc.Width;
	l_result.Height = textureDataDesc.Height;
	l_result.DepthOrArraySize = textureDataDesc.DepthOrArraySize;
	l_result.BorderColor[0] = textureDataDesc.BorderColor[0];
	l_result.BorderColor[1] = textureDataDesc.BorderColor[1];
	l_result.BorderColor[2] = textureDataDesc.BorderColor[2];
	l_result.BorderColor[3] = textureDataDesc.BorderColor[3];

	return l_result;
}

GLenum GLHelper::GetTextureSamplerType(TextureSamplerType rhs)
{
	switch (rhs)
	{
	case TextureSamplerType::Sampler1D:
		return GL_TEXTURE_1D;
	case TextureSamplerType::Sampler2D:
		return GL_TEXTURE_2D;
	case TextureSamplerType::Sampler3D:
		return GL_TEXTURE_3D;
	case TextureSamplerType::Sampler1DArray:
		return GL_TEXTURE_1D_ARRAY;
	case TextureSamplerType::Sampler2DArray:
		return GL_TEXTURE_2D_ARRAY;
	case TextureSamplerType::SamplerCubemap:
		return GL_TEXTURE_CUBE_MAP;
	default:
		return 0;
	}
}

GLenum GLHelper::GetTextureWrapMethod(TextureWrapMethod rhs)
{
	GLenum l_result;

	switch (rhs)
	{
	case TextureWrapMethod::Edge: l_result = GL_CLAMP_TO_EDGE; break;
	case TextureWrapMethod::Repeat: l_result = GL_REPEAT; break;
	case TextureWrapMethod::Border: l_result = GL_CLAMP_TO_BORDER; break;
	}

	return l_result;
}

GLenum GLHelper::GetTextureFilterParam(TextureFilterMethod rhs, bool useMipMap)
{
	GLenum l_result;

	if (useMipMap)
	{
		switch (rhs)
		{
		case TextureFilterMethod::Nearest: l_result = GL_LINEAR_MIPMAP_NEAREST; break;
		case TextureFilterMethod::Linear: l_result = GL_LINEAR_MIPMAP_LINEAR; break;
		}
	}
	else
	{
		switch (rhs)
		{
		case TextureFilterMethod::Nearest: l_result = GL_NEAREST; break;
		case TextureFilterMethod::Linear: l_result = GL_LINEAR; break;
		}
	}

	return l_result;
}

GLenum GLHelper::GetTextureInternalFormat(TextureDataDesc textureDataDesc)
{
	GLenum l_internalFormat;

	if (textureDataDesc.UsageType == TextureUsageType::Albedo)
	{
		if (textureDataDesc.PixelDataFormat == TexturePixelDataFormat::RGB)
		{
			l_internalFormat = GL_SRGB;
		}
		else if (textureDataDesc.PixelDataFormat == TexturePixelDataFormat::RGBA)
		{
			l_internalFormat = GL_SRGB_ALPHA;
		}
	}
	else if (textureDataDesc.UsageType == TextureUsageType::DepthAttachment)
	{
		l_internalFormat = GL_DEPTH_COMPONENT32F;
	}
	else if (textureDataDesc.UsageType == TextureUsageType::DepthStencilAttachment)
	{
		l_internalFormat = GL_DEPTH24_STENCIL8;
	}
	else
	{
		if (textureDataDesc.PixelDataType == TexturePixelDataType::UBYTE)
		{
			switch (textureDataDesc.PixelDataFormat)
			{
			case TexturePixelDataFormat::R: l_internalFormat = GL_R8; break;
			case TexturePixelDataFormat::RG: l_internalFormat = GL_RG8; break;
			case TexturePixelDataFormat::RGB: l_internalFormat = GL_RGB8; break;
			case TexturePixelDataFormat::RGBA: l_internalFormat = GL_RGBA8; break;
			default: break;
			}
		}
		else if (textureDataDesc.PixelDataType == TexturePixelDataType::SBYTE)
		{
			switch (textureDataDesc.PixelDataFormat)
			{
			case TexturePixelDataFormat::R: l_internalFormat = GL_R8_SNORM; break;
			case TexturePixelDataFormat::RG: l_internalFormat = GL_RG8_SNORM; break;
			case TexturePixelDataFormat::RGB: l_internalFormat = GL_RGB8_SNORM; break;
			case TexturePixelDataFormat::RGBA: l_internalFormat = GL_RGBA8_SNORM; break;
			default: break;
			}
		}
		else if (textureDataDesc.PixelDataType == TexturePixelDataType::USHORT)
		{
			switch (textureDataDesc.PixelDataFormat)
			{
			case TexturePixelDataFormat::R: l_internalFormat = GL_R16; break;
			case TexturePixelDataFormat::RG: l_internalFormat = GL_RG16; break;
			case TexturePixelDataFormat::RGB: l_internalFormat = GL_RGB16; break;
			case TexturePixelDataFormat::RGBA: l_internalFormat = GL_RGBA16; break;
			default: break;
			}
		}
		else if (textureDataDesc.PixelDataType == TexturePixelDataType::SSHORT)
		{
			switch (textureDataDesc.PixelDataFormat)
			{
			case TexturePixelDataFormat::R: l_internalFormat = GL_R16_SNORM; break;
			case TexturePixelDataFormat::RG: l_internalFormat = GL_RG16_SNORM; break;
			case TexturePixelDataFormat::RGB: l_internalFormat = GL_RGB16_SNORM; break;
			case TexturePixelDataFormat::RGBA: l_internalFormat = GL_RGBA16_SNORM; break;
			default: break;
			}
		}
		else if (textureDataDesc.PixelDataType == TexturePixelDataType::UINT8)
		{
			switch (textureDataDesc.PixelDataFormat)
			{
			case TexturePixelDataFormat::R: l_internalFormat = GL_R8UI; break;
			case TexturePixelDataFormat::RG: l_internalFormat = GL_RG8UI; break;
			case TexturePixelDataFormat::RGB: l_internalFormat = GL_RGB8UI; break;
			case TexturePixelDataFormat::RGBA: l_internalFormat = GL_RGBA8UI; break;
			default: break;
			}
		}
		else if (textureDataDesc.PixelDataType == TexturePixelDataType::SINT8)
		{
			switch (textureDataDesc.PixelDataFormat)
			{
			case TexturePixelDataFormat::R: l_internalFormat = GL_R8I; break;
			case TexturePixelDataFormat::RG: l_internalFormat = GL_RG8I; break;
			case TexturePixelDataFormat::RGB: l_internalFormat = GL_RGB8I; break;
			case TexturePixelDataFormat::RGBA: l_internalFormat = GL_RGBA8I; break;
			default: break;
			}
		}
		else if (textureDataDesc.PixelDataType == TexturePixelDataType::UINT16)
		{
			switch (textureDataDesc.PixelDataFormat)
			{
			case TexturePixelDataFormat::R: l_internalFormat = GL_R16UI; break;
			case TexturePixelDataFormat::RG: l_internalFormat = GL_RG16UI; break;
			case TexturePixelDataFormat::RGB: l_internalFormat = GL_RGB16UI; break;
			case TexturePixelDataFormat::RGBA: l_internalFormat = GL_RGBA16UI; break;
			default: break;
			}
		}
		else if (textureDataDesc.PixelDataType == TexturePixelDataType::SINT16)
		{
			switch (textureDataDesc.PixelDataFormat)
			{
			case TexturePixelDataFormat::R: l_internalFormat = GL_R16I; break;
			case TexturePixelDataFormat::RG: l_internalFormat = GL_RG16I; break;
			case TexturePixelDataFormat::RGB: l_internalFormat = GL_RGB16I; break;
			case TexturePixelDataFormat::RGBA: l_internalFormat = GL_RGBA16I; break;
			default: break;
			}
		}
		else if (textureDataDesc.PixelDataType == TexturePixelDataType::UINT32)
		{
			switch (textureDataDesc.PixelDataFormat)
			{
			case TexturePixelDataFormat::R: l_internalFormat = GL_R32UI; break;
			case TexturePixelDataFormat::RG: l_internalFormat = GL_RG32UI; break;
			case TexturePixelDataFormat::RGB: l_internalFormat = GL_RGB32UI; break;
			case TexturePixelDataFormat::RGBA: l_internalFormat = GL_RGBA32UI; break;
			default: break;
			}
		}
		else if (textureDataDesc.PixelDataType == TexturePixelDataType::SINT32)
		{
			switch (textureDataDesc.PixelDataFormat)
			{
			case TexturePixelDataFormat::R: l_internalFormat = GL_R32I; break;
			case TexturePixelDataFormat::RG: l_internalFormat = GL_RG32I; break;
			case TexturePixelDataFormat::RGB: l_internalFormat = GL_RGB32I; break;
			case TexturePixelDataFormat::RGBA: l_internalFormat = GL_RGBA32I; break;
			default: break;
			}
		}
		else if (textureDataDesc.PixelDataType == TexturePixelDataType::FLOAT16)
		{
			switch (textureDataDesc.PixelDataFormat)
			{
			case TexturePixelDataFormat::R: l_internalFormat = GL_R16F; break;
			case TexturePixelDataFormat::RG: l_internalFormat = GL_RG16F; break;
			case TexturePixelDataFormat::RGB: l_internalFormat = GL_RGB16F; break;
			case TexturePixelDataFormat::RGBA: l_internalFormat = GL_RGBA16F; break;
			default: break;
			}
		}
		else if (textureDataDesc.PixelDataType == TexturePixelDataType::FLOAT32)
		{
			switch (textureDataDesc.PixelDataFormat)
			{
			case TexturePixelDataFormat::R: l_internalFormat = GL_R32F; break;
			case TexturePixelDataFormat::RG: l_internalFormat = GL_RG32F; break;
			case TexturePixelDataFormat::RGB: l_internalFormat = GL_RGB32F; break;
			case TexturePixelDataFormat::RGBA: l_internalFormat = GL_RGBA32F; break;
			default: break;
			}
		}
	}

	return l_internalFormat;
}

GLenum GLHelper::GetTexturePixelDataFormat(TextureDataDesc textureDataDesc)
{
	GLenum l_result;
	if (textureDataDesc.PixelDataFormat == TexturePixelDataFormat::Depth)
	{
		l_result = GL_DEPTH_COMPONENT;
	}
	else if (textureDataDesc.PixelDataFormat == TexturePixelDataFormat::DepthStencil)
	{
		l_result = GL_DEPTH_STENCIL;
	}
	else
	{
		if (textureDataDesc.PixelDataType == TexturePixelDataType::UINT8
			|| textureDataDesc.PixelDataType == TexturePixelDataType::UINT16
			|| textureDataDesc.PixelDataType == TexturePixelDataType::UINT32
			)
		{
			switch (textureDataDesc.PixelDataFormat)
			{
			case TexturePixelDataFormat::R:l_result = GL_RED_INTEGER; break;
			case TexturePixelDataFormat::RG:l_result = GL_RG_INTEGER; break;
			case TexturePixelDataFormat::RGB:l_result = GL_RGB_INTEGER; break;
			case TexturePixelDataFormat::RGBA:l_result = GL_RGBA_INTEGER; break;
			default:
				break;
			}
		}
		else
		{
			switch (textureDataDesc.PixelDataFormat)
			{
			case TexturePixelDataFormat::R:l_result = GL_RED; break;
			case TexturePixelDataFormat::RG:l_result = GL_RG; break;
			case TexturePixelDataFormat::RGB:l_result = GL_RGB; break;
			case TexturePixelDataFormat::RGBA:l_result = GL_RGBA; break;
			default:
				break;
			}
		}
	}

	return l_result;
}

GLenum GLHelper::GetTexturePixelDataType(TextureDataDesc textureDataDesc)
{
	GLenum l_result;

	if (textureDataDesc.PixelDataFormat == TexturePixelDataFormat::Depth)
	{
		l_result = GL_FLOAT;
	}
	else if (textureDataDesc.PixelDataFormat == TexturePixelDataFormat::DepthStencil)
	{
		l_result = GL_UNSIGNED_INT_24_8;
	}
	else
	{
		switch (textureDataDesc.PixelDataType)
		{
		case TexturePixelDataType::UBYTE:l_result = GL_UNSIGNED_BYTE; break;
		case TexturePixelDataType::SBYTE:l_result = GL_BYTE; break;
		case TexturePixelDataType::USHORT:l_result = GL_UNSIGNED_SHORT; break;
		case TexturePixelDataType::SSHORT:l_result = GL_SHORT; break;
		case TexturePixelDataType::UINT8:l_result = GL_UNSIGNED_INT; break;
		case TexturePixelDataType::SINT8:l_result = GL_INT; break;
		case TexturePixelDataType::UINT16:l_result = GL_UNSIGNED_INT; break;
		case TexturePixelDataType::SINT16:l_result = GL_INT; break;
		case TexturePixelDataType::UINT32:l_result = GL_UNSIGNED_INT; break;
		case TexturePixelDataType::SINT32:l_result = GL_INT; break;
		case TexturePixelDataType::FLOAT16:l_result = GL_HALF_FLOAT; break;
		case TexturePixelDataType::FLOAT32:l_result = GL_FLOAT; break;
		case TexturePixelDataType::DOUBLE:l_result = GL_DOUBLE; break;
		}
	}

	return l_result;
}

GLsizei GLHelper::GetTexturePixelDataSize(TextureDataDesc textureDataDesc)
{
	GLsizei l_singlePixelSize;

	switch (textureDataDesc.PixelDataType)
	{
	case TexturePixelDataType::UBYTE:l_singlePixelSize = 1; break;
	case TexturePixelDataType::SBYTE:l_singlePixelSize = 1; break;
	case TexturePixelDataType::USHORT:l_singlePixelSize = 2; break;
	case TexturePixelDataType::SSHORT:l_singlePixelSize = 2; break;
	case TexturePixelDataType::UINT8:l_singlePixelSize = 1; break;
	case TexturePixelDataType::SINT8:l_singlePixelSize = 1; break;
	case TexturePixelDataType::UINT16:l_singlePixelSize = 2; break;
	case TexturePixelDataType::SINT16:l_singlePixelSize = 2; break;
	case TexturePixelDataType::UINT32:l_singlePixelSize = 4; break;
	case TexturePixelDataType::SINT32:l_singlePixelSize = 4; break;
	case TexturePixelDataType::FLOAT16:l_singlePixelSize = 2; break;
	case TexturePixelDataType::FLOAT32:l_singlePixelSize = 4; break;
	case TexturePixelDataType::DOUBLE:l_singlePixelSize = 8; break;
	}

	GLsizei l_channelSize;
	switch (textureDataDesc.PixelDataFormat)
	{
	case TexturePixelDataFormat::R:l_channelSize = 1; break;
	case TexturePixelDataFormat::RG:l_channelSize = 2; break;
	case TexturePixelDataFormat::RGB:l_channelSize = 3; break;
	case TexturePixelDataFormat::RGBA:l_channelSize = 4; break;
	case TexturePixelDataFormat::Depth:l_channelSize = 1; break;
	case TexturePixelDataFormat::DepthStencil:l_channelSize = 1; break;
	}

	return l_singlePixelSize * l_channelSize;
}

bool GLHelper::CreateFramebuffer(GLRenderPassDataComponent * GLRPDC)
{
	// FBO
	glGenFramebuffers(1, &GLRPDC->m_FBO);
	glBindFramebuffer(GL_FRAMEBUFFER, GLRPDC->m_FBO);

#ifdef _DEBUG
	auto l_FBOName = std::string(GLRPDC->m_componentName.c_str());
	l_FBOName += "_FBO";
	glObjectLabel(GL_FRAMEBUFFER, GLRPDC->m_FBO, (GLsizei)l_FBOName.size(), l_FBOName.c_str());
#endif

	InnoLogger::Log(LogLevel::Verbose, "GLRenderingServer: ", GLRPDC->m_componentName.c_str(), " FBO has been generated.");

	// RBO
	if (GLRPDC->m_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_UseDepthBuffer)
	{
		GLRPDC->m_renderBufferAttachmentType = GL_DEPTH_ATTACHMENT;
		GLRPDC->m_renderBufferInternalFormat = GL_DEPTH_COMPONENT32F;

		if (GLRPDC->m_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_UseStencilBuffer)
		{
			GLRPDC->m_renderBufferAttachmentType = GL_DEPTH_STENCIL_ATTACHMENT;
			GLRPDC->m_renderBufferInternalFormat = GL_DEPTH24_STENCIL8;
		}

		glGenRenderbuffers(1, &GLRPDC->m_RBO);
		glBindRenderbuffer(GL_RENDERBUFFER, GLRPDC->m_RBO);

#ifdef _DEBUG
		auto l_RBOName = std::string(GLRPDC->m_componentName.c_str());
		l_RBOName += "_RBO";
		glObjectLabel(GL_RENDERBUFFER, GLRPDC->m_RBO, (GLsizei)l_RBOName.size(), l_RBOName.c_str());
#endif

		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GLRPDC->m_renderBufferAttachmentType, GL_RENDERBUFFER, GLRPDC->m_RBO);
		glRenderbufferStorage(GL_RENDERBUFFER, GLRPDC->m_renderBufferInternalFormat, GLRPDC->m_RenderPassDesc.m_RenderTargetDesc.Width, GLRPDC->m_RenderPassDesc.m_RenderTargetDesc.Height);

		std::vector<unsigned int> l_colorAttachments;
		for (unsigned int i = 0; i < GLRPDC->m_RenderPassDesc.m_RenderTargetCount; ++i)
		{
			l_colorAttachments.emplace_back(GL_COLOR_ATTACHMENT0 + i);
		}
		glDrawBuffers((GLsizei)l_colorAttachments.size(), &l_colorAttachments[0]);

		auto l_result = glCheckFramebufferStatus(GL_FRAMEBUFFER);
		if (l_result != GL_FRAMEBUFFER_COMPLETE)
		{
			InnoLogger::Log(LogLevel::Error, "GLRenderingServer: ", GLRPDC->m_componentName.c_str(), " Framebuffer is not completed: ", l_result);
			return false;
		}
		else
		{
			InnoLogger::Log(LogLevel::Verbose, "GLRenderingServer: ", GLRPDC->m_componentName.c_str(), " RBO has been generated.");
		}
	}

	return true;
}

bool GLHelper::ReserveRenderTargets(GLRenderPassDataComponent * GLRPDC, IRenderingServer * renderingServer)
{
	GLRPDC->m_RenderTargets.reserve(GLRPDC->m_RenderPassDesc.m_RenderTargetCount);

	for (unsigned int i = 0; i < GLRPDC->m_RenderPassDesc.m_RenderTargetCount; i++)
	{
		GLRPDC->m_RenderTargets.emplace_back();
		GLRPDC->m_RenderTargets[i] = renderingServer->AddTextureDataComponent((std::string(GLRPDC->m_componentName.c_str()) + "_" + std::to_string(i) + "/").c_str());
	}

	return true;
}

bool GLHelper::CreateRenderTargets(GLRenderPassDataComponent * GLRPDC, IRenderingServer* renderingServer)
{
	// Color RT
	for (unsigned int i = 0; i < GLRPDC->m_RenderPassDesc.m_RenderTargetCount; i++)
	{
		auto l_TDC = GLRPDC->m_RenderTargets[i];

		l_TDC->m_textureDataDesc = GLRPDC->m_RenderPassDesc.m_RenderTargetDesc;

		l_TDC->m_textureData = nullptr;

		renderingServer->InitializeTextureDataComponent(l_TDC);

		AttachTextureToFramebuffer(reinterpret_cast<GLTextureDataComponent*>(l_TDC), GLRPDC, i);
	}

	// DS RT
	if (GLRPDC->m_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_UseDepthBuffer)
	{
		auto l_TDC = renderingServer->AddTextureDataComponent((std::string(GLRPDC->m_componentName.c_str()) + "_DS/").c_str());

		l_TDC->m_textureDataDesc = GLRPDC->m_RenderPassDesc.m_RenderTargetDesc;

		if (GLRPDC->m_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_UseStencilBuffer)
		{
			l_TDC->m_textureDataDesc.UsageType = TextureUsageType::DepthStencilAttachment;
			l_TDC->m_textureDataDesc.PixelDataFormat = TexturePixelDataFormat::DepthStencil;
		}
		else
		{
			l_TDC->m_textureDataDesc.UsageType = TextureUsageType::DepthAttachment;
			l_TDC->m_textureDataDesc.PixelDataFormat = TexturePixelDataFormat::Depth;
		}

		l_TDC->m_textureData = nullptr;

		renderingServer->InitializeTextureDataComponent(l_TDC);

		AttachTextureToFramebuffer(reinterpret_cast<GLTextureDataComponent*>(l_TDC), GLRPDC, 0);

		GLRPDC->m_DepthStencilRenderTarget = l_TDC;
	}

	return true;
}

bool GLHelper::CreateResourcesBinder(GLRenderPassDataComponent * GLRPDC)
{
	for (size_t i = 0; i < GLRPDC->m_RenderTargetsResourceBinders.size(); i++)
	{
		GLRPDC->m_RenderTargetsResourceBinders[i] = GLRPDC->m_RenderTargets[i]->m_ResourceBinder;
	}

	return true;
}

bool GLHelper::CreateStateObjects(GLRenderPassDataComponent * GLRPDC)
{
	auto l_PSO = reinterpret_cast<GLPipelineStateObject*>(GLRPDC->m_PipelineStateObject);

	GenerateDepthStencilState(GLRPDC->m_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc, l_PSO);
	GenerateBlendState(GLRPDC->m_RenderPassDesc.m_GraphicsPipelineDesc.m_BlendDesc, l_PSO);
	GenerateRasterizerState(GLRPDC->m_RenderPassDesc.m_GraphicsPipelineDesc.m_RasterizerDesc, l_PSO);
	GenerateViewportState(GLRPDC->m_RenderPassDesc.m_GraphicsPipelineDesc.m_ViewportDesc, l_PSO);

	return true;
}

GLenum GLHelper::GetComparisionFunctionEnum(ComparisionFunction comparisionFunction)
{
	GLenum l_result;

	switch (comparisionFunction)
	{
	case ComparisionFunction::Never: l_result = GL_NEVER;
		break;
	case ComparisionFunction::Less: l_result = GL_LESS;
		break;
	case ComparisionFunction::Equal: l_result = GL_EQUAL;
		break;
	case ComparisionFunction::LessEqual: l_result = GL_LEQUAL;
		break;
	case ComparisionFunction::Greater: l_result = GL_GREATER;
		break;
	case ComparisionFunction::NotEqual: l_result = GL_NOTEQUAL;
		break;
	case ComparisionFunction::GreaterEqual: l_result = GL_GEQUAL;
		break;
	case ComparisionFunction::Always: l_result = GL_ALWAYS;
		break;
	default:
		break;
	}

	return l_result;
}

GLenum GLHelper::GetStencilOperationEnum(StencilOperation stencilOperation)
{
	GLenum l_result;

	switch (stencilOperation)
	{
	case StencilOperation::Keep: l_result = GL_KEEP;
		break;
	case StencilOperation::Zero: l_result = GL_ZERO;
		break;
	case StencilOperation::Replace: l_result = GL_REPLACE;
		break;
	case StencilOperation::IncreaseSat: l_result = GL_INCR_WRAP;
		break;
	case StencilOperation::DecreaseSat: l_result = GL_DECR_WRAP;
		break;
	case StencilOperation::Invert: l_result = GL_INVERT;
		break;
	case StencilOperation::Increase: l_result = GL_INCR;
		break;
	case StencilOperation::Decrease: l_result = GL_DECR;
		break;
	default:
		break;
	}

	return l_result;
}

GLenum GLHelper::GetBlendFactorEnum(BlendFactor blendFactor)
{
	GLenum l_result;

	switch (blendFactor)
	{
	case BlendFactor::Zero: l_result = GL_ZERO;
		break;
	case BlendFactor::One: l_result = GL_ONE;
		break;
	case BlendFactor::SrcColor: l_result = GL_SRC_COLOR;
		break;
	case BlendFactor::OneMinusSrcColor: l_result = GL_ONE_MINUS_SRC_COLOR;
		break;
	case BlendFactor::SrcAlpha: l_result = GL_SRC_ALPHA;
		break;
	case BlendFactor::OneMinusSrcAlpha: l_result = GL_ONE_MINUS_SRC_ALPHA;
		break;
	case BlendFactor::DestColor: l_result = GL_DST_COLOR;
		break;
	case BlendFactor::OneMinusDestColor: l_result = GL_ONE_MINUS_DST_COLOR;
		break;
	case BlendFactor::DestAlpha: l_result = GL_DST_ALPHA;
		break;
	case BlendFactor::OneMinusDestAlpha: l_result = GL_ONE_MINUS_DST_ALPHA;
		break;
	case BlendFactor::Src1Color: l_result = GL_SRC1_COLOR;
		break;
	case BlendFactor::OneMinusSrc1Color: l_result = GL_ONE_MINUS_SRC1_COLOR;
		break;
	case BlendFactor::Src1Alpha: l_result = GL_SRC1_ALPHA;
		break;
	case BlendFactor::OneMinusSrc1Alpha: l_result = GL_ONE_MINUS_SRC1_ALPHA;
		break;
	default:
		break;
	}

	return l_result;
}

GLenum GLHelper::GetPrimitiveTopologyEnum(PrimitiveTopology primitiveTopology)
{
	GLenum l_result;

	switch (primitiveTopology)
	{
	case PrimitiveTopology::Point: l_result = GL_POINTS;
		break;
	case PrimitiveTopology::Line: l_result = GL_LINE;
		break;
	case PrimitiveTopology::TriangleList: l_result = GL_TRIANGLES;
		break;
	case PrimitiveTopology::TriangleStrip: l_result = GL_TRIANGLE_STRIP;
		break;
	case PrimitiveTopology::Patch: l_result = GL_PATCHES;
		break;
	default:
		break;
	}

	return l_result;
}

GLenum GLHelper::GetRasterizerFillModeEnum(RasterizerFillMode rasterizerFillMode)
{
	GLenum l_result;

	switch (rasterizerFillMode)
	{
	case RasterizerFillMode::Point: l_result = GL_POINT;
		break;
	case RasterizerFillMode::Wireframe: l_result = GL_LINE;
		break;
	case RasterizerFillMode::Solid: l_result = GL_FILL;
		break;
	default:
		break;
	}

	return l_result;
}

bool GLHelper::GenerateDepthStencilState(DepthStencilDesc DSDesc, GLPipelineStateObject * PSO)
{
	if (DSDesc.m_UseDepthBuffer)
	{
		PSO->m_Activate.emplace_back([]() { glEnable(GL_DEPTH_TEST); });
		PSO->m_Deactivate.emplace_front([]() { glDisable(GL_DEPTH_TEST); });

		if (DSDesc.m_AllowDepthWrite)
		{
			PSO->m_Activate.emplace_back([]() { glDepthMask(GL_TRUE); });
		}
		else
		{
			PSO->m_Activate.emplace_back([]() { glDepthMask(GL_FALSE); });
		}

		auto l_comparisionEnum = GetComparisionFunctionEnum(DSDesc.m_DepthComparisionFunction);
		PSO->m_Activate.emplace_back([=]() { glDepthFunc(l_comparisionEnum); });

		if (DSDesc.m_AllowDepthClamp)
		{
			PSO->m_Activate.emplace_back([]() { glEnable(GL_DEPTH_CLAMP); });
			PSO->m_Deactivate.emplace_front([]() { glDisable(GL_DEPTH_CLAMP); });
		}
	}

	if (DSDesc.m_UseStencilBuffer)
	{
		PSO->m_Activate.emplace_back([]() { glEnable(GL_STENCIL_TEST); });
		PSO->m_Deactivate.emplace_front([]() { glDisable(GL_STENCIL_TEST); });

		if (DSDesc.m_AllowStencilWrite)
		{
			PSO->m_Activate.emplace_back([]() { glStencilMask(0xFF); });
		}
		else
		{
			PSO->m_Activate.emplace_back([]() { glStencilMask(0x00); });
		}

		auto l_FFStencilComparisionFunction = GetComparisionFunctionEnum(DSDesc.m_FrontFaceStencilComparisionFunction);
		auto l_BFStencilComparisionFunction = GetComparisionFunctionEnum(DSDesc.m_BackFaceStencilComparisionFunction);

		PSO->m_Activate.emplace_back([=]() { glStencilFuncSeparate(GL_FRONT, l_FFStencilComparisionFunction, DSDesc.m_StencilReference, DSDesc.m_StencilWriteMask); });
		PSO->m_Activate.emplace_back([=]() { glStencilFuncSeparate(GL_BACK, l_BFStencilComparisionFunction, DSDesc.m_StencilReference, DSDesc.m_StencilWriteMask); });

		auto l_FFStencilFailOp = GetStencilOperationEnum(DSDesc.m_FrontFaceStencilFailOperation);
		auto l_FFStencilPassDepthFailOp = GetStencilOperationEnum(DSDesc.m_FrontFaceStencilPassDepthFailOperation);
		auto l_FFStencilPassOp = GetStencilOperationEnum(DSDesc.m_FrontFaceStencilPassOperation);

		auto l_BFStencilFailOp = GetStencilOperationEnum(DSDesc.m_BackFaceStencilFailOperation);
		auto l_BFStencilPassDepthFailOp = GetStencilOperationEnum(DSDesc.m_BackFaceStencilPassDepthFailOperation);
		auto l_BFStencilPassOp = GetStencilOperationEnum(DSDesc.m_BackFaceStencilPassOperation);

		PSO->m_Activate.emplace_back([=]() { glStencilOpSeparate(GL_FRONT, l_FFStencilFailOp, l_FFStencilPassDepthFailOp, l_FFStencilPassOp); });
		PSO->m_Activate.emplace_back([=]() { glStencilOpSeparate(GL_BACK, l_BFStencilFailOp, l_BFStencilPassDepthFailOp, l_BFStencilPassOp); });
	}

	return true;
}

bool GLHelper::GenerateBlendState(BlendDesc blendDesc, GLPipelineStateObject * PSO)
{
	if (blendDesc.m_UseBlend)
	{
		PSO->m_Activate.emplace_back([]() { glEnable(GL_BLEND); });
		PSO->m_Deactivate.emplace_front([]() { glDisable(GL_BLEND); });

		auto l_srcRGBFactor = GetBlendFactorEnum(blendDesc.m_SourceRGBFactor);
		auto l_srcAFactor = GetBlendFactorEnum(blendDesc.m_SourceAlphaFactor);
		auto l_destRGBFactor = GetBlendFactorEnum(blendDesc.m_DestinationRGBFactor);
		auto l_destAFactor = GetBlendFactorEnum(blendDesc.m_DestinationAlphaFactor);

		PSO->m_Activate.emplace_back([=]() { glBlendFuncSeparate(l_srcRGBFactor, l_destRGBFactor, l_srcAFactor, l_destAFactor); });
	}
	return true;
}

bool GLHelper::GenerateRasterizerState(RasterizerDesc rasterizerDesc, GLPipelineStateObject * PSO)
{
	PSO->m_GLPrimitiveTopology = GetPrimitiveTopologyEnum(rasterizerDesc.m_PrimitiveTopology);

	auto l_rasterizerFillMode = GetRasterizerFillModeEnum(rasterizerDesc.m_RasterizerFillMode);
	PSO->m_Activate.emplace_back([=]() { glPolygonMode(GL_FRONT_AND_BACK, l_rasterizerFillMode); });

	if (rasterizerDesc.m_RasterizerFaceWinding == RasterizerFaceWinding::CCW)
	{
		PSO->m_Activate.emplace_back([]() { glFrontFace(GL_CCW); });
	}
	else
	{
		PSO->m_Activate.emplace_back([]() { glFrontFace(GL_CW); });
	}

	if (rasterizerDesc.m_UseCulling)
	{
		PSO->m_Activate.emplace_back([]() { glEnable(GL_CULL_FACE); });
		PSO->m_Deactivate.emplace_front([]() { glDisable(GL_CULL_FACE); });

		if (rasterizerDesc.m_RasterizerCullMode == RasterizerCullMode::Back)
		{
			PSO->m_Activate.emplace_back([]() { glCullFace(GL_BACK); });
		}
		else
		{
			PSO->m_Activate.emplace_back([]() { glCullFace(GL_FRONT); });
		}
	}

	if (rasterizerDesc.m_AllowMultisample)
	{
		PSO->m_Activate.emplace_back([]() { glEnable(GL_MULTISAMPLE); });
		PSO->m_Deactivate.emplace_front([]() { glDisable(GL_MULTISAMPLE); });
	}

	return true;
}

bool GLHelper::GenerateViewportState(ViewportDesc viewportDesc, GLPipelineStateObject * PSO)
{
	PSO->m_Activate.emplace_back([=]() { glViewport((GLint)viewportDesc.m_OriginX, (GLint)(viewportDesc.m_OriginY), (GLsizei)viewportDesc.m_Width, (GLsizei)viewportDesc.m_Height); });
	PSO->m_Activate.emplace_back([=]() { glDepthRange(viewportDesc.m_MinDepth, viewportDesc.m_MaxDepth); });
	return true;
}

bool GLHelper::AddShaderHandle(GLuint & shaderProgram, GLuint & shaderID, GLuint shaderStage, const ShaderFilePath & shaderFilePath)
{
	// Create shader object
	shaderID = glCreateShader(shaderStage);
	glObjectLabel(GL_SHADER, shaderID, (GLsizei)shaderFilePath.size(), shaderFilePath.c_str());

	if (shaderID == 0)
	{
		InnoLogger::Log(LogLevel::Error, "GLRenderingServer: Shader creation failed! Memory location is invalid when adding shader!");
		glDeleteShader(shaderID);
		return false;
	}

#ifdef _DEBUG
	std::function<std::string(const std::string&)> l_loadShaderFile = [&](const std::string & path) -> std::string
	{
		auto f_findIncludeFilePath = [](const std::string & content) {
			auto l_includePos = content.find("#include ");
			return l_includePos;
		};

		auto f_findGLSLExtensionPos = [](const std::string & content) {
			size_t l_glslExtensionPos = std::string::npos;

			l_glslExtensionPos = content.find(".glsl");
			if (l_glslExtensionPos != std::string::npos)
			{
				return l_glslExtensionPos;
			}

			l_glslExtensionPos = content.find(".vert");
			if (l_glslExtensionPos != std::string::npos)
			{
				return l_glslExtensionPos;
			}

			l_glslExtensionPos = content.find(".tesc");
			if (l_glslExtensionPos != std::string::npos)
			{
				return l_glslExtensionPos;
			}

			l_glslExtensionPos = content.find(".tese");
			if (l_glslExtensionPos != std::string::npos)
			{
				return l_glslExtensionPos;
			}

			l_glslExtensionPos = content.find(".geom");
			if (l_glslExtensionPos != std::string::npos)
			{
				return l_glslExtensionPos;
			}

			l_glslExtensionPos = content.find(".frag");
			if (l_glslExtensionPos != std::string::npos)
			{
				return l_glslExtensionPos;
			}

			l_glslExtensionPos = content.find(".comp");
			if (l_glslExtensionPos != std::string::npos)
			{
				return l_glslExtensionPos;
			}

			return l_glslExtensionPos;
		};

		auto l_rawContent = g_pModuleManager->getFileSystem()->loadFile((m_shaderRelativePath + path), IOMode::Text);

		std::string l_content = &l_rawContent[0];
		auto l_includePos = f_findIncludeFilePath(l_content);

		while (l_includePos != std::string::npos)
		{
			auto l_GLSLExtensionPos = f_findGLSLExtensionPos(l_content);
			auto l_includedFileName = l_content.substr(l_includePos + 10, l_GLSLExtensionPos - 5 - l_includePos);
			l_content.replace(l_includePos, l_GLSLExtensionPos - l_includePos + 6, l_loadShaderFile(l_includedFileName));

			l_includePos = f_findIncludeFilePath(l_content);
		}

		return l_content;
	};

	auto l_shaderCodeContent = l_loadShaderFile(std::string(shaderFilePath.c_str()));

	if (l_shaderCodeContent.empty())
	{
		InnoLogger::Log(LogLevel::Error, "GLRenderingServer: ", shaderFilePath.c_str(), " content is empty!");
		return false;
	}

	const char* l_sourcePointer = l_shaderCodeContent.c_str();

	glShaderSource(shaderID, 1, &l_sourcePointer, NULL);

	glCompileShader(shaderID);
#else
	// load shader
	auto l_shaderCodeContent = g_pModuleManager->getFileSystem()->loadFile(m_shaderRelativePath + std::string(shaderFilePath.c_str()) + ".spv", IOMode::Binary);

	if (l_shaderCodeContent.empty())
	{
		InnoLogger::Log(LogLevel::Error, "GLRenderingServer: ", shaderFilePath.c_str(), " content is empty!");
		return false;
	}

	// Apply the SPIR-V to the shader object.
	glShaderBinary(1, &shaderID, GL_SHADER_BINARY_FORMAT_SPIR_V, l_shaderCodeContent.data(), (GLsizei)l_shaderCodeContent.size());

	// Specialize the shader.
	glSpecializeShader(shaderID, "main", 0, 0, 0);
#endif

	// Validate shader
	GLint l_validationResult = GL_FALSE;
	GLint l_infoLogLength = 0;
	GLint l_shaderFileLength = 0;
	glGetShaderiv(shaderID, GL_COMPILE_STATUS, &l_validationResult);

	if (!l_validationResult)
	{
		InnoLogger::Log(LogLevel::Error, "GLRenderingServer: ", shaderFilePath.c_str(), " compile failed!");
		glGetShaderiv(shaderID, GL_SHADER_SOURCE_LENGTH, &l_shaderFileLength);
		InnoLogger::Log(LogLevel::Error, "GLRenderingServer: ", shaderFilePath.c_str(), " file length is: ", l_shaderFileLength);
		glGetShaderiv(shaderID, GL_INFO_LOG_LENGTH, &l_infoLogLength);

		if (l_infoLogLength > 0)
		{
			std::vector<char> l_shaderErrorMessage(l_infoLogLength + 1);
			glGetShaderInfoLog(shaderID, l_infoLogLength, NULL, &l_shaderErrorMessage[0]);
			InnoLogger::Log(LogLevel::Error, "GLRenderingServer: ", shaderFilePath.c_str(), " compile error: ", &l_shaderErrorMessage[0], "\n -- --------------------------------------------------- -- ");
		}
		else
		{
			InnoLogger::Log(LogLevel::Error, "GLRenderingServer: ", shaderFilePath.c_str(), " compile error: no info log provided!");
		}

		return false;
	}

	InnoLogger::Log(LogLevel::Verbose, "GLRenderingServer: ", shaderFilePath.c_str(), " has been compiled.");

	// Link shader to program
	glAttachShader(shaderProgram, shaderID);
	glLinkProgram(shaderProgram);
	glValidateProgram(shaderProgram);

	glGetProgramiv(shaderProgram, GL_LINK_STATUS, &l_validationResult);
	if (!l_validationResult)
	{
		InnoLogger::Log(LogLevel::Error, "GLRenderingServer: ", shaderFilePath.c_str(), " link failed!");
		glGetProgramiv(shaderProgram, GL_INFO_LOG_LENGTH, &l_infoLogLength);

		if (l_infoLogLength > 0) {
			std::vector<char> l_shaderErrorMessage(l_infoLogLength + 1);
			glGetProgramInfoLog(shaderProgram, l_infoLogLength, NULL, &l_shaderErrorMessage[0]);
			InnoLogger::Log(LogLevel::Error, "GLRenderingServer: ", shaderFilePath.c_str(), " link error: ", &l_shaderErrorMessage[0], "\n -- --------------------------------------------------- -- ");
		}
		else
		{
			InnoLogger::Log(LogLevel::Error, "GLRenderingServer: ", shaderFilePath.c_str(), " link error: no info log provided!");
		}

		return false;
	}

	InnoLogger::Log(LogLevel::Verbose, "GLRenderingServer: ", shaderFilePath.c_str(), " has been linked.");

	return true;
}

bool GLHelper::ActivateTexture(GLTextureDataComponent * GLTDC, int activateIndex)
{
	glActiveTexture(GL_TEXTURE0 + activateIndex);
	glBindTexture(GLTDC->m_GLTextureDataDesc.TextureSamplerType, GLTDC->m_TO);

	return true;
}

bool GLHelper::BindTextureAsImage(GLTextureDataComponent * GLTDC, int bindingSlot, Accessibility accessibility)
{
	GLenum l_accessibility;

	switch (accessibility)
	{
	case Accessibility::Immutable:
		l_accessibility = GL_READ_ONLY;
		break;
	case Accessibility::ReadOnly:
		l_accessibility = GL_READ_ONLY;
		break;
	case Accessibility::WriteOnly:
		l_accessibility = GL_WRITE_ONLY;
		break;
	case Accessibility::ReadWrite:
		l_accessibility = GL_READ_WRITE;
		break;
	default:
		break;
	}
	glBindImageTexture(bindingSlot, GLTDC->m_TO, 0, false, 0, l_accessibility, GLTDC->m_GLTextureDataDesc.InternalFormat);

	return true;
}

bool GLHelper::AttachTextureToFramebuffer(GLTextureDataComponent * GLTDC, GLRenderPassDataComponent * GLRPDC, unsigned int attachmentIndex, unsigned int textureIndex, unsigned int mipLevel, unsigned int layer)
{
	glBindFramebuffer(GL_FRAMEBUFFER, GLRPDC->m_FBO);

	GLenum l_attachmentType;

	if (GLTDC->m_textureDataDesc.PixelDataFormat == TexturePixelDataFormat::Depth)
	{
		l_attachmentType = GL_DEPTH_ATTACHMENT;
	}
	else if (GLTDC->m_textureDataDesc.PixelDataFormat == TexturePixelDataFormat::DepthStencil)
	{
		l_attachmentType = GL_DEPTH_STENCIL_ATTACHMENT;
	}
	else
	{
		l_attachmentType = GL_COLOR_ATTACHMENT0 + attachmentIndex;
	}

	switch (GLTDC->m_textureDataDesc.SamplerType)
	{
	case TextureSamplerType::Sampler1D:
		glFramebufferTexture1D(GL_FRAMEBUFFER, l_attachmentType, GL_TEXTURE_1D, GLTDC->m_TO, mipLevel);
		break;
	case TextureSamplerType::Sampler2D:
		glFramebufferTexture2D(GL_FRAMEBUFFER, l_attachmentType, GL_TEXTURE_2D, GLTDC->m_TO, mipLevel);
		break;
	case TextureSamplerType::Sampler3D:
		glFramebufferTexture3D(GL_FRAMEBUFFER, l_attachmentType, GL_TEXTURE_3D, GLTDC->m_TO, mipLevel, layer);
		break;
	case TextureSamplerType::Sampler1DArray:
		glFramebufferTexture(GL_FRAMEBUFFER, l_attachmentType, GLTDC->m_TO, mipLevel);
		break;
	case TextureSamplerType::Sampler2DArray:
		glFramebufferTexture(GL_FRAMEBUFFER, l_attachmentType, GLTDC->m_TO, mipLevel);
		break;
	case TextureSamplerType::SamplerCubemap:
		if (textureIndex == -1)
		{
			glFramebufferTexture(GL_FRAMEBUFFER, l_attachmentType, GLTDC->m_TO, mipLevel);
		}
		else
		{
			glFramebufferTexture2D(GL_FRAMEBUFFER, l_attachmentType, GL_TEXTURE_CUBE_MAP_POSITIVE_X + textureIndex, GLTDC->m_TO, mipLevel);
		}
		break;
	default:
		break;
	}

	return true;
}