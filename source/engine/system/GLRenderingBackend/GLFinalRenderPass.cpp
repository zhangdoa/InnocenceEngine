#include "GLFinalRenderPass.h"
#include "GLLightPass.h"
#include "GLOpaquePass.h"
#include "GLTransparentPass.h"
#include "GLTerrainPass.h"

#include "GLSkyPass.h"

#include "../../component/GLRenderingSystemComponent.h"

#include "GLRenderingSystemUtilities.h"

#include "../ICoreSystem.h"

extern ICoreSystem* g_pCoreSystem;

using namespace GLRenderingSystemNS;

INNO_PRIVATE_SCOPE GLFinalRenderPass
{
	void initializeTAAPass();
	void bindTAAPassUniformLocations();
	void initializeBloomExtractPass();
	void bindBloomExtractPassUniformLocations();
	void initializeBloomBlurPass();
	void bindBloomBlurPassUniformLocations();
	void initializeBloomMergePass();
	void bindBloomMergePassUniformLocations();
	void initializeMotionBlurPass();
	void bindMotionBlurUniformLocations();
	void initializeBillboardPass();
	void bindBillboardPassUniformLocations();
	void initializeDebuggerPass();
	void bindDebuggerPassUniformLocations();
	void initializeFinalBlendPass();
	void bindFinalBlendPassUniformLocations();

	GLTextureDataComponent* updatePreTAAPass();
	GLTextureDataComponent* updateTAAPass(GLTextureDataComponent* inputGLTDC);
	GLTextureDataComponent* updateTAASharpenPass(GLTextureDataComponent* inputGLTDC);
	GLTextureDataComponent* updateBloomExtractPass(GLTextureDataComponent* inputGLTDC);
	GLTextureDataComponent* updateBloomBlurPass(GLTextureDataComponent* inputGLTDC);
	GLTextureDataComponent* updateBloomMergePass();
	GLTextureDataComponent* updateMotionBlurPass(GLTextureDataComponent * inputGLTDC);
	GLTextureDataComponent* updateBillboardPass();
	GLTextureDataComponent* updateDebuggerPass();
	GLTextureDataComponent* updateFinalBlendPass(GLTextureDataComponent * inputGLTDC);

	EntityID m_entityID;

	bool m_isTAAPingPass = true;

	GLRenderPassComponent* m_preTAAPassGLRPC;
	GLShaderProgramComponent* m_preTAAPassGLSPC;
	ShaderFilePaths m_preTAAPassShaderFilePaths = { "GL//preTAAPassVertex.sf", "", "GL//preTAAPassFragment.sf" };

	std::vector<std::string> m_preTAAPassUniformNames =
	{
		"uni_lightPassRT0",
		"uni_transparentPassRT0",
		"uni_transparentPassRT1",
		"uni_skyPassRT0",
		"uni_terrainPassRT0",
	};

	GLRenderPassComponent* m_TAAHistory0PassGLRPC;
	GLRenderPassComponent* m_TAAHistory1PassGLRPC;
	GLRenderPassComponent* m_TAAHistory2PassGLRPC;

	GLRenderPassComponent* m_TAAPingPassGLRPC;
	GLRenderPassComponent* m_TAAPongPassGLRPC;
	GLShaderProgramComponent* m_TAAPassGLSPC;
	ShaderFilePaths m_TAAPassShaderFilePaths = { "GL//TAAPassVertex.sf", "", "GL//TAAPassFragment.sf" };

	std::vector<std::string> m_TAAPassUniformNames =
	{
		"uni_preTAAPassRT0",
		"uni_history0",
		"uni_history1",
		"uni_history2",
		"uni_history3",
		"uni_motionVectorTexture",
	};

	GLRenderPassComponent* m_TAASharpenPassGLRPC;
	GLShaderProgramComponent* m_TAASharpenPassGLSPC;
	ShaderFilePaths m_TAASharpenPassShaderFilePaths = { "GL//TAASharpenPassVertex.sf", "", "GL//TAASharpenPassFragment.sf" };

	std::vector<std::string> m_TAASharpenPassUniformNames =
	{
		"uni_lastTAAPassRT0",
	};

	GLRenderPassComponent* m_bloomExtractPassGLRPC;
	GLShaderProgramComponent* m_bloomExtractPassGLSPC;
	ShaderFilePaths m_bloomExtractPassShaderFilePaths = { "GL//bloomExtractPassVertex.sf", "", "GL//bloomExtractPassFragment.sf" };

	std::vector<std::string> m_bloomExtractPassUniformNames =
	{
		"uni_TAAPassRT0",
	};

	GLRenderPassComponent* m_bloomDownsampleGLRPC_Half;
	GLRenderPassComponent* m_bloomDownsampleGLRPC_Quarter;
	GLRenderPassComponent* m_bloomDownsampleGLRPC_Eighth;

	GLRenderPassComponent* m_bloomBlurPingPassGLRPC;
	GLRenderPassComponent* m_bloomBlurPongPassGLRPC;
	GLShaderProgramComponent* m_bloomBlurPassGLSPC;
	ShaderFilePaths m_bloomBlurPassShaderFilePaths = { "GL//bloomBlurPassVertex.sf", "", "GL//bloomBlurPassFragment.sf" };

	std::vector<std::string> m_bloomBlurPassUniformNames =
	{
		"uni_bloomExtractPassRT0",
	};

	GLuint m_bloomBlurPass_uni_horizontal;

	GLRenderPassComponent* m_bloomMergePassGLRPC;
	GLShaderProgramComponent* m_bloomMergePassGLSPC;
	ShaderFilePaths m_bloomMergePassShaderFilePaths = { "GL//bloomMergePassVertex.sf", "", "GL//bloomMergePassFragment.sf" };

	std::vector<std::string> m_bloomMergePassUniformNames =
	{
		"uni_Full",
		"uni_Half",
		"uni_Quarter",
		"uni_Eighth",
	};

	GLRenderPassComponent* m_motionBlurPassGLRPC;
	GLShaderProgramComponent* m_motionBlurPassGLSPC;
	ShaderFilePaths m_motionBlurPassShaderFilePaths = { "GL//motionBlurPassVertex.sf", "", "GL//motionBlurPassFragment.sf" };

	std::vector<std::string> m_motionBlurPassUniformNames =
	{
		"uni_motionVectorTexture",
		"uni_TAAPassRT0",
	};

	GLRenderPassComponent* m_billboardPassGLRPC;
	GLShaderProgramComponent* m_billboardPassGLSPC;
	ShaderFilePaths m_billboardPassShaderFilePaths = { "GL//billboardPassVertex.sf", "", "GL//billboardPassFragment.sf" };

	std::vector<std::string> m_billboardPassUniformNames =
	{
		"uni_texture",
	};

	GLuint m_billboardPass_uni_p;
	GLuint m_billboardPass_uni_r;
	GLuint m_billboardPass_uni_t;
	GLuint m_billboardPass_uni_pos;
	GLuint m_billboardPass_uni_size;

	GLRenderPassComponent* m_debuggerPassGLRPC;
	GLShaderProgramComponent* m_debuggerPassGLSPC;
	ShaderFilePaths m_debuggerPassShaderFilePaths = { "GL//wireframeOverlayPassVertex.sf", "", "GL//wireframeOverlayPassFragment.sf" };
	//ShaderFilePaths m_debuggerPassShaderFilePaths = { "GL//debuggerPassVertex.sf", "GL//debuggerPassGeometry.sf", "GL//debuggerPassFragment.sf" };

	std::vector<std::string> m_debuggerPassUniformNames =
	{
		"uni_normalTexture",
	};

	GLuint m_debuggerPass_uni_p;
	GLuint m_debuggerPass_uni_r;
	GLuint m_debuggerPass_uni_t;
	GLuint m_debuggerPass_uni_m;

	GLRenderPassComponent* m_finalBlendPassGLRPC;
	GLShaderProgramComponent* m_finalBlendPassGLSPC;
	ShaderFilePaths m_finalBlendPassShaderFilePaths = { "GL//finalBlendPassVertex.sf", "", "GL//finalBlendPassFragment.sf" };

	std::vector<std::string> m_finalBlendPassUniformNames =
	{
		"uni_basePassRT0",
		"uni_bloomPassRT0",
		"uni_billboardPassRT0",
		"uni_debuggerPassRT0",
	};
}

void GLFinalRenderPass::initialize()
{
	m_entityID = InnoMath::createEntityID();

	GLSkyPass::initialize();

	initializeTAAPass();
	initializeBloomExtractPass();
	initializeBloomBlurPass();
	initializeBloomMergePass();
	initializeMotionBlurPass();
	initializeBillboardPass();
	initializeDebuggerPass();
	initializeFinalBlendPass();
}

void GLFinalRenderPass::initializeTAAPass()
{
	// pre mix pass
	m_preTAAPassGLRPC = addGLRenderPassComponent(1, GLRenderingSystemComponent::get().deferredPassFBDesc, GLRenderingSystemComponent::get().deferredPassTextureDesc);

	// history buffer pass
	m_TAAHistory0PassGLRPC = addGLRenderPassComponent(1, GLRenderingSystemComponent::get().deferredPassFBDesc, GLRenderingSystemComponent::get().deferredPassTextureDesc);
	m_TAAHistory1PassGLRPC = addGLRenderPassComponent(1, GLRenderingSystemComponent::get().deferredPassFBDesc, GLRenderingSystemComponent::get().deferredPassTextureDesc);
	m_TAAHistory2PassGLRPC = addGLRenderPassComponent(1, GLRenderingSystemComponent::get().deferredPassFBDesc, GLRenderingSystemComponent::get().deferredPassTextureDesc);

	// Ping pass
	m_TAAPingPassGLRPC = addGLRenderPassComponent(1, GLRenderingSystemComponent::get().deferredPassFBDesc, GLRenderingSystemComponent::get().deferredPassTextureDesc);

	// Pong pass
	m_TAAPongPassGLRPC = addGLRenderPassComponent(1, GLRenderingSystemComponent::get().deferredPassFBDesc, GLRenderingSystemComponent::get().deferredPassTextureDesc);

	// Sharpen pass
	m_TAASharpenPassGLRPC = addGLRenderPassComponent(1, GLRenderingSystemComponent::get().deferredPassFBDesc, GLRenderingSystemComponent::get().deferredPassTextureDesc);

	// shader programs and shaders
	// pre mix pass
	auto rhs = addGLShaderProgramComponent(m_entityID);

	initializeGLShaderProgramComponent(rhs, m_preTAAPassShaderFilePaths);

	m_preTAAPassGLSPC = rhs;

	// TAA pass
	rhs = addGLShaderProgramComponent(m_entityID);

	initializeGLShaderProgramComponent(rhs, m_TAAPassShaderFilePaths);

	m_TAAPassGLSPC = rhs;

	// Sharpen pass
	rhs = addGLShaderProgramComponent(m_entityID);

	initializeGLShaderProgramComponent(rhs, m_TAASharpenPassShaderFilePaths);

	m_TAASharpenPassGLSPC = rhs;

	bindTAAPassUniformLocations();
}

void GLFinalRenderPass::bindTAAPassUniformLocations()
{
	updateTextureUniformLocations(m_preTAAPassGLSPC->m_program, m_preTAAPassUniformNames);

	updateTextureUniformLocations(m_TAAPassGLSPC->m_program, m_TAAPassUniformNames);
}

void GLFinalRenderPass::initializeBloomExtractPass()
{
	auto l_FBDesc = GLRenderingSystemComponent::get().deferredPassFBDesc;
	auto l_TextureDesc = GLRenderingSystemComponent::get().deferredPassTextureDesc;
	l_TextureDesc.textureMagFilterMethod = TextureFilterMethod::LINEAR;
	l_TextureDesc.textureMinFilterMethod = TextureFilterMethod::LINEAR;
	m_bloomExtractPassGLRPC = addGLRenderPassComponent(2, l_FBDesc, l_TextureDesc);

	l_FBDesc.sizeX = l_FBDesc.sizeX / 2;
	l_FBDesc.sizeY = l_FBDesc.sizeY / 2;
	l_TextureDesc.textureWidth = l_TextureDesc.textureWidth / 2;
	l_TextureDesc.textureHeight = l_TextureDesc.textureHeight / 2;

	m_bloomDownsampleGLRPC_Half = addGLRenderPassComponent(2, l_FBDesc, l_TextureDesc);

	l_FBDesc.sizeX = l_FBDesc.sizeX / 2;
	l_FBDesc.sizeY = l_FBDesc.sizeY / 2;
	l_TextureDesc.textureWidth = l_TextureDesc.textureWidth / 2;
	l_TextureDesc.textureHeight = l_TextureDesc.textureHeight / 2;

	m_bloomDownsampleGLRPC_Quarter = addGLRenderPassComponent(2, l_FBDesc, l_TextureDesc);

	l_FBDesc.sizeX = l_FBDesc.sizeX / 2;
	l_FBDesc.sizeY = l_FBDesc.sizeY / 2;
	l_TextureDesc.textureWidth = l_TextureDesc.textureWidth / 2;
	l_TextureDesc.textureHeight = l_TextureDesc.textureHeight / 2;

	m_bloomDownsampleGLRPC_Eighth = addGLRenderPassComponent(2, l_FBDesc, l_TextureDesc);

	// shader programs and shaders
	auto rhs = addGLShaderProgramComponent(m_entityID);

	initializeGLShaderProgramComponent(rhs, m_bloomExtractPassShaderFilePaths);

	m_bloomExtractPassGLSPC = rhs;

	bindBloomExtractPassUniformLocations();
}

void GLFinalRenderPass::bindBloomExtractPassUniformLocations()
{
	updateTextureUniformLocations(m_bloomExtractPassGLSPC->m_program, m_bloomExtractPassUniformNames);
}

void GLFinalRenderPass::initializeBloomBlurPass()
{
	//Ping pass
	m_bloomBlurPingPassGLRPC = addGLRenderPassComponent(1, GLRenderingSystemComponent::get().deferredPassFBDesc, GLRenderingSystemComponent::get().deferredPassTextureDesc);

	//Pong pass
	m_bloomBlurPongPassGLRPC = addGLRenderPassComponent(1, GLRenderingSystemComponent::get().deferredPassFBDesc, GLRenderingSystemComponent::get().deferredPassTextureDesc);

	// shader programs and shaders
	auto rhs = addGLShaderProgramComponent(m_entityID);

	initializeGLShaderProgramComponent(rhs, m_bloomBlurPassShaderFilePaths);

	m_bloomBlurPassGLSPC = rhs;

	bindBloomBlurPassUniformLocations();
}

void GLFinalRenderPass::bindBloomBlurPassUniformLocations()
{
	updateTextureUniformLocations(m_bloomBlurPassGLSPC->m_program, m_bloomBlurPassUniformNames);

	m_bloomBlurPass_uni_horizontal = getUniformLocation(
		m_bloomBlurPassGLSPC->m_program,
		"uni_horizontal");
}

void GLFinalRenderPass::initializeBloomMergePass()
{
	m_bloomMergePassGLRPC = addGLRenderPassComponent(1, GLRenderingSystemComponent::get().deferredPassFBDesc, GLRenderingSystemComponent::get().deferredPassTextureDesc);

	// shader programs and shaders
	auto rhs = addGLShaderProgramComponent(m_entityID);

	initializeGLShaderProgramComponent(rhs, m_bloomMergePassShaderFilePaths);

	m_bloomMergePassGLSPC = rhs;

	bindBloomMergePassUniformLocations();
}

void GLFinalRenderPass::bindBloomMergePassUniformLocations()
{
	updateTextureUniformLocations(m_bloomMergePassGLSPC->m_program, m_bloomMergePassUniformNames);
}

void GLFinalRenderPass::initializeMotionBlurPass()
{
	m_motionBlurPassGLRPC = addGLRenderPassComponent(1, GLRenderingSystemComponent::get().deferredPassFBDesc, GLRenderingSystemComponent::get().deferredPassTextureDesc);

	// shader programs and shaders
	auto rhs = addGLShaderProgramComponent(m_entityID);

	initializeGLShaderProgramComponent(rhs, m_motionBlurPassShaderFilePaths);

	m_motionBlurPassGLSPC = rhs;

	bindMotionBlurUniformLocations();
}

void GLFinalRenderPass::bindMotionBlurUniformLocations()
{
	updateTextureUniformLocations(m_motionBlurPassGLSPC->m_program, m_motionBlurPassUniformNames);
}

void GLFinalRenderPass::initializeBillboardPass()
{
	m_billboardPassGLRPC = addGLRenderPassComponent(1, GLRenderingSystemComponent::get().deferredPassFBDesc, GLRenderingSystemComponent::get().deferredPassTextureDesc);

	// shader programs and shaders
	auto rhs = addGLShaderProgramComponent(m_entityID);

	initializeGLShaderProgramComponent(rhs, m_billboardPassShaderFilePaths);

	m_billboardPassGLSPC = rhs;

	bindBillboardPassUniformLocations();
}

void GLFinalRenderPass::bindBillboardPassUniformLocations()
{
	updateTextureUniformLocations(m_billboardPassGLSPC->m_program, m_billboardPassUniformNames);

	m_billboardPass_uni_p = getUniformLocation(
		m_billboardPassGLSPC->m_program,
		"uni_p");
	m_billboardPass_uni_r = getUniformLocation(
		m_billboardPassGLSPC->m_program,
		"uni_r");
	m_billboardPass_uni_t = getUniformLocation(
		m_billboardPassGLSPC->m_program,
		"uni_t");
	m_billboardPass_uni_pos = getUniformLocation(
		m_billboardPassGLSPC->m_program,
		"uni_pos");
	m_billboardPass_uni_size = getUniformLocation(
		m_billboardPassGLSPC->m_program,
		"uni_size");
}

void GLFinalRenderPass::initializeDebuggerPass()
{
	m_debuggerPassGLRPC = addGLRenderPassComponent(1, GLRenderingSystemComponent::get().deferredPassFBDesc, GLRenderingSystemComponent::get().deferredPassTextureDesc);

	// shader programs and shaders
	auto rhs = addGLShaderProgramComponent(m_entityID);

	initializeGLShaderProgramComponent(rhs, m_debuggerPassShaderFilePaths);

	m_debuggerPassGLSPC = rhs;

	bindDebuggerPassUniformLocations();
}

void GLFinalRenderPass::bindDebuggerPassUniformLocations()
{
	updateTextureUniformLocations(m_debuggerPassGLSPC->m_program, m_debuggerPassUniformNames);

	m_debuggerPass_uni_p = getUniformLocation(
		m_debuggerPassGLSPC->m_program,
		"uni_p");
	m_debuggerPass_uni_r = getUniformLocation(
		m_debuggerPassGLSPC->m_program,
		"uni_r");
	m_debuggerPass_uni_t = getUniformLocation(
		m_debuggerPassGLSPC->m_program,
		"uni_t");
	m_debuggerPass_uni_m = getUniformLocation(
		m_debuggerPassGLSPC->m_program,
		"uni_m");
}

void GLFinalRenderPass::initializeFinalBlendPass()
{
	m_finalBlendPassGLRPC = addGLRenderPassComponent(1, GLRenderingSystemComponent::get().deferredPassFBDesc, GLRenderingSystemComponent::get().deferredPassTextureDesc);

	// shader programs and shaders
	auto rhs = addGLShaderProgramComponent(m_entityID);

	initializeGLShaderProgramComponent(rhs, m_finalBlendPassShaderFilePaths);

	m_finalBlendPassGLSPC = rhs;

	bindFinalBlendPassUniformLocations();
}

void GLFinalRenderPass::bindFinalBlendPassUniformLocations()
{
	updateTextureUniformLocations(m_finalBlendPassGLSPC->m_program, m_finalBlendPassUniformNames);
}

void GLFinalRenderPass::update()
{
	auto l_renderingConfig = g_pCoreSystem->getVisionSystem()->getRenderingFrontend()->getRenderingConfig();

	GLSkyPass::update();

	auto preTAAPassResult = updatePreTAAPass();

	GLTextureDataComponent* finalInputGLTDC;

	finalInputGLTDC = updateMotionBlurPass(preTAAPassResult);

	if (l_renderingConfig.useTAA)
	{
		auto TAAPassResult = updateTAAPass(finalInputGLTDC);

		finalInputGLTDC = updateTAASharpenPass(TAAPassResult);
	}

	if (l_renderingConfig.useBloom)
	{
		auto bloomExtractPassResult = updateBloomExtractPass(finalInputGLTDC);
		copyColorBuffer(m_bloomExtractPassGLRPC, 0, m_bloomDownsampleGLRPC_Half, 0);
		copyColorBuffer(m_bloomExtractPassGLRPC, 0, m_bloomDownsampleGLRPC_Quarter, 0);
		copyColorBuffer(m_bloomExtractPassGLRPC, 0, m_bloomDownsampleGLRPC_Eighth, 0);

		//glEnable(GL_STENCIL_TEST);
		//glClear(GL_STENCIL_BUFFER_BIT);

		//glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
		//glStencilFunc(GL_EQUAL, 0x02, 0xFF);
		//glStencilMask(0x00);

		//copyColorBuffer(GLLightRenderPassComponent::get().m_GLRPC,
		//	m_bloomExtractPassGLRPC);

		//glDisable(GL_STENCIL_TEST);

		updateBloomBlurPass(bloomExtractPassResult);
		copyColorBuffer(m_bloomBlurPongPassGLRPC, 0, m_bloomExtractPassGLRPC, 0);

		updateBloomBlurPass(m_bloomDownsampleGLRPC_Half->m_GLTDCs[0]);
		copyColorBuffer(m_bloomBlurPongPassGLRPC, 0, m_bloomDownsampleGLRPC_Half, 0);

		updateBloomBlurPass(m_bloomDownsampleGLRPC_Quarter->m_GLTDCs[0]);
		copyColorBuffer(m_bloomBlurPongPassGLRPC, 0, m_bloomDownsampleGLRPC_Quarter, 0);

		updateBloomBlurPass(m_bloomDownsampleGLRPC_Eighth->m_GLTDCs[0]);
		copyColorBuffer(m_bloomBlurPongPassGLRPC, 0, m_bloomDownsampleGLRPC_Eighth, 0);

		updateBloomMergePass();
	}
	else
	{
		cleanFBC(m_bloomExtractPassGLRPC);
		cleanFBC(m_bloomDownsampleGLRPC_Half);
		cleanFBC(m_bloomDownsampleGLRPC_Quarter);
		cleanFBC(m_bloomDownsampleGLRPC_Eighth);

		cleanFBC(m_bloomBlurPingPassGLRPC);
		cleanFBC(m_bloomBlurPongPassGLRPC);

		cleanFBC(m_bloomMergePassGLRPC);
	}

	updateBillboardPass();

	if (l_renderingConfig.drawDebugObject)
	{
		updateDebuggerPass();
	}
	else
	{
		cleanFBC(m_debuggerPassGLRPC);
	}

	updateFinalBlendPass(finalInputGLTDC);
}

GLTextureDataComponent* GLFinalRenderPass::updatePreTAAPass()
{
	activateRenderPass(m_preTAAPassGLRPC);

	activateShaderProgram(m_preTAAPassGLSPC);

	activateTexture(
		GLLightPass::getGLRPC()->m_GLTDCs[0],
		0);
	activateTexture(
		GLTransparentPass::getGLRPC()->m_GLTDCs[0],
		1);
	activateTexture(
		GLTransparentPass::getGLRPC()->m_GLTDCs[1],
		2);
	activateTexture(
		GLSkyPass::getGLRPC()->m_GLTDCs[0],
		3);
	activateTexture(
		GLTerrainPass::getGLRPC()->m_GLTDCs[0],
		4);

	auto l_MDC = g_pCoreSystem->getAssetSystem()->getMeshDataComponent(MeshShapeType::QUAD);
	drawMesh(l_MDC);

	return m_preTAAPassGLRPC->m_GLTDCs[0];
}

GLTextureDataComponent* GLFinalRenderPass::updateTAAPass(GLTextureDataComponent* inputGLTDC)
{
	GLTextureDataComponent* l_currentFrameGLTDC;
	GLTextureDataComponent* l_lastFrameGLTDC;
	GLRenderPassComponent* l_currentFrameGLRPC;

	if (m_isTAAPingPass)
	{
		l_currentFrameGLTDC = m_TAAPingPassGLRPC->m_GLTDCs[0];
		l_lastFrameGLTDC = m_TAAPongPassGLRPC->m_GLTDCs[0];

		l_currentFrameGLRPC = m_TAAPingPassGLRPC;

		m_isTAAPingPass = false;
	}
	else
	{
		l_currentFrameGLTDC = m_TAAPongPassGLRPC->m_GLTDCs[0];
		l_lastFrameGLTDC = m_TAAPingPassGLRPC->m_GLTDCs[0];

		l_currentFrameGLRPC = m_TAAPongPassGLRPC;

		m_isTAAPingPass = true;
	}

	copyColorBuffer(m_TAAHistory1PassGLRPC, 0, m_TAAHistory0PassGLRPC, 0);
	copyColorBuffer(m_TAAHistory2PassGLRPC, 0, m_TAAHistory1PassGLRPC, 0);
	copyColorBuffer(l_currentFrameGLRPC, 0, m_TAAHistory2PassGLRPC, 0);

	activateShaderProgram(m_TAAPassGLSPC);

	activateRenderPass(l_currentFrameGLRPC);

	activateTexture(inputGLTDC, 0);
	activateTexture(m_TAAHistory0PassGLRPC->m_GLTDCs[0], 1);
	activateTexture(m_TAAHistory1PassGLRPC->m_GLTDCs[0], 2);
	activateTexture(m_TAAHistory2PassGLRPC->m_GLTDCs[0], 3);
	activateTexture(l_lastFrameGLTDC, 4);
	activateTexture(GLOpaquePass::getGLRPC()->m_GLTDCs[3], 5);

	auto l_MDC = g_pCoreSystem->getAssetSystem()->getMeshDataComponent(MeshShapeType::QUAD);
	drawMesh(l_MDC);

	return l_currentFrameGLTDC;
}

GLTextureDataComponent* GLFinalRenderPass::updateTAASharpenPass(GLTextureDataComponent * inputGLTDC)
{
	activateRenderPass(m_TAASharpenPassGLRPC);

	activateShaderProgram(m_TAASharpenPassGLSPC);

	activateTexture(
		inputGLTDC,
		0);

	auto l_MDC = g_pCoreSystem->getAssetSystem()->getMeshDataComponent(MeshShapeType::QUAD);
	drawMesh(l_MDC);

	return m_TAASharpenPassGLRPC->m_GLTDCs[0];
}

GLTextureDataComponent* GLFinalRenderPass::updateBloomExtractPass(GLTextureDataComponent * inputGLTDC)
{
	activateRenderPass(m_bloomExtractPassGLRPC);

	activateShaderProgram(m_bloomExtractPassGLSPC);

	activateTexture(inputGLTDC, 0);

	auto l_MDC = g_pCoreSystem->getAssetSystem()->getMeshDataComponent(MeshShapeType::QUAD);
	drawMesh(l_MDC);

	return m_bloomExtractPassGLRPC->m_GLTDCs[0];
}

GLTextureDataComponent* GLFinalRenderPass::updateBloomBlurPass(GLTextureDataComponent * inputGLTDC)
{
	GLTextureDataComponent* l_currentFrameBloomBlurGLTDC = m_bloomBlurPingPassGLRPC->m_GLTDCs[0];
	GLTextureDataComponent* l_lastFrameBloomBlurGLTDC = m_bloomBlurPongPassGLRPC->m_GLTDCs[0];

	activateShaderProgram(m_bloomBlurPassGLSPC);

	bool l_isPing = true;
	bool l_isFirstIteration = true;

	auto l_MDC = g_pCoreSystem->getAssetSystem()->getMeshDataComponent(MeshShapeType::QUAD);

	for (size_t i = 0; i < 5; i++)
	{
		if (l_isPing)
		{
			l_currentFrameBloomBlurGLTDC = m_bloomBlurPingPassGLRPC->m_GLTDCs[0];
			l_lastFrameBloomBlurGLTDC = m_bloomBlurPongPassGLRPC->m_GLTDCs[0];

			activateRenderPass(m_bloomBlurPingPassGLRPC);

			updateUniform(
				m_bloomBlurPass_uni_horizontal,
				true);

			if (l_isFirstIteration)
			{
				activateTexture(
					inputGLTDC,
					0);
				l_isFirstIteration = false;
			}
			else
			{
				activateTexture(
					l_lastFrameBloomBlurGLTDC,
					0);
			}

			drawMesh(l_MDC);

			l_isPing = false;
		}
		else
		{
			l_currentFrameBloomBlurGLTDC = m_bloomBlurPongPassGLRPC->m_GLTDCs[0];
			l_lastFrameBloomBlurGLTDC = m_bloomBlurPingPassGLRPC->m_GLTDCs[0];

			activateRenderPass(m_bloomBlurPongPassGLRPC);

			updateUniform(
				m_bloomBlurPass_uni_horizontal,
				false);

			activateTexture(
				l_lastFrameBloomBlurGLTDC,
				0);

			drawMesh(l_MDC);

			l_isPing = true;
		}
	}

	return l_currentFrameBloomBlurGLTDC;
}
GLTextureDataComponent* GLFinalRenderPass::updateBloomMergePass()
{
	activateRenderPass(m_bloomMergePassGLRPC);

	activateShaderProgram(m_bloomMergePassGLSPC);

	activateTexture(
		m_bloomExtractPassGLRPC->m_GLTDCs[0],
		0);
	activateTexture(
		m_bloomDownsampleGLRPC_Half->m_GLTDCs[0],
		1);
	activateTexture(
		m_bloomDownsampleGLRPC_Quarter->m_GLTDCs[0],
		2);
	activateTexture(
		m_bloomDownsampleGLRPC_Eighth->m_GLTDCs[0],
		3);

	auto l_MDC = g_pCoreSystem->getAssetSystem()->getMeshDataComponent(MeshShapeType::QUAD);
	drawMesh(l_MDC);

	return m_bloomMergePassGLRPC->m_GLTDCs[0];
}

GLTextureDataComponent* GLFinalRenderPass::updateMotionBlurPass(GLTextureDataComponent * inputGLTDC)
{
	activateRenderPass(m_motionBlurPassGLRPC);

	activateShaderProgram(m_motionBlurPassGLSPC);

	activateTexture(
		GLOpaquePass::getGLRPC()->m_GLTDCs[3],
		0);
	activateTexture(inputGLTDC, 1);

	auto l_MDC = g_pCoreSystem->getAssetSystem()->getMeshDataComponent(MeshShapeType::QUAD);
	drawMesh(l_MDC);

	return m_motionBlurPassGLRPC->m_GLTDCs[0];
}

GLTextureDataComponent* GLFinalRenderPass::updateBillboardPass()
{
	auto l_cameraDataPack = g_pCoreSystem->getVisionSystem()->getRenderingFrontend()->getCameraDataPack();

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);

	activateRenderPass(m_billboardPassGLRPC);

	// copy depth buffer from G-Pass
	copyDepthBuffer(GLOpaquePass::getGLRPC(), m_billboardPassGLRPC);

	activateShaderProgram(m_billboardPassGLSPC);

	updateUniform(
		m_billboardPass_uni_p,
		l_cameraDataPack.p_Original);
	updateUniform(
		m_billboardPass_uni_r,
		l_cameraDataPack.r);
	updateUniform(
		m_billboardPass_uni_t,
		l_cameraDataPack.t);

	while (GLRenderingSystemComponent::get().m_billboardPassDataQueue.size() > 0)
	{
		auto l_renderPack = GLRenderingSystemComponent::get().m_billboardPassDataQueue.front();

		auto l_GlobalPos = l_renderPack.globalPos;

		updateUniform(
			m_billboardPass_uni_pos,
			l_GlobalPos.x, l_GlobalPos.y, l_GlobalPos.z);

		auto l_distanceToCamera = l_renderPack.distanceToCamera;

		if (l_distanceToCamera > 1.0f)
		{
			updateUniform(
				m_billboardPass_uni_size,
				(1.0f / (l_distanceToCamera * l_cameraDataPack.WHRatio)), (1.0f / l_distanceToCamera));
		}
		else
		{
			updateUniform(
				m_billboardPass_uni_size,
				(1.0f / l_cameraDataPack.WHRatio), 1.0f);
		}

		GLTextureDataComponent* l_iconTexture = 0;

		switch (l_renderPack.iconType)
		{
		case WorldEditorIconType::DIRECTIONAL_LIGHT: l_iconTexture = GLRenderingSystemComponent::get().m_iconTemplate_DirectionalLight; break;
		case WorldEditorIconType::POINT_LIGHT: l_iconTexture = GLRenderingSystemComponent::get().m_iconTemplate_PointLight; break;
		case WorldEditorIconType::SPHERE_LIGHT: l_iconTexture = GLRenderingSystemComponent::get().m_iconTemplate_SphereLight; break;
		default:
			break;
		}

		activateTexture(l_iconTexture, 0);

		drawMesh(6, MeshPrimitiveTopology::TRIANGLE_STRIP, GLRenderingSystemComponent::get().m_UnitQuadGLMDC);

		GLRenderingSystemComponent::get().m_billboardPassDataQueue.pop();
	}

	glDisable(GL_DEPTH_TEST);

	return m_billboardPassGLRPC->m_GLTDCs[0];
}

GLTextureDataComponent* GLFinalRenderPass::updateDebuggerPass()
{
	auto l_cameraDataPack = g_pCoreSystem->getVisionSystem()->getRenderingFrontend()->getCameraDataPack();

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);

	activateRenderPass(m_debuggerPassGLRPC);

	// copy depth buffer from G-Pass
	copyDepthBuffer(GLOpaquePass::getGLRPC(), m_debuggerPassGLRPC);

	activateShaderProgram(m_debuggerPassGLSPC);

	updateUniform(
		m_debuggerPass_uni_p,
		l_cameraDataPack.p_Original);
	updateUniform(
		m_debuggerPass_uni_r,
		l_cameraDataPack.r);
	updateUniform(
		m_debuggerPass_uni_t,
		l_cameraDataPack.t);

	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	while (GLRenderingSystemComponent::get().m_debuggerPassDataQueue.size() > 0)
	{
		auto l_renderPack = GLRenderingSystemComponent::get().m_debuggerPassDataQueue.front();

		auto l_m = l_renderPack.m;

		updateUniform(
			m_debuggerPass_uni_m,
			l_m);

		drawMesh(l_renderPack.indiceSize, l_renderPack.meshPrimitiveTopology, l_renderPack.GLMDC);

		GLRenderingSystemComponent::get().m_debuggerPassDataQueue.pop();
	}

	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glDisable(GL_DEPTH_TEST);

	return m_debuggerPassGLRPC->m_GLTDCs[0];
}

GLTextureDataComponent* GLFinalRenderPass::updateFinalBlendPass(GLTextureDataComponent * inputGLTDC)
{
	activateRenderPass(m_finalBlendPassGLRPC);

	activateShaderProgram(m_finalBlendPassGLSPC);

	// last pass rendering target as the mixing background
	activateTexture(
		inputGLTDC,
		0);
	// bloom pass rendering target
	activateTexture(
		m_bloomMergePassGLRPC->m_GLTDCs[0],
		1);
	// billboard pass rendering target
	activateTexture(
		m_billboardPassGLRPC->m_GLTDCs[0],
		2);
	// debugger pass rendering target
	activateTexture(
		m_debuggerPassGLRPC->m_GLTDCs[0],
		3);
	// draw final pass rectangle
	auto l_MDC = g_pCoreSystem->getAssetSystem()->getMeshDataComponent(MeshShapeType::QUAD);
	drawMesh(l_MDC);

	// draw again for game build
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);

	drawMesh(l_MDC);

	return m_finalBlendPassGLRPC->m_GLTDCs[0];
}

bool GLFinalRenderPass::resize()
{
	GLSkyPass::resize();
	resizeGLRenderPassComponent(m_preTAAPassGLRPC, GLRenderingSystemComponent::get().deferredPassFBDesc);
	resizeGLRenderPassComponent(m_TAAPingPassGLRPC, GLRenderingSystemComponent::get().deferredPassFBDesc);
	resizeGLRenderPassComponent(m_TAAPongPassGLRPC, GLRenderingSystemComponent::get().deferredPassFBDesc);
	resizeGLRenderPassComponent(m_TAASharpenPassGLRPC, GLRenderingSystemComponent::get().deferredPassFBDesc);
	resizeGLRenderPassComponent(m_bloomExtractPassGLRPC, GLRenderingSystemComponent::get().deferredPassFBDesc);
	resizeGLRenderPassComponent(m_bloomBlurPingPassGLRPC, GLRenderingSystemComponent::get().deferredPassFBDesc);
	resizeGLRenderPassComponent(m_bloomBlurPongPassGLRPC, GLRenderingSystemComponent::get().deferredPassFBDesc);
	resizeGLRenderPassComponent(m_motionBlurPassGLRPC, GLRenderingSystemComponent::get().deferredPassFBDesc);
	resizeGLRenderPassComponent(m_billboardPassGLRPC, GLRenderingSystemComponent::get().deferredPassFBDesc);
	resizeGLRenderPassComponent(m_debuggerPassGLRPC, GLRenderingSystemComponent::get().deferredPassFBDesc);
	resizeGLRenderPassComponent(m_finalBlendPassGLRPC, GLRenderingSystemComponent::get().deferredPassFBDesc);

	return true;
}

bool GLFinalRenderPass::reloadFinalPassShaders()
{
	GLSkyPass::reloadShader();
	deleteShaderProgram(m_preTAAPassGLSPC);
	deleteShaderProgram(m_TAAPassGLSPC);
	deleteShaderProgram(m_TAASharpenPassGLSPC);
	deleteShaderProgram(m_bloomExtractPassGLSPC);
	deleteShaderProgram(m_bloomBlurPassGLSPC);
	deleteShaderProgram(m_motionBlurPassGLSPC);
	deleteShaderProgram(m_billboardPassGLSPC);
	deleteShaderProgram(m_debuggerPassGLSPC);
	deleteShaderProgram(m_finalBlendPassGLSPC);

	initializeGLShaderProgramComponent(m_preTAAPassGLSPC, m_preTAAPassShaderFilePaths);
	initializeGLShaderProgramComponent(m_TAAPassGLSPC, m_TAAPassShaderFilePaths);
	initializeGLShaderProgramComponent(m_TAASharpenPassGLSPC, m_TAASharpenPassShaderFilePaths);
	initializeGLShaderProgramComponent(m_bloomExtractPassGLSPC, m_bloomExtractPassShaderFilePaths);
	initializeGLShaderProgramComponent(m_bloomBlurPassGLSPC, m_bloomBlurPassShaderFilePaths);
	initializeGLShaderProgramComponent(m_motionBlurPassGLSPC, m_motionBlurPassShaderFilePaths);
	initializeGLShaderProgramComponent(m_billboardPassGLSPC, m_billboardPassShaderFilePaths);
	initializeGLShaderProgramComponent(m_debuggerPassGLSPC, m_billboardPassShaderFilePaths);
	initializeGLShaderProgramComponent(m_finalBlendPassGLSPC, m_finalBlendPassShaderFilePaths);

	bindTAAPassUniformLocations();
	bindBloomExtractPassUniformLocations();
	bindBloomBlurPassUniformLocations();
	bindMotionBlurUniformLocations();
	bindBillboardPassUniformLocations();
	bindDebuggerPassUniformLocations();
	bindFinalBlendPassUniformLocations();

	return true;
}