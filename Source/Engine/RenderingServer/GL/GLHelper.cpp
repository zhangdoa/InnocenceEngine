#include "GLHelper.h"
#include "../../Core/InnoLogger.h"

#include "../../ModuleManager/IModuleManager.h"

extern IModuleManager* g_pModuleManager;

auto m_shaderRelativePath = "Res//Shaders//GL//";

GLTextureDataDesc GLHelper::getGLTextureDataDesc(TextureDataDesc textureDataDesc)
{
	GLTextureDataDesc l_result;

	l_result.textureSamplerType = getTextureSamplerType(textureDataDesc.samplerType);
	l_result.textureWrapMethod = getTextureWrapMethod(textureDataDesc.wrapMethod);
	l_result.minFilterParam = getTextureFilterParam(textureDataDesc.minFilterMethod);
	l_result.magFilterParam = getTextureFilterParam(textureDataDesc.magFilterMethod);
	l_result.internalFormat = getTextureInternalFormat(textureDataDesc);
	l_result.pixelDataFormat = getTexturePixelDataFormat(textureDataDesc);
	l_result.pixelDataType = getTexturePixelDataType(textureDataDesc.pixelDataType);
	l_result.pixelDataSize = getTexturePixelDataSize(textureDataDesc);
	l_result.width = textureDataDesc.width;
	l_result.height = textureDataDesc.height;
	l_result.depth = textureDataDesc.depth;
	l_result.borderColor[0] = textureDataDesc.borderColor[0];
	l_result.borderColor[1] = textureDataDesc.borderColor[1];
	l_result.borderColor[2] = textureDataDesc.borderColor[2];
	l_result.borderColor[3] = textureDataDesc.borderColor[3];

	return l_result;
}

GLenum GLHelper::getTextureSamplerType(TextureSamplerType rhs)
{
	switch (rhs)
	{
	case TextureSamplerType::SAMPLER_1D:
		return GL_TEXTURE_1D;
	case TextureSamplerType::SAMPLER_2D:
		return GL_TEXTURE_2D;
	case TextureSamplerType::SAMPLER_3D:
		return GL_TEXTURE_3D;
	case TextureSamplerType::SAMPLER_CUBEMAP:
		return GL_TEXTURE_CUBE_MAP;
	default:
		return 0;
	}
}

GLenum GLHelper::getTextureWrapMethod(TextureWrapMethod rhs)
{
	GLenum l_result;

	switch (rhs)
	{
	case TextureWrapMethod::CLAMP_TO_EDGE: l_result = GL_CLAMP_TO_EDGE; break;
	case TextureWrapMethod::REPEAT: l_result = GL_REPEAT; break;
	case TextureWrapMethod::CLAMP_TO_BORDER: l_result = GL_CLAMP_TO_BORDER; break;
	}

	return l_result;
}

GLenum GLHelper::getTextureFilterParam(TextureFilterMethod rhs)
{
	GLenum l_result;

	switch (rhs)
	{
	case TextureFilterMethod::NEAREST: l_result = GL_NEAREST; break;
	case TextureFilterMethod::LINEAR: l_result = GL_LINEAR; break;
	case TextureFilterMethod::LINEAR_MIPMAP_LINEAR: l_result = GL_LINEAR_MIPMAP_LINEAR; break;
	}

	return l_result;
}

GLenum GLHelper::getTextureInternalFormat(TextureDataDesc textureDataDesc)
{
	GLenum l_internalFormat;

	if (textureDataDesc.usageType == TextureUsageType::ALBEDO)
	{
		if (textureDataDesc.pixelDataFormat == TexturePixelDataFormat::RGB)
		{
			l_internalFormat = GL_SRGB;
		}
		else if (textureDataDesc.pixelDataFormat == TexturePixelDataFormat::RGBA)
		{
			l_internalFormat = GL_SRGB_ALPHA;
		}
	}
	else if (textureDataDesc.usageType == TextureUsageType::DEPTH_ATTACHMENT)
	{
		l_internalFormat = GL_DEPTH_COMPONENT32F;
	}
	else if (textureDataDesc.usageType == TextureUsageType::DEPTH_STENCIL_ATTACHMENT)
	{
		l_internalFormat = GL_DEPTH32F_STENCIL8;
	}
	else
	{
		if (textureDataDesc.pixelDataType == TexturePixelDataType::UBYTE)
		{
			switch (textureDataDesc.pixelDataFormat)
			{
			case TexturePixelDataFormat::R: l_internalFormat = GL_R8; break;
			case TexturePixelDataFormat::RG: l_internalFormat = GL_RG8; break;
			case TexturePixelDataFormat::RGB: l_internalFormat = GL_RGB8; break;
			case TexturePixelDataFormat::RGBA: l_internalFormat = GL_RGBA8; break;
			default: break;
			}
		}
		else if (textureDataDesc.pixelDataType == TexturePixelDataType::SBYTE)
		{
			switch (textureDataDesc.pixelDataFormat)
			{
			case TexturePixelDataFormat::R: l_internalFormat = GL_R8_SNORM; break;
			case TexturePixelDataFormat::RG: l_internalFormat = GL_RG8_SNORM; break;
			case TexturePixelDataFormat::RGB: l_internalFormat = GL_RGB8_SNORM; break;
			case TexturePixelDataFormat::RGBA: l_internalFormat = GL_RGBA8_SNORM; break;
			default: break;
			}
		}
		else if (textureDataDesc.pixelDataType == TexturePixelDataType::USHORT)
		{
			switch (textureDataDesc.pixelDataFormat)
			{
			case TexturePixelDataFormat::R: l_internalFormat = GL_R16; break;
			case TexturePixelDataFormat::RG: l_internalFormat = GL_RG16; break;
			case TexturePixelDataFormat::RGB: l_internalFormat = GL_RGB16; break;
			case TexturePixelDataFormat::RGBA: l_internalFormat = GL_RGBA16; break;
			default: break;
			}
		}
		else if (textureDataDesc.pixelDataType == TexturePixelDataType::SSHORT)
		{
			switch (textureDataDesc.pixelDataFormat)
			{
			case TexturePixelDataFormat::R: l_internalFormat = GL_R16_SNORM; break;
			case TexturePixelDataFormat::RG: l_internalFormat = GL_RG16_SNORM; break;
			case TexturePixelDataFormat::RGB: l_internalFormat = GL_RGB16_SNORM; break;
			case TexturePixelDataFormat::RGBA: l_internalFormat = GL_RGBA16_SNORM; break;
			default: break;
			}
		}
		else if (textureDataDesc.pixelDataType == TexturePixelDataType::UINT8)
		{
			switch (textureDataDesc.pixelDataFormat)
			{
			case TexturePixelDataFormat::R: l_internalFormat = GL_R8UI; break;
			case TexturePixelDataFormat::RG: l_internalFormat = GL_RG8UI; break;
			case TexturePixelDataFormat::RGB: l_internalFormat = GL_RGB8UI; break;
			case TexturePixelDataFormat::RGBA: l_internalFormat = GL_RGBA8UI; break;
			default: break;
			}
		}
		else if (textureDataDesc.pixelDataType == TexturePixelDataType::SINT8)
		{
			switch (textureDataDesc.pixelDataFormat)
			{
			case TexturePixelDataFormat::R: l_internalFormat = GL_R8I; break;
			case TexturePixelDataFormat::RG: l_internalFormat = GL_RG8I; break;
			case TexturePixelDataFormat::RGB: l_internalFormat = GL_RGB8I; break;
			case TexturePixelDataFormat::RGBA: l_internalFormat = GL_RGBA8I; break;
			default: break;
			}
		}
		else if (textureDataDesc.pixelDataType == TexturePixelDataType::UINT16)
		{
			switch (textureDataDesc.pixelDataFormat)
			{
			case TexturePixelDataFormat::R: l_internalFormat = GL_R16UI; break;
			case TexturePixelDataFormat::RG: l_internalFormat = GL_RG16UI; break;
			case TexturePixelDataFormat::RGB: l_internalFormat = GL_RGB16UI; break;
			case TexturePixelDataFormat::RGBA: l_internalFormat = GL_RGBA16UI; break;
			default: break;
			}
		}
		else if (textureDataDesc.pixelDataType == TexturePixelDataType::SINT16)
		{
			switch (textureDataDesc.pixelDataFormat)
			{
			case TexturePixelDataFormat::R: l_internalFormat = GL_R16I; break;
			case TexturePixelDataFormat::RG: l_internalFormat = GL_RG16I; break;
			case TexturePixelDataFormat::RGB: l_internalFormat = GL_RGB16I; break;
			case TexturePixelDataFormat::RGBA: l_internalFormat = GL_RGBA16I; break;
			default: break;
			}
		}
		else if (textureDataDesc.pixelDataType == TexturePixelDataType::UINT32)
		{
			switch (textureDataDesc.pixelDataFormat)
			{
			case TexturePixelDataFormat::R: l_internalFormat = GL_R32UI; break;
			case TexturePixelDataFormat::RG: l_internalFormat = GL_RG32UI; break;
			case TexturePixelDataFormat::RGB: l_internalFormat = GL_RGB32UI; break;
			case TexturePixelDataFormat::RGBA: l_internalFormat = GL_RGBA32UI; break;
			default: break;
			}
		}
		else if (textureDataDesc.pixelDataType == TexturePixelDataType::SINT32)
		{
			switch (textureDataDesc.pixelDataFormat)
			{
			case TexturePixelDataFormat::R: l_internalFormat = GL_R32I; break;
			case TexturePixelDataFormat::RG: l_internalFormat = GL_RG32I; break;
			case TexturePixelDataFormat::RGB: l_internalFormat = GL_RGB32I; break;
			case TexturePixelDataFormat::RGBA: l_internalFormat = GL_RGBA32I; break;
			default: break;
			}
		}
		else if (textureDataDesc.pixelDataType == TexturePixelDataType::FLOAT16)
		{
			switch (textureDataDesc.pixelDataFormat)
			{
			case TexturePixelDataFormat::R: l_internalFormat = GL_R16F; break;
			case TexturePixelDataFormat::RG: l_internalFormat = GL_RG16F; break;
			case TexturePixelDataFormat::RGB: l_internalFormat = GL_RGB16F; break;
			case TexturePixelDataFormat::RGBA: l_internalFormat = GL_RGBA16F; break;
			default: break;
			}
		}
		else if (textureDataDesc.pixelDataType == TexturePixelDataType::FLOAT32)
		{
			switch (textureDataDesc.pixelDataFormat)
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

GLenum GLHelper::getTexturePixelDataFormat(TextureDataDesc textureDataDesc)
{
	GLenum l_result;

	if (textureDataDesc.pixelDataType == TexturePixelDataType::UINT8
		|| textureDataDesc.pixelDataType == TexturePixelDataType::UINT16
		|| textureDataDesc.pixelDataType == TexturePixelDataType::UINT32
		)
	{
		switch (textureDataDesc.pixelDataFormat)
		{
		case TexturePixelDataFormat::R:l_result = GL_RED_INTEGER; break;
		case TexturePixelDataFormat::RG:l_result = GL_RG_INTEGER; break;
		case TexturePixelDataFormat::RGB:l_result = GL_RGB_INTEGER; break;
		case TexturePixelDataFormat::RGBA:l_result = GL_RGBA_INTEGER; break;
		case TexturePixelDataFormat::DEPTH_COMPONENT:l_result = GL_DEPTH_COMPONENT; break;
		default:
			break;
		}
	}
	else
	{
		switch (textureDataDesc.pixelDataFormat)
		{
		case TexturePixelDataFormat::R:l_result = GL_RED; break;
		case TexturePixelDataFormat::RG:l_result = GL_RG; break;
		case TexturePixelDataFormat::RGB:l_result = GL_RGB; break;
		case TexturePixelDataFormat::RGBA:l_result = GL_RGBA; break;
		case TexturePixelDataFormat::DEPTH_COMPONENT:l_result = GL_DEPTH_COMPONENT; break;
		default:
			break;
		}
	}

	return l_result;
}

GLenum GLHelper::getTexturePixelDataType(TexturePixelDataType rhs)
{
	GLenum l_result;

	switch (rhs)
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

	return l_result;
}

GLsizei GLHelper::getTexturePixelDataSize(TextureDataDesc textureDataDesc)
{
	GLsizei l_singlePixelSize;

	switch (textureDataDesc.pixelDataType)
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
	switch (textureDataDesc.pixelDataFormat)
	{
	case TexturePixelDataFormat::R:l_channelSize = 1; break;
	case TexturePixelDataFormat::RG:l_channelSize = 2; break;
	case TexturePixelDataFormat::RGB:l_channelSize = 3; break;
	case TexturePixelDataFormat::RGBA:l_channelSize = 4; break;
	case TexturePixelDataFormat::DEPTH_COMPONENT:l_channelSize = 1; break;
	case TexturePixelDataFormat::DEPTH_STENCIL_COMPONENT:l_channelSize = 1; break;
	default:
		break;
	}

	return l_singlePixelSize * l_channelSize;
}

std::string GLHelper::LoadShaderFile(const std::string & path)
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
		l_content.replace(l_includePos, l_GLSLExtensionPos - l_includePos + 6, LoadShaderFile(l_includedFileName));

		l_includePos = f_findIncludeFilePath(l_content);
	}

	return l_content;
}

bool GLHelper::AddShaderHandle(GLuint & shaderProgram, GLuint & shaderID, GLuint shaderType, const ShaderFilePath & shaderFilePath)
{
	// Create shader object
	shaderID = glCreateShader(shaderType);
	glObjectLabel(GL_SHADER, shaderID, (GLsizei)shaderFilePath.size(), shaderFilePath.c_str());

	if (shaderID == 0)
	{
		InnoLogger::Log(LogLevel::Error, "GLRenderingServer: Shader creation failed! Memory location is invaild when adding shader!");
		glDeleteShader(shaderID);
		return false;
	}

	// load shader
	if (shaderFilePath.find(".spv"))
	{
		auto l_shaderCodeContent = g_pModuleManager->getFileSystem()->loadFile(m_shaderRelativePath + std::string(shaderFilePath.c_str()), IOMode::Binary);

		if (l_shaderCodeContent.empty())
		{
			InnoLogger::Log(LogLevel::Error, "GLRenderingServer: ", shaderFilePath.c_str(), " content is empty!");
			return false;
		}

		// Apply the SPIR-V to the shader object.
		glShaderBinary(1, &shaderID, GL_SHADER_BINARY_FORMAT_SPIR_V, l_shaderCodeContent.data(), (GLsizei)l_shaderCodeContent.size());

		// Specialize the shader.
		glSpecializeShader(shaderID, "main", 0, 0, 0);
	}
	else
	{
		auto l_shaderCodeContent = LoadShaderFile(std::string(shaderFilePath.c_str()));

		if (l_shaderCodeContent.empty())
		{
			InnoLogger::Log(LogLevel::Error, "GLRenderingServer: ", shaderFilePath.c_str(), " content is empty!");
			return false;
		}

#ifdef _DEBUG
		std::ofstream m_debugExportFile;
		m_debugExportFile.open(g_pModuleManager->getFileSystem()->getWorkingDirectory() + "Res//Intermediate//Shaders//GL//" + std::string(shaderFilePath.c_str()));
		m_debugExportFile << l_shaderCodeContent;
		m_debugExportFile.close();
#endif // _DEBUG

		const char* l_sourcePointer = l_shaderCodeContent.c_str();

		glShaderSource(shaderID, 1, &l_sourcePointer, NULL);

		glCompileShader(shaderID);
	}

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