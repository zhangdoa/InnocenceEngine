#include "GLRenderingSystemUtilities.h"
#include "GLFinalRenderingPassUtilities.h"
#include "../component/GLFinalRenderPassComponent.h"
#include "../component/RenderingSystemComponent.h"
#include "../component/GLRenderingSystemComponent.h"
#include "../component/PhysicsSystemComponent.h"
#include "../component/GameSystemComponent.h"
#include "../component/GLGeometryRenderPassComponent.h"
#include "../component/GLTerrainRenderPassComponent.h"
#include "../component/GLLightRenderPassComponent.h"

#include "ICoreSystem.h"

extern ICoreSystem* g_pCoreSystem;

INNO_PRIVATE_SCOPE GLRenderingSystemNS
{
	void initializeSkyPass();
	void bindSkyPassUniformLocations();
	void initializeTAAPass();
	void bindTAAPassUniformLocations();
	void initializeBloomExtractPass();
	void bindBloomExtractPassUniformLocations();
	void initializeBloomBlurPass();
	void bindBloomBlurPassUniformLocations();
	void initializeMotionBlurPass();
	void bindMotionBlurUniformLocations();
	void initializeBillboardPass();
	void bindBillboardPassUniformLocations();
	void initializeDebuggerPass();
	void bindDebuggerPassUniformLocations();
	void initializeFinalBlendPass();
	void bindFinalBlendPassUniformLocations();

	GLTextureDataComponent* updateSkyPass();
	GLTextureDataComponent* updatePreTAAPass();
	GLTextureDataComponent* updateTAAPass(GLTextureDataComponent* inputGLTDC);
	GLTextureDataComponent* updateTAASharpenPass(GLTextureDataComponent* inputGLTDC);
	GLTextureDataComponent* updateBloomExtractPass(GLTextureDataComponent* inputGLTDC);
	GLTextureDataComponent* updateBloomBlurPass(GLTextureDataComponent* inputGLTDC);
	GLTextureDataComponent* updateMotionBlurPass(GLTextureDataComponent * inputGLTDC);
	GLTextureDataComponent* updateBillboardPass();
	GLTextureDataComponent* updateDebuggerPass();
	GLTextureDataComponent* updateFinalBlendPass(GLTextureDataComponent * inputGLTDC);
}

void GLRenderingSystemNS::initializeFinalPass()
{
	initializeSkyPass();
	initializeTAAPass();
	initializeBloomExtractPass();
	initializeBloomBlurPass();
	initializeMotionBlurPass();
	initializeBillboardPass();
	initializeDebuggerPass();
	initializeFinalBlendPass();
}

void GLRenderingSystemNS::initializeSkyPass()
{
	GLFinalRenderPassComponent::get().m_skyPassGLRPC = addGLRenderPassComponent(1, GLRenderingSystemComponent::get().deferredPassFBDesc, GLRenderingSystemComponent::get().deferredPassTextureDesc);

	// shader programs and shaders
	auto rhs = addGLShaderProgramComponent(0);

	initializeGLShaderProgramComponent(rhs, GLFinalRenderPassComponent::get().m_skyPassShaderFilePaths);

	GLFinalRenderPassComponent::get().m_skyPassGLSPC = rhs;

	bindSkyPassUniformLocations();
}

void  GLRenderingSystemNS::bindSkyPassUniformLocations()
{
	GLFinalRenderPassComponent::get().m_skyPass_uni_p = getUniformLocation(
		GLFinalRenderPassComponent::get().m_skyPassGLSPC->m_program,
		"uni_p");
	GLFinalRenderPassComponent::get().m_skyPass_uni_r = getUniformLocation(
		GLFinalRenderPassComponent::get().m_skyPassGLSPC->m_program,
		"uni_r");
	GLFinalRenderPassComponent::get().m_skyPass_uni_viewportSize = getUniformLocation(
		GLFinalRenderPassComponent::get().m_skyPassGLSPC->m_program,
		"uni_viewportSize");
	GLFinalRenderPassComponent::get().m_skyPass_uni_eyePos = getUniformLocation(
		GLFinalRenderPassComponent::get().m_skyPassGLSPC->m_program,
		"uni_eyePos");
	GLFinalRenderPassComponent::get().m_skyPass_uni_lightDir = getUniformLocation(
		GLFinalRenderPassComponent::get().m_skyPassGLSPC->m_program,
		"uni_lightDir");
}

void GLRenderingSystemNS::initializeTAAPass()
{
	// pre mix pass
	GLFinalRenderPassComponent::get().m_preTAAPassGLRPC = addGLRenderPassComponent(1, GLRenderingSystemComponent::get().deferredPassFBDesc, GLRenderingSystemComponent::get().deferredPassTextureDesc);

	// Ping pass
	GLFinalRenderPassComponent::get().m_TAAPingPassGLRPC = addGLRenderPassComponent(1, GLRenderingSystemComponent::get().deferredPassFBDesc, GLRenderingSystemComponent::get().deferredPassTextureDesc);

	// Pong pass
	GLFinalRenderPassComponent::get().m_TAAPongPassGLRPC = addGLRenderPassComponent(1, GLRenderingSystemComponent::get().deferredPassFBDesc, GLRenderingSystemComponent::get().deferredPassTextureDesc);

	// Sharpen pass
	GLFinalRenderPassComponent::get().m_TAASharpenPassGLRPC = addGLRenderPassComponent(1, GLRenderingSystemComponent::get().deferredPassFBDesc, GLRenderingSystemComponent::get().deferredPassTextureDesc);

	// shader programs and shaders
	// pre mix pass
	auto rhs = addGLShaderProgramComponent(0);

	initializeGLShaderProgramComponent(rhs, GLFinalRenderPassComponent::get().m_preTAAPassShaderFilePaths);

	GLFinalRenderPassComponent::get().m_preTAAPassGLSPC = rhs;

	// TAA pass	
	rhs = addGLShaderProgramComponent(0);

	initializeGLShaderProgramComponent(rhs, GLFinalRenderPassComponent::get().m_TAAPassShaderFilePaths);

	GLFinalRenderPassComponent::get().m_TAAPassGLSPC = rhs;

	// Sharpen pass
	rhs = addGLShaderProgramComponent(0);

	initializeGLShaderProgramComponent(rhs, GLFinalRenderPassComponent::get().m_TAASharpenPassShaderFilePaths);

	GLFinalRenderPassComponent::get().m_TAASharpenPassGLSPC = rhs;

	bindTAAPassUniformLocations();
}

void GLRenderingSystemNS::bindTAAPassUniformLocations()
{
	updateTextureUniformLocations(GLFinalRenderPassComponent::get().m_preTAAPassGLSPC->m_program, GLFinalRenderPassComponent::get().m_preTAAPassUniformNames);

	updateTextureUniformLocations(GLFinalRenderPassComponent::get().m_TAAPassGLSPC->m_program, GLFinalRenderPassComponent::get().m_TAAPassUniformNames);
}

void GLRenderingSystemNS::initializeBloomExtractPass()
{
	GLFinalRenderPassComponent::get().m_bloomExtractPassGLRPC = addGLRenderPassComponent(1, GLRenderingSystemComponent::get().deferredPassFBDesc, GLRenderingSystemComponent::get().deferredPassTextureDesc);

	// shader programs and shaders
	auto rhs = addGLShaderProgramComponent(0); 
	
	initializeGLShaderProgramComponent(rhs, GLFinalRenderPassComponent::get().m_bloomExtractPassShaderFilePaths);

	GLFinalRenderPassComponent::get().m_bloomExtractPassGLSPC = rhs;

	bindBloomExtractPassUniformLocations();
}

void GLRenderingSystemNS::bindBloomExtractPassUniformLocations()
{
	updateTextureUniformLocations(GLFinalRenderPassComponent::get().m_bloomExtractPassGLSPC->m_program, GLFinalRenderPassComponent::get().m_bloomExtractPassUniformNames);
}

void GLRenderingSystemNS::initializeBloomBlurPass()
{
	//Ping pass
	GLFinalRenderPassComponent::get().m_bloomBlurPingPassGLRPC = addGLRenderPassComponent(1, GLRenderingSystemComponent::get().deferredPassFBDesc, GLRenderingSystemComponent::get().deferredPassTextureDesc);

	//Pong pass
	GLFinalRenderPassComponent::get().m_bloomBlurPongPassGLRPC = addGLRenderPassComponent(1, GLRenderingSystemComponent::get().deferredPassFBDesc, GLRenderingSystemComponent::get().deferredPassTextureDesc);

	// shader programs and shaders
	auto rhs = addGLShaderProgramComponent(0);

	initializeGLShaderProgramComponent(rhs, GLFinalRenderPassComponent::get().m_bloomBlurPassShaderFilePaths);

	GLFinalRenderPassComponent::get().m_bloomBlurPassGLSPC = rhs;

	bindBloomBlurPassUniformLocations();
}

void GLRenderingSystemNS::bindBloomBlurPassUniformLocations()
{
	updateTextureUniformLocations(GLFinalRenderPassComponent::get().m_bloomBlurPassGLSPC->m_program, GLFinalRenderPassComponent::get().m_bloomBlurPassUniformNames);

	GLFinalRenderPassComponent::get().m_bloomBlurPass_uni_horizontal = getUniformLocation(
		GLFinalRenderPassComponent::get().m_bloomBlurPassGLSPC->m_program,
		"uni_horizontal");
}

void GLRenderingSystemNS::initializeMotionBlurPass()
{
	GLFinalRenderPassComponent::get().m_motionBlurPassGLRPC = addGLRenderPassComponent(1, GLRenderingSystemComponent::get().deferredPassFBDesc, GLRenderingSystemComponent::get().deferredPassTextureDesc);

	// shader programs and shaders
	auto rhs = addGLShaderProgramComponent(0);

	initializeGLShaderProgramComponent(rhs, GLFinalRenderPassComponent::get().m_motionBlurPassShaderFilePaths);

	GLFinalRenderPassComponent::get().m_motionBlurPassGLSPC = rhs;

	bindMotionBlurUniformLocations();
}

void GLRenderingSystemNS::bindMotionBlurUniformLocations()
{
	updateTextureUniformLocations(GLFinalRenderPassComponent::get().m_motionBlurPassGLSPC->m_program, GLFinalRenderPassComponent::get().m_motionBlurPassUniformNames);
}

void GLRenderingSystemNS::initializeBillboardPass()
{
	GLFinalRenderPassComponent::get().m_billboardPassGLRPC = addGLRenderPassComponent(1, GLRenderingSystemComponent::get().deferredPassFBDesc, GLRenderingSystemComponent::get().deferredPassTextureDesc);

	// shader programs and shaders
	auto rhs = addGLShaderProgramComponent(0);

	initializeGLShaderProgramComponent(rhs, GLFinalRenderPassComponent::get().m_billboardPassShaderFilePaths);

	GLFinalRenderPassComponent::get().m_billboardPassGLSPC = rhs;

	bindBillboardPassUniformLocations();
}

void GLRenderingSystemNS::bindBillboardPassUniformLocations()
{
	updateTextureUniformLocations(GLFinalRenderPassComponent::get().m_billboardPassGLSPC->m_program, GLFinalRenderPassComponent::get().m_billboardPassUniformNames);

	GLFinalRenderPassComponent::get().m_billboardPass_uni_p = getUniformLocation(
		GLFinalRenderPassComponent::get().m_billboardPassGLSPC->m_program,
		"uni_p");
	GLFinalRenderPassComponent::get().m_billboardPass_uni_r = getUniformLocation(
		GLFinalRenderPassComponent::get().m_billboardPassGLSPC->m_program,
		"uni_r");
	GLFinalRenderPassComponent::get().m_billboardPass_uni_t = getUniformLocation(
		GLFinalRenderPassComponent::get().m_billboardPassGLSPC->m_program,
		"uni_t");
	GLFinalRenderPassComponent::get().m_billboardPass_uni_pos = getUniformLocation(
		GLFinalRenderPassComponent::get().m_billboardPassGLSPC->m_program,
		"uni_pos");
	GLFinalRenderPassComponent::get().m_billboardPass_uni_albedo = getUniformLocation(
		GLFinalRenderPassComponent::get().m_billboardPassGLSPC->m_program,
		"uni_albedo");
	GLFinalRenderPassComponent::get().m_billboardPass_uni_size = getUniformLocation(
		GLFinalRenderPassComponent::get().m_billboardPassGLSPC->m_program,
		"uni_size");
}

void GLRenderingSystemNS::initializeDebuggerPass()
{
	GLFinalRenderPassComponent::get().m_debuggerPassGLRPC = addGLRenderPassComponent(1, GLRenderingSystemComponent::get().deferredPassFBDesc, GLRenderingSystemComponent::get().deferredPassTextureDesc);

	// shader programs and shaders
	auto rhs = addGLShaderProgramComponent(0);
	
	initializeGLShaderProgramComponent(rhs, GLFinalRenderPassComponent::get().m_debuggerPassShaderFilePaths);
	
	GLFinalRenderPassComponent::get().m_debuggerPassGLSPC = rhs;

	bindDebuggerPassUniformLocations();
}

void GLRenderingSystemNS::bindDebuggerPassUniformLocations()
{
	updateTextureUniformLocations(GLFinalRenderPassComponent::get().m_debuggerPassGLSPC->m_program, GLFinalRenderPassComponent::get().m_debuggerPassUniformNames);

	GLFinalRenderPassComponent::get().m_debuggerPass_uni_p = getUniformLocation(
		GLFinalRenderPassComponent::get().m_debuggerPassGLSPC->m_program,
		"uni_p");
	GLFinalRenderPassComponent::get().m_debuggerPass_uni_r = getUniformLocation(
		GLFinalRenderPassComponent::get().m_debuggerPassGLSPC->m_program,
		"uni_r");
	GLFinalRenderPassComponent::get().m_debuggerPass_uni_t = getUniformLocation(
		GLFinalRenderPassComponent::get().m_debuggerPassGLSPC->m_program,
		"uni_t");
	GLFinalRenderPassComponent::get().m_debuggerPass_uni_m = getUniformLocation(
		GLFinalRenderPassComponent::get().m_debuggerPassGLSPC->m_program,
		"uni_m");
}

void GLRenderingSystemNS::initializeFinalBlendPass()
{
	GLFinalRenderPassComponent::get().m_finalBlendPassGLRPC = addGLRenderPassComponent(1, GLRenderingSystemComponent::get().deferredPassFBDesc, GLRenderingSystemComponent::get().deferredPassTextureDesc);

	// shader programs and shaders
	auto rhs = addGLShaderProgramComponent(0);

	initializeGLShaderProgramComponent(rhs, GLFinalRenderPassComponent::get().m_finalBlendPassShaderFilePaths);

	GLFinalRenderPassComponent::get().m_finalBlendPassGLSPC = rhs;

	bindFinalBlendPassUniformLocations();
}

void GLRenderingSystemNS::bindFinalBlendPassUniformLocations()
{
	updateTextureUniformLocations(GLFinalRenderPassComponent::get().m_finalBlendPassGLSPC->m_program, GLFinalRenderPassComponent::get().m_finalBlendPassUniformNames);
}

void GLRenderingSystemNS::updateFinalPass()
{
	auto skyPassResult = updateSkyPass();

	auto preTAAPassResult = updatePreTAAPass();

	GLTextureDataComponent* finalInputGLTDC;

	finalInputGLTDC = updateMotionBlurPass(preTAAPassResult);

	if (RenderingSystemComponent::get().m_useTAA)
	{
		auto TAAPassResult = updateTAAPass(finalInputGLTDC);

		finalInputGLTDC = updateTAASharpenPass(TAAPassResult);
	}

	if (RenderingSystemComponent::get().m_useBloom)
	{
		auto bloomExtractPassResult = updateBloomExtractPass(finalInputGLTDC);

		//glEnable(GL_STENCIL_TEST);
		//glClear(GL_STENCIL_BUFFER_BIT);

		//glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
		//glStencilFunc(GL_EQUAL, 0x02, 0xFF);
		//glStencilMask(0x00);

		//copyColorBuffer(GLLightRenderPassComponent::get().m_GLRPC->m_GLFBC,
		//	GLFinalRenderPassComponent::get().m_bloomExtractPassGLRPC->m_GLFBC);

		//glDisable(GL_STENCIL_TEST);

		updateBloomBlurPass(bloomExtractPassResult);
	}
	else
	{
		cleanFBC(GLFinalRenderPassComponent::get().m_bloomExtractPassGLRPC->m_GLFBC);
		cleanFBC(GLFinalRenderPassComponent::get().m_bloomBlurPingPassGLRPC->m_GLFBC);
		cleanFBC(GLFinalRenderPassComponent::get().m_bloomBlurPongPassGLRPC->m_GLFBC);
	}

	updateBillboardPass();

	if (RenderingSystemComponent::get().m_drawOverlapWireframe)
	{
		updateDebuggerPass();
	}
	else
	{
		cleanFBC(GLFinalRenderPassComponent::get().m_debuggerPassGLRPC->m_GLFBC);
	}

	updateFinalBlendPass(finalInputGLTDC);
}

GLTextureDataComponent* GLRenderingSystemNS::updateSkyPass()
{
	if (RenderingSystemComponent::get().m_drawSky)
	{
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LEQUAL);

		//glEnable(GL_CULL_FACE);
		//glFrontFace(GL_CW);
		//glCullFace(GL_FRONT);

		// bind to framebuffer
		auto l_FBC = GLFinalRenderPassComponent::get().m_skyPassGLRPC->m_GLFBC;
		bindFBC(l_FBC);

		activateShaderProgram(GLFinalRenderPassComponent::get().m_skyPassGLSPC);

		updateUniform(
			GLFinalRenderPassComponent::get().m_skyPass_uni_p,
			GLRenderingSystemComponent::get().m_CamProjOriginal);
		updateUniform(
			GLFinalRenderPassComponent::get().m_skyPass_uni_r,
			GLRenderingSystemComponent::get().m_CamRot);
		updateUniform(
			GLFinalRenderPassComponent::get().m_skyPass_uni_viewportSize,
			(float)GLRenderingSystemComponent::get().deferredPassFBDesc.sizeX, (float)GLRenderingSystemComponent::get().deferredPassFBDesc.sizeY);
		updateUniform(
			GLFinalRenderPassComponent::get().m_skyPass_uni_eyePos,
			GLRenderingSystemComponent::get().m_CamGlobalPos.x, GLRenderingSystemComponent::get().m_CamGlobalPos.y, GLRenderingSystemComponent::get().m_CamGlobalPos.z);
		updateUniform(
			GLFinalRenderPassComponent::get().m_skyPass_uni_lightDir,
			GLRenderingSystemComponent::get().m_sunDir.x, GLRenderingSystemComponent::get().m_sunDir.y, GLRenderingSystemComponent::get().m_sunDir.z);

		auto l_MDC = g_pCoreSystem->getAssetSystem()->getMeshDataComponent(MeshShapeType::CUBE);
		drawMesh(l_MDC);

		glDisable(GL_CULL_FACE);
		glDisable(GL_DEPTH_TEST);
	}
	else
	{
		GLRenderingSystemNS::cleanFBC(GLFinalRenderPassComponent::get().m_skyPassGLRPC->m_GLFBC);
	}

	return GLFinalRenderPassComponent::get().m_skyPassGLRPC->m_GLTDCs[0];
}

GLTextureDataComponent* GLRenderingSystemNS::updatePreTAAPass()
{
	auto l_FBC = GLFinalRenderPassComponent::get().m_preTAAPassGLRPC->m_GLFBC;
	bindFBC(l_FBC);

	activateShaderProgram(GLFinalRenderPassComponent::get().m_preTAAPassGLSPC);

	activateTexture(
		GLLightRenderPassComponent::get().m_GLRPC->m_GLTDCs[0],
		0);
	activateTexture(
		GLGeometryRenderPassComponent::get().m_transparentPass_GLRPC->m_GLTDCs[0],
		1);
	activateTexture(
		GLFinalRenderPassComponent::get().m_skyPassGLRPC->m_GLTDCs[0],
		2);
	activateTexture(
		GLTerrainRenderPassComponent::get().m_GLRPC->m_GLTDCs[0],
		3);

	auto l_MDC = g_pCoreSystem->getAssetSystem()->getMeshDataComponent(MeshShapeType::QUAD);
	drawMesh(l_MDC);

	return GLFinalRenderPassComponent::get().m_preTAAPassGLRPC->m_GLTDCs[0];
}

GLTextureDataComponent* GLRenderingSystemNS::updateTAAPass(GLTextureDataComponent* inputGLTDC)
{
	GLTextureDataComponent* l_currentFrameTAAGLTDC = GLFinalRenderPassComponent::get().m_TAAPingPassGLRPC->m_GLTDCs[0];
	GLTextureDataComponent* l_lastFrameTAAGLTDC = GLFinalRenderPassComponent::get().m_TAAPongPassGLRPC->m_GLTDCs[0];

	activateShaderProgram(GLFinalRenderPassComponent::get().m_TAAPassGLSPC);

	GLFrameBufferComponent* l_FBC;

	if (RenderingSystemComponent::get().m_isTAAPingPass)
	{
		l_currentFrameTAAGLTDC = GLFinalRenderPassComponent::get().m_TAAPingPassGLRPC->m_GLTDCs[0];
		l_lastFrameTAAGLTDC = GLFinalRenderPassComponent::get().m_TAAPongPassGLRPC->m_GLTDCs[0];

		l_FBC = GLFinalRenderPassComponent::get().m_TAAPingPassGLRPC->m_GLFBC;

		RenderingSystemComponent::get().m_isTAAPingPass = false;
	}
	else
	{
		l_currentFrameTAAGLTDC = GLFinalRenderPassComponent::get().m_TAAPongPassGLRPC->m_GLTDCs[0];
		l_lastFrameTAAGLTDC = GLFinalRenderPassComponent::get().m_TAAPingPassGLRPC->m_GLTDCs[0];

		l_FBC = GLFinalRenderPassComponent::get().m_TAAPongPassGLRPC->m_GLFBC;

		RenderingSystemComponent::get().m_isTAAPingPass = true;
	}

	bindFBC(l_FBC);

	activateTexture(inputGLTDC,
		0);
	activateTexture(
		l_lastFrameTAAGLTDC,
		1);
	activateTexture(
		GLGeometryRenderPassComponent::get().m_opaquePass_GLRPC->m_GLTDCs[3],
		2);

	auto l_MDC = g_pCoreSystem->getAssetSystem()->getMeshDataComponent(MeshShapeType::QUAD);
	drawMesh(l_MDC);

	return l_currentFrameTAAGLTDC;
}

GLTextureDataComponent* GLRenderingSystemNS::updateTAASharpenPass(GLTextureDataComponent * inputGLTDC)
{
	auto l_FBC = GLFinalRenderPassComponent::get().m_TAASharpenPassGLRPC->m_GLFBC;
	bindFBC(l_FBC);
	activateShaderProgram(GLFinalRenderPassComponent::get().m_TAASharpenPassGLSPC);

	activateTexture(
		inputGLTDC,
		0);

	auto l_MDC = g_pCoreSystem->getAssetSystem()->getMeshDataComponent(MeshShapeType::QUAD);
	drawMesh(l_MDC);

	return GLFinalRenderPassComponent::get().m_TAASharpenPassGLRPC->m_GLTDCs[0];
}

GLTextureDataComponent* GLRenderingSystemNS::updateBloomExtractPass(GLTextureDataComponent * inputGLTDC)
{
	auto l_FBC = GLFinalRenderPassComponent::get().m_bloomExtractPassGLRPC->m_GLFBC;
	bindFBC(l_FBC);

	activateShaderProgram(GLFinalRenderPassComponent::get().m_bloomExtractPassGLSPC);

	activateTexture(inputGLTDC, 0);

	auto l_MDC = g_pCoreSystem->getAssetSystem()->getMeshDataComponent(MeshShapeType::QUAD);
	drawMesh(l_MDC);

	return GLFinalRenderPassComponent::get().m_bloomExtractPassGLRPC->m_GLTDCs[0];
}

GLTextureDataComponent* GLRenderingSystemNS::updateBloomBlurPass(GLTextureDataComponent * inputGLTDC)
{
	GLTextureDataComponent* l_currentFrameBloomBlurGLTDC = GLFinalRenderPassComponent::get().m_bloomBlurPingPassGLRPC->m_GLTDCs[0];
	GLTextureDataComponent* l_lastFrameBloomBlurGLTDC = GLFinalRenderPassComponent::get().m_bloomBlurPongPassGLRPC->m_GLTDCs[0];

	activateShaderProgram(GLFinalRenderPassComponent::get().m_bloomBlurPassGLSPC);

	bool l_isPing = true;
	bool l_isFirstIteration = true;

	auto l_MDC = g_pCoreSystem->getAssetSystem()->getMeshDataComponent(MeshShapeType::QUAD);

	for (size_t i = 0; i < 5; i++)
	{
		if (l_isPing)
		{
			l_currentFrameBloomBlurGLTDC = GLFinalRenderPassComponent::get().m_bloomBlurPingPassGLRPC->m_GLTDCs[0];
			l_lastFrameBloomBlurGLTDC = GLFinalRenderPassComponent::get().m_bloomBlurPongPassGLRPC->m_GLTDCs[0];

			auto l_FBC = GLFinalRenderPassComponent::get().m_bloomBlurPingPassGLRPC->m_GLFBC;
			bindFBC(l_FBC);

			updateUniform(
				GLFinalRenderPassComponent::get().m_bloomBlurPass_uni_horizontal,
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
			l_currentFrameBloomBlurGLTDC = GLFinalRenderPassComponent::get().m_bloomBlurPongPassGLRPC->m_GLTDCs[0];
			l_lastFrameBloomBlurGLTDC = GLFinalRenderPassComponent::get().m_bloomBlurPingPassGLRPC->m_GLTDCs[0];

			auto l_FBC = GLFinalRenderPassComponent::get().m_bloomBlurPongPassGLRPC->m_GLFBC;
			bindFBC(l_FBC);

			updateUniform(
				GLFinalRenderPassComponent::get().m_bloomBlurPass_uni_horizontal,
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

GLTextureDataComponent* GLRenderingSystemNS::updateMotionBlurPass(GLTextureDataComponent * inputGLTDC)
{
	auto l_FBC = GLFinalRenderPassComponent::get().m_motionBlurPassGLRPC->m_GLFBC;
	bindFBC(l_FBC);

	activateShaderProgram(GLFinalRenderPassComponent::get().m_motionBlurPassGLSPC);

	activateTexture(
		GLGeometryRenderPassComponent::get().m_opaquePass_GLRPC->m_GLTDCs[3],
		0);
	activateTexture(inputGLTDC, 1);

	auto l_MDC = g_pCoreSystem->getAssetSystem()->getMeshDataComponent(MeshShapeType::QUAD);
	drawMesh(l_MDC);

	return GLFinalRenderPassComponent::get().m_motionBlurPassGLRPC->m_GLTDCs[0];
}

GLTextureDataComponent* GLRenderingSystemNS::updateBillboardPass()
{
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);

	auto l_FBC = GLFinalRenderPassComponent::get().m_billboardPassGLRPC->m_GLFBC;
	bindFBC(l_FBC);

	// copy depth buffer from G-Pass
	copyDepthBuffer(GLGeometryRenderPassComponent::get().m_opaquePass_GLRPC->m_GLFBC, l_FBC);

	activateShaderProgram(GLFinalRenderPassComponent::get().m_billboardPassGLSPC);

	updateUniform(
		GLFinalRenderPassComponent::get().m_billboardPass_uni_p,
		GLRenderingSystemComponent::get().m_CamProjOriginal);
	updateUniform(
		GLFinalRenderPassComponent::get().m_billboardPass_uni_r,
		GLRenderingSystemComponent::get().m_CamRot);
	updateUniform(
		GLFinalRenderPassComponent::get().m_billboardPass_uni_t,
		GLRenderingSystemComponent::get().m_CamTrans);

	if (GameSystemComponent::get().m_VisibleComponents.size() > 0)
	{
		// draw each visibleComponent
		for (auto& l_visibleComponent : GameSystemComponent::get().m_VisibleComponents)
		{
			if (l_visibleComponent->m_visiblilityType == VisiblilityType::INNO_BILLBOARD)
			{
				auto l_GlobalPos = g_pCoreSystem->getGameSystem()->get<TransformComponent>(l_visibleComponent->m_parentEntity)->m_globalTransformVector.m_pos;

				updateUniform(
					GLFinalRenderPassComponent::get().m_billboardPass_uni_pos,
					l_GlobalPos.x, l_GlobalPos.y, l_GlobalPos.z);

				auto l_distanceToCamera = (GLRenderingSystemComponent::get().m_CamGlobalPos - l_GlobalPos).length();
				if (l_distanceToCamera > 1.0f)
				{
					updateUniform(
						GLFinalRenderPassComponent::get().m_billboardPass_uni_size,
						(1.0f / l_distanceToCamera) * (9.0f / 16.0f), (1.0f / l_distanceToCamera));
				}
				else
				{
					updateUniform(
						GLFinalRenderPassComponent::get().m_billboardPass_uni_size,
						(9.0f / 16.0f), 1.0f);
				}
				// draw each graphic data of visibleComponent
				for (auto& l_modelPair : l_visibleComponent->m_modelMap)
				{
					// draw meshes
					auto l_MDC = l_modelPair.first;
					if (l_MDC)
					{
						auto l_textureMap = l_modelPair.second;
						// any normal?
						auto l_TDC = l_textureMap->m_texturePack.m_normalTDC.second;
						if (l_TDC)
						{
							activateTexture(l_TDC, 0);
						}
						else
						{
							activateTexture(GLRenderingSystemComponent::get().m_basicNormalGLTDC, 0);
						}
						drawMesh(l_MDC);
					}
				}
			}
		}
	}

	glDisable(GL_DEPTH_TEST);

	return GLFinalRenderPassComponent::get().m_billboardPassGLRPC->m_GLTDCs[0];
}

GLTextureDataComponent* GLRenderingSystemNS::updateDebuggerPass()
{
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);

	auto l_FBC = GLFinalRenderPassComponent::get().m_debuggerPassGLRPC->m_GLFBC;
	bindFBC(l_FBC);

	// copy depth buffer from G-Pass
	copyDepthBuffer(GLGeometryRenderPassComponent::get().m_opaquePass_GLRPC->m_GLFBC, l_FBC);

	activateShaderProgram(GLFinalRenderPassComponent::get().m_debuggerPassGLSPC);

	updateUniform(
		GLFinalRenderPassComponent::get().m_debuggerPass_uni_p,
		GLRenderingSystemComponent::get().m_CamProjOriginal);
	updateUniform(
		GLFinalRenderPassComponent::get().m_debuggerPass_uni_r,
		GLRenderingSystemComponent::get().m_CamRot);
	updateUniform(
		GLFinalRenderPassComponent::get().m_debuggerPass_uni_t,
		GLRenderingSystemComponent::get().m_CamTrans);

	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	for (auto& AABBWireframeDataPack : PhysicsSystemComponent::get().m_AABBWireframeDataPack)
	{
		updateUniform(
			GLFinalRenderPassComponent::get().m_debuggerPass_uni_m,
			AABBWireframeDataPack.m);
		drawMesh(AABBWireframeDataPack.MDC);
	}

	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glDisable(GL_DEPTH_TEST);

	return GLFinalRenderPassComponent::get().m_debuggerPassGLRPC->m_GLTDCs[0];
}

GLTextureDataComponent* GLRenderingSystemNS::updateFinalBlendPass(GLTextureDataComponent * inputGLTDC)
{
	auto l_FBC = GLFinalRenderPassComponent::get().m_finalBlendPassGLRPC->m_GLFBC;
	bindFBC(l_FBC);

	activateShaderProgram(GLFinalRenderPassComponent::get().m_finalBlendPassGLSPC);

	// last pass rendering target as the mixing background
	activateTexture(
		inputGLTDC,
		0);
	// bloom pass rendering target
	activateTexture(
		GLFinalRenderPassComponent::get().m_bloomBlurPongPassGLRPC->m_GLTDCs[0],
		1);
	// billboard pass rendering target
	activateTexture(
		GLFinalRenderPassComponent::get().m_billboardPassGLRPC->m_GLTDCs[0],
		2);
	// debugger pass rendering target
	activateTexture(
		GLFinalRenderPassComponent::get().m_debuggerPassGLRPC->m_GLTDCs[0],
		3);
	// draw final pass rectangle
	auto l_MDC = g_pCoreSystem->getAssetSystem()->getMeshDataComponent(MeshShapeType::QUAD);
	drawMesh(l_MDC);

	// draw again for game build
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);

	drawMesh(l_MDC);

	return GLFinalRenderPassComponent::get().m_finalBlendPassGLRPC->m_GLTDCs[0];
}

bool GLRenderingSystemNS::resizeFinalPass()
{
	resizeGLRenderPassComponent(GLFinalRenderPassComponent::get().m_skyPassGLRPC, GLRenderingSystemComponent::get().deferredPassFBDesc);
	resizeGLRenderPassComponent(GLFinalRenderPassComponent::get().m_preTAAPassGLRPC, GLRenderingSystemComponent::get().deferredPassFBDesc);
	resizeGLRenderPassComponent(GLFinalRenderPassComponent::get().m_TAAPingPassGLRPC, GLRenderingSystemComponent::get().deferredPassFBDesc);
	resizeGLRenderPassComponent(GLFinalRenderPassComponent::get().m_TAAPongPassGLRPC, GLRenderingSystemComponent::get().deferredPassFBDesc);
	resizeGLRenderPassComponent(GLFinalRenderPassComponent::get().m_TAASharpenPassGLRPC, GLRenderingSystemComponent::get().deferredPassFBDesc);
	resizeGLRenderPassComponent(GLFinalRenderPassComponent::get().m_bloomExtractPassGLRPC, GLRenderingSystemComponent::get().deferredPassFBDesc);
	resizeGLRenderPassComponent(GLFinalRenderPassComponent::get().m_bloomBlurPingPassGLRPC, GLRenderingSystemComponent::get().deferredPassFBDesc);
	resizeGLRenderPassComponent(GLFinalRenderPassComponent::get().m_bloomBlurPongPassGLRPC, GLRenderingSystemComponent::get().deferredPassFBDesc);
	resizeGLRenderPassComponent(GLFinalRenderPassComponent::get().m_motionBlurPassGLRPC, GLRenderingSystemComponent::get().deferredPassFBDesc);
	resizeGLRenderPassComponent(GLFinalRenderPassComponent::get().m_billboardPassGLRPC, GLRenderingSystemComponent::get().deferredPassFBDesc);
	resizeGLRenderPassComponent(GLFinalRenderPassComponent::get().m_debuggerPassGLRPC, GLRenderingSystemComponent::get().deferredPassFBDesc);
	resizeGLRenderPassComponent(GLFinalRenderPassComponent::get().m_finalBlendPassGLRPC, GLRenderingSystemComponent::get().deferredPassFBDesc);

	return true;
}

bool GLRenderingSystemNS::reloadFinalPassShaders()
{
	deleteShaderProgram(GLFinalRenderPassComponent::get().m_skyPassGLSPC);
	deleteShaderProgram(GLFinalRenderPassComponent::get().m_preTAAPassGLSPC);
	deleteShaderProgram(GLFinalRenderPassComponent::get().m_TAAPassGLSPC);
	deleteShaderProgram(GLFinalRenderPassComponent::get().m_TAASharpenPassGLSPC);
	deleteShaderProgram(GLFinalRenderPassComponent::get().m_bloomExtractPassGLSPC);
	deleteShaderProgram(GLFinalRenderPassComponent::get().m_bloomBlurPassGLSPC);
	deleteShaderProgram(GLFinalRenderPassComponent::get().m_motionBlurPassGLSPC);
	deleteShaderProgram(GLFinalRenderPassComponent::get().m_billboardPassGLSPC);
	deleteShaderProgram(GLFinalRenderPassComponent::get().m_debuggerPassGLSPC);
	deleteShaderProgram(GLFinalRenderPassComponent::get().m_finalBlendPassGLSPC);

	initializeGLShaderProgramComponent(GLFinalRenderPassComponent::get().m_skyPassGLSPC, GLFinalRenderPassComponent::get().m_skyPassShaderFilePaths);
	initializeGLShaderProgramComponent(GLFinalRenderPassComponent::get().m_preTAAPassGLSPC, GLFinalRenderPassComponent::get().m_preTAAPassShaderFilePaths);
	initializeGLShaderProgramComponent(GLFinalRenderPassComponent::get().m_TAAPassGLSPC, GLFinalRenderPassComponent::get().m_TAAPassShaderFilePaths);
	initializeGLShaderProgramComponent(GLFinalRenderPassComponent::get().m_TAASharpenPassGLSPC, GLFinalRenderPassComponent::get().m_TAASharpenPassShaderFilePaths);
	initializeGLShaderProgramComponent(GLFinalRenderPassComponent::get().m_bloomExtractPassGLSPC, GLFinalRenderPassComponent::get().m_bloomExtractPassShaderFilePaths);
	initializeGLShaderProgramComponent(GLFinalRenderPassComponent::get().m_bloomBlurPassGLSPC, GLFinalRenderPassComponent::get().m_bloomBlurPassShaderFilePaths);
	initializeGLShaderProgramComponent(GLFinalRenderPassComponent::get().m_motionBlurPassGLSPC, GLFinalRenderPassComponent::get().m_motionBlurPassShaderFilePaths);
	initializeGLShaderProgramComponent(GLFinalRenderPassComponent::get().m_billboardPassGLSPC, GLFinalRenderPassComponent::get().m_billboardPassShaderFilePaths);
	initializeGLShaderProgramComponent(GLFinalRenderPassComponent::get().m_debuggerPassGLSPC, GLFinalRenderPassComponent::get().m_billboardPassShaderFilePaths);
	initializeGLShaderProgramComponent(GLFinalRenderPassComponent::get().m_finalBlendPassGLSPC, GLFinalRenderPassComponent::get().m_finalBlendPassShaderFilePaths);

	bindSkyPassUniformLocations();
	bindTAAPassUniformLocations();
	bindBloomExtractPassUniformLocations();
	bindBloomBlurPassUniformLocations();
	bindMotionBlurUniformLocations();
	bindBillboardPassUniformLocations();
	bindDebuggerPassUniformLocations();
	bindFinalBlendPassUniformLocations();

	return true;
}