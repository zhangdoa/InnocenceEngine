#pragma once
#include "../../Component/GLTextureComponent.h"
#include "../../Component/GLRenderPassComponent.h"
#include "../IRenderingServer.h"

namespace Inno
{
	namespace GLHelper
	{
		GLTextureDesc GetGLTextureDesc(TextureDesc textureDesc);
		GLenum GetTextureSampler(TextureSampler rhs);
		GLenum GetTextureWrapMethod(TextureWrapMethod rhs);
		GLenum GetTextureFilterParam(TextureFilterMethod rhs, bool useMipMap);
		GLenum GetTextureInternalFormat(TextureDesc textureDesc);
		GLenum GetTexturePixelDataFormat(TextureDesc textureDesc);
		GLenum GetTexturePixelDataType(TextureDesc textureDesc);
		GLsizei GetTexturePixelDataSize(TextureDesc textureDesc);

		bool CreateFramebuffer(GLRenderPassComponent* GLRenderPassComp);
		bool AttachFramebuffer(GLRenderPassComponent* GLRenderPassComp);
		bool CreateStateObjects(GLRenderPassComponent* GLRenderPassComp);

		GLenum GetComparisionFunctionEnum(ComparisionFunction comparisionFunction);
		GLenum GetStencilOperationEnum(StencilOperation stencilOperation);
		GLenum GetBlendFactorEnum(BlendFactor blendFactor);
		GLenum GetPrimitiveTopologyEnum(PrimitiveTopology primitiveTopology);
		GLenum GetRasterizerFillModeEnum(RasterizerFillMode rasterizerFillMode);

		bool GenerateDepthStencilState(DepthStencilDesc DSDesc, GLPipelineStateObject* PSO);
		bool GenerateBlendState(BlendDesc blendDesc, GLPipelineStateObject* PSO);
		bool GenerateRasterizerState(RasterizerDesc rasterizerDesc, GLPipelineStateObject* PSO);
		bool GenerateViewportState(ViewportDesc viewportDesc, GLPipelineStateObject* PSO);

		bool AddShaderObject(GLuint& shaderID, GLuint shaderStage, const ShaderFilePath& shaderFilePath);
		bool LinkProgramObject(GLuint& shaderProgram);
		bool ActivateTexture(GLTextureComponent* GLTextureComp, int32_t activateIndex);
		bool BindTextureAsImage(GLTextureComponent* GLTextureComp, int32_t bindingSlot, Accessibility accessibility);

		/*
		attachmentIndex: GL_COLOR_ATTACHMENT0 to GL_MAX_COLOR_ATTACHMENTS
		textureIndex : Only valid for layered texture, include texture array and cubemap; default value -1 means attach all layers to the framebuffer
		*/
		bool AttachTextureToFramebuffer(GLTextureComponent* GLTextureComp, GLRenderPassComponent* GLRenderPassComp, uint32_t attachmentIndex, uint32_t textureIndex = -1, uint32_t mipLevel = 0, uint32_t layer = 0);
	}
}