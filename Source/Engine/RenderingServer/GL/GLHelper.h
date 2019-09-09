#pragma once
#include "../../Component/GLTextureDataComponent.h"
#include "../../Component/GLRenderPassDataComponent.h"
#include "../IRenderingServer.h"

namespace GLHelper
{
	GLTextureDataDesc GetGLTextureDataDesc(TextureDataDesc textureDataDesc);
	GLenum GetTextureSamplerType(TextureSamplerType rhs);
	GLenum GetTextureWrapMethod(TextureWrapMethod rhs);
	GLenum GetTextureFilterParam(TextureFilterMethod rhs, bool useMipMap);
	GLenum GetTextureInternalFormat(TextureDataDesc textureDataDesc);
	GLenum GetTexturePixelDataFormat(TextureDataDesc textureDataDesc);
	GLenum GetTexturePixelDataType(TextureDataDesc textureDataDesc);
	GLsizei GetTexturePixelDataSize(TextureDataDesc textureDataDesc);

	bool CreateFramebuffer(GLRenderPassDataComponent * GLRPDC);
	bool ReserveRenderTargets(GLRenderPassDataComponent * GLRPDC, IRenderingServer * renderingServer);
	bool CreateRenderTargets(GLRenderPassDataComponent * GLRPDC, IRenderingServer* renderingServer);
	bool CreateResourcesBinder(GLRenderPassDataComponent * GLRPDC);
	bool CreateStateObjects(GLRenderPassDataComponent * GLRPDC);

	GLenum GetComparisionFunctionEnum(ComparisionFunction comparisionFunction);
	GLenum GetStencilOperationEnum(StencilOperation stencilOperation);
	GLenum GetBlendFactorEnum(BlendFactor blendFactor);
	GLenum GetPrimitiveTopologyEnum(PrimitiveTopology primitiveTopology);
	GLenum GetRasterizerFillModeEnum(RasterizerFillMode rasterizerFillMode);

	bool GenerateDepthStencilState(DepthStencilDesc DSDesc, GLPipelineStateObject* PSO);
	bool GenerateBlendState(BlendDesc blendDesc, GLPipelineStateObject* PSO);
	bool GenerateRasterizerState(RasterizerDesc rasterizerDesc, GLPipelineStateObject* PSO);
	bool GenerateViewportState(ViewportDesc viewportDesc, GLPipelineStateObject* PSO);

	bool AddShaderHandle(GLuint& shaderProgram, GLuint& shaderID, GLuint shaderStage, const ShaderFilePath& shaderFilePath);
	bool ActivateTexture(GLTextureDataComponent * GLTDC, int32_t activateIndex);
	bool BindTextureAsImage(GLTextureDataComponent * GLTDC, int32_t bindingSlot, Accessibility accessibility);

	/*
	attachmentIndex: GL_COLOR_ATTACHMENT0 to GL_MAX_COLOR_ATTACHMENTS
	textureIndex : Only valid for layered texture, include texture array and cubemap; default value -1 means attach all layers to the framebuffer
	*/
	bool AttachTextureToFramebuffer(GLTextureDataComponent * GLTDC, GLRenderPassDataComponent * GLRPDC, uint32_t attachmentIndex, uint32_t textureIndex = -1, uint32_t mipLevel = 0, uint32_t layer = 0);
}