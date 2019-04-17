#include "GLBloomExtractPass.h"

#include "GLRenderingSystemUtilities.h"
#include "../../component/GLRenderingSystemComponent.h"

using namespace GLRenderingSystemNS;

#include "../ICoreSystem.h"

extern ICoreSystem* g_pCoreSystem;

INNO_PRIVATE_SCOPE GLBloomExtractPass
{
	void initializeShaders();

	EntityID m_entityID;

	GLRenderPassComponent* m_baseGLRPC;
	GLRenderPassComponent* m_downsampleGLRPC_Half;
	GLRenderPassComponent* m_downsampleGLRPC_Quarter;
	GLRenderPassComponent* m_downsampleGLRPC_Eighth;

	GLShaderProgramComponent* m_GLSPC;
	ShaderFilePaths m_shaderFilePaths = { "GL//bloomExtractPass.vert", "", "GL//bloomExtractPass.frag" };
}

bool GLBloomExtractPass::initialize()
{
	m_entityID = InnoMath::createEntityID();

	auto l_FBDesc = GLRenderingSystemComponent::get().deferredPassFBDesc;
	auto l_TextureDesc = GLRenderingSystemComponent::get().deferredPassTextureDesc;
	l_TextureDesc.magFilterMethod = TextureFilterMethod::LINEAR;
	l_TextureDesc.minFilterMethod = TextureFilterMethod::LINEAR;
	m_baseGLRPC = addGLRenderPassComponent(1, l_FBDesc, l_TextureDesc);

	l_FBDesc.sizeX = l_FBDesc.sizeX / 2;
	l_FBDesc.sizeY = l_FBDesc.sizeY / 2;
	l_TextureDesc.width = l_TextureDesc.width / 2;
	l_TextureDesc.height = l_TextureDesc.height / 2;

	m_downsampleGLRPC_Half = addGLRenderPassComponent(1, l_FBDesc, l_TextureDesc);

	l_FBDesc.sizeX = l_FBDesc.sizeX / 2;
	l_FBDesc.sizeY = l_FBDesc.sizeY / 2;
	l_TextureDesc.width = l_TextureDesc.width / 2;
	l_TextureDesc.height = l_TextureDesc.height / 2;

	m_downsampleGLRPC_Quarter = addGLRenderPassComponent(1, l_FBDesc, l_TextureDesc);

	l_FBDesc.sizeX = l_FBDesc.sizeX / 2;
	l_FBDesc.sizeY = l_FBDesc.sizeY / 2;
	l_TextureDesc.width = l_TextureDesc.width / 2;
	l_TextureDesc.height = l_TextureDesc.height / 2;

	m_downsampleGLRPC_Eighth = addGLRenderPassComponent(1, l_FBDesc, l_TextureDesc);

	initializeShaders();

	return true;
}

void GLBloomExtractPass::initializeShaders()
{
	// shader programs and shaders
	auto rhs = addGLShaderProgramComponent(m_entityID);

	initializeGLShaderProgramComponent(rhs, m_shaderFilePaths);

	m_GLSPC = rhs;
}

bool GLBloomExtractPass::update(GLRenderPassComponent* prePassGLRPC)
{
	activateRenderPass(m_baseGLRPC);

	activateShaderProgram(m_GLSPC);

	activateTexture(prePassGLRPC->m_GLTDCs[0], 0);

	auto l_MDC = getGLMeshDataComponent(MeshShapeType::QUAD);
	drawMesh(l_MDC);

	copyColorBuffer(m_baseGLRPC, 0, m_downsampleGLRPC_Half, 0);
	copyColorBuffer(m_baseGLRPC, 0, m_downsampleGLRPC_Quarter, 0);
	copyColorBuffer(m_baseGLRPC, 0, m_downsampleGLRPC_Eighth, 0);

	return true;
}

bool GLBloomExtractPass::resize(unsigned int newSizeX, unsigned int newSizeY)
{
	auto l_newSizeX = newSizeX;
	auto l_newSizeY = newSizeY;

	resizeGLRenderPassComponent(m_baseGLRPC, l_newSizeX, l_newSizeY);

	l_newSizeX = l_newSizeX / 2;
	l_newSizeY = l_newSizeY / 2;

	resizeGLRenderPassComponent(m_downsampleGLRPC_Half, l_newSizeX, l_newSizeY);

	l_newSizeX = l_newSizeX / 2;
	l_newSizeY = l_newSizeY / 2;

	resizeGLRenderPassComponent(m_downsampleGLRPC_Quarter, l_newSizeX, l_newSizeY);

	l_newSizeX = l_newSizeX / 2;
	l_newSizeY = l_newSizeY / 2;

	resizeGLRenderPassComponent(m_downsampleGLRPC_Eighth, l_newSizeX, l_newSizeY);

	return true;
}

bool GLBloomExtractPass::reloadShader()
{
	deleteShaderProgram(m_GLSPC);

	initializeGLShaderProgramComponent(m_GLSPC, m_shaderFilePaths);

	return true;
}

GLRenderPassComponent * GLBloomExtractPass::getGLRPC(unsigned int index)
{
	if (index == 0)
	{
		return m_baseGLRPC;
	}
	else if (index == 1)
	{
		return m_downsampleGLRPC_Half;
	}
	else if (index == 2)
	{
		return m_downsampleGLRPC_Quarter;
	}
	else if (index == 3)
	{
		return m_downsampleGLRPC_Eighth;
	}
	else
	{
		return nullptr;
	}
}