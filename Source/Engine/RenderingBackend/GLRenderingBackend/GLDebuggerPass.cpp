#include "GLDebuggerPass.h"
#include "../../Common/CommonMacro.inl"
#include "../../ComponentManager/ITransformComponentManager.h"
#include "../../ComponentManager/IVisibleComponentManager.h"
#include "../../ComponentManager/IDirectionalLightComponentManager.h"

#include "GLOpaquePass.h"
#include "GLSHPass.h"
#include "GLEnvironmentCapturePass.h"

#include "GLRenderingBackendUtilities.h"
#include "../../Component/GLRenderingBackendComponent.h"

using namespace GLRenderingBackendNS;

#include "../../ModuleManager/IModuleManager.h"

extern IModuleManager* g_pModuleManager;

namespace GLDebuggerPass
{
	void initializeShaders();
	bool initializeSHTest();

	bool drawCoordinateAxis();
	bool drawWireframeForMainCameraFrustum();
	bool drawWireframeForDebugObjects();

	bool drawMainView();
	bool drawRightView();
	bool drawTopView();
	bool drawFrontView();

	bool drawTestSceneForSH();
	bool drawProbes(GLRenderPassComponent* canvas);
	bool drawBricks();

	EntityID m_entityID;

	std::function<void()> f_mouseSelect;
	uint32_t m_pickedID;
	VisibleComponent* m_pickedVisibleComponent;

	GLTextureDataComponent* m_testSampleCubemap;
	SH9 m_testSH9;
	GLuint m_SH9UBO;

	GLRenderPassComponent* m_mainCanvasGLRPC;
	GLRenderPassComponent* m_rightViewGLRPC;
	GLRenderPassComponent* m_topViewGLRPC;
	GLRenderPassComponent* m_frontViewGLRPC;

	GLRenderPassComponent* m_SHVisualizationGLRPC;

	GLShaderProgramComponent* m_cubemapVisualizationGLSPC;
	ShaderFilePaths m_cubemapVisualizationPassShaderFilePaths = { "cubemapVisualizationPass.vert/", "", "", "", "cubemapVisualizationPass.frag/" };

	GLShaderProgramComponent* m_SHVisualizationGLSPC;
	ShaderFilePaths m_SHVisualizationPassShaderFilePaths = { "SHVisualizationPass.vert/", "", "", "", "SHVisualizationPass.frag/" };

	GLShaderProgramComponent* m_wireframeOverlayGLRPC;
	ShaderFilePaths m_shaderFilePaths = { "wireframeOverlayPass.vert/", "", "", "", "wireframeOverlayPass.frag/" };
	//ShaderFilePaths m_shaderFilePaths = { "debuggerPass.vert/", "", "", "debuggerPass.geom/", "debuggerPass.frag/" };
}

bool GLDebuggerPass::initializeSHTest()
{
	m_SH9UBO = generateUBO(sizeof(SH9) * 64, 9, "SH9UBO");

	const uint32_t m_captureResolution = 128;
	const uint32_t m_sampleCountPerFace = m_captureResolution * m_captureResolution;

	m_testSampleCubemap = addGLTextureDataComponent();
	m_testSampleCubemap->m_textureDataDesc = GLRenderingBackendComponent::get().m_deferredRenderPassDesc.RTDesc;
	m_testSampleCubemap->m_textureDataDesc.samplerType = TextureSamplerType::SAMPLER_CUBEMAP;
	m_testSampleCubemap->m_textureDataDesc.usageType = TextureUsageType::NORMAL;
	m_testSampleCubemap->m_textureDataDesc.pixelDataFormat = TexturePixelDataFormat::RGBA;
	m_testSampleCubemap->m_textureDataDesc.minFilterMethod = TextureFilterMethod::LINEAR;
	m_testSampleCubemap->m_textureDataDesc.magFilterMethod = TextureFilterMethod::LINEAR;
	m_testSampleCubemap->m_textureDataDesc.wrapMethod = TextureWrapMethod::REPEAT;
	m_testSampleCubemap->m_textureDataDesc.width = m_captureResolution;
	m_testSampleCubemap->m_textureDataDesc.height = m_captureResolution;
	m_testSampleCubemap->m_textureDataDesc.pixelDataType = TexturePixelDataType::FLOAT32;

	std::vector<vec4> l_textureSamples(m_captureResolution * m_captureResolution * 6);
	std::vector<vec4> l_faceColors = {
	vec4(1.0f, 0.0f, 0.0f, 1.0f),
	vec4(1.0f, 1.0f, 0.0f, 1.0f),
	vec4(0.0f, 1.0f, 0.0f, 1.0f),
	vec4(0.0f, 1.0f, 1.0f, 1.0f),
	vec4(0.0f, 0.0f, 1.0f, 1.0f),
	vec4(1.0f, 0.0f, 1.0f, 1.0f),
	};
	for (size_t i = 0; i < 6; i++)
	{
		for (size_t j = 0; j < m_sampleCountPerFace; j++)
		{
			auto l_color = l_faceColors[i] * 2.0f * (float)j / (float)m_sampleCountPerFace;
			l_color.w = 1.0f;
			l_textureSamples[i * m_sampleCountPerFace + j] = l_color;
		}
	}
	m_testSampleCubemap->m_textureData = &l_textureSamples[0];
	initializeGLTextureDataComponent(m_testSampleCubemap);

	m_testSH9 = GLSHPass::getSH9(m_testSampleCubemap);

	return true;
}

bool GLDebuggerPass::initialize()
{
	f_mouseSelect = [&]() {
		auto l_mousePos = g_pModuleManager->getEventSystem()->getMousePositionInScreenSpace();
		l_mousePos.y = g_pModuleManager->getRenderingFrontend()->getScreenResolution().y - l_mousePos.y;

		auto l_pixelValue = readPixel(GLOpaquePass::getGLRPC(), 3, (GLint)l_mousePos.x, (GLint)l_mousePos.y);

		m_pickedID = (uint32_t)l_pixelValue.z;

		auto l_visibleComponents = GetComponentManager(VisibleComponent)->GetAllComponents();
		auto l_findResult =
			std::find_if(l_visibleComponents.begin(), l_visibleComponents.end(), [&](auto& val) -> bool {
			return val->m_UUID == m_pickedID;
		}
		);
		if (l_findResult != l_visibleComponents.end())
		{
			m_pickedVisibleComponent = *l_findResult;
		}
	};

	g_pModuleManager->getEventSystem()->addButtonStateCallback(ButtonData{ INNO_MOUSE_BUTTON_LEFT, ButtonStatus::PRESSED }, &f_mouseSelect);

	m_entityID = InnoMath::createEntityID();

	auto l_renderPassDesc = GLRenderingBackendComponent::get().m_deferredRenderPassDesc;

	m_SHVisualizationGLRPC = addGLRenderPassComponent(m_entityID, "SHVisualizationPassGLRPC/");
	m_SHVisualizationGLRPC->m_renderPassDesc = l_renderPassDesc;
	m_SHVisualizationGLRPC->m_renderPassDesc.RTNumber = 1;
	m_SHVisualizationGLRPC->m_renderPassDesc.useDepthAttachment = true;
	m_SHVisualizationGLRPC->m_drawColorBuffers = true;

	initializeGLRenderPassComponent(m_SHVisualizationGLRPC);

	m_mainCanvasGLRPC = addGLRenderPassComponent(m_entityID, "DebugPassGLRPC/");
	m_mainCanvasGLRPC->m_renderPassDesc = l_renderPassDesc;
	m_mainCanvasGLRPC->m_renderPassDesc.useDepthAttachment = true;
	m_mainCanvasGLRPC->m_renderPassDesc.useStencilAttachment = true;
	initializeGLRenderPassComponent(m_mainCanvasGLRPC);

	l_renderPassDesc.RTDesc.width /= 2;
	l_renderPassDesc.RTDesc.height /= 2;

	m_rightViewGLRPC = addGLRenderPassComponent(m_entityID, "DebugRightViewPassGLRPC/");
	m_rightViewGLRPC->m_renderPassDesc = l_renderPassDesc;
	initializeGLRenderPassComponent(m_rightViewGLRPC);

	m_topViewGLRPC = addGLRenderPassComponent(m_entityID, "DebugTopViewPassGLRPC/");
	m_topViewGLRPC->m_renderPassDesc = l_renderPassDesc;
	initializeGLRenderPassComponent(m_topViewGLRPC);

	m_frontViewGLRPC = addGLRenderPassComponent(m_entityID, "DebugFrontViewPassGLRPC/");
	m_frontViewGLRPC->m_renderPassDesc = l_renderPassDesc;
	initializeGLRenderPassComponent(m_frontViewGLRPC);

	initializeShaders();
	initializeSHTest();

	return true;
}

void GLDebuggerPass::initializeShaders()
{
	auto rhs = addGLShaderProgramComponent(m_entityID);

	initializeGLShaderProgramComponent(rhs, m_shaderFilePaths);

	m_wireframeOverlayGLRPC = rhs;

	rhs = addGLShaderProgramComponent(m_entityID);

	initializeGLShaderProgramComponent(rhs, m_cubemapVisualizationPassShaderFilePaths);

	m_cubemapVisualizationGLSPC = rhs;

	rhs = addGLShaderProgramComponent(m_entityID);

	initializeGLShaderProgramComponent(rhs, m_SHVisualizationPassShaderFilePaths);

	m_SHVisualizationGLSPC = rhs;
}

bool GLDebuggerPass::drawCoordinateAxis()
{
	auto l_MDC = getGLMeshDataComponent(MeshShapeType::CUBE);

	auto l_p = g_pModuleManager->getRenderingFrontend()->getCameraGPUData().p_original;
	auto l_r = g_pModuleManager->getRenderingFrontend()->getCameraGPUData().r;

	auto l_pos = vec4(-70.0f, -60.0f, -1.0f, 1.0f);
	l_pos = InnoMath::clipToViewSpace(l_pos, l_p);
	l_pos.z = 0.0f;
	l_pos.w = 1.0f;

	auto l_tX = InnoMath::toTranslationMatrix(l_pos);
	auto l_tY = InnoMath::toTranslationMatrix(l_pos);
	auto l_tZ = InnoMath::toTranslationMatrix(l_pos);

	auto l_sX = InnoMath::toScaleMatrix(vec4(0.5f, 0.02f, 0.02f, 1.0f));
	auto l_sY = InnoMath::toScaleMatrix(vec4(0.02f, 0.5f, 0.02f, 1.0f));
	auto l_sZ = InnoMath::toScaleMatrix(vec4(0.02f, 0.02f, 0.5f, 1.0f));

	auto l_mX = l_tX * l_r *l_sX;
	auto l_mY = l_tY * l_r *l_sY;
	auto l_mZ = l_tZ * l_r *l_sZ;

	auto l_camPos = vec4(0.0f, 0.0f, 8.0f, 1.0f);

	auto l_cam_r = InnoMath::generateIdentityMatrix<float>();
	auto l_cam_t = InnoMath::toTranslationMatrix(l_camPos).inverse();

	updateUniform(0, l_p);
	updateUniform(1, l_cam_r);
	updateUniform(2, l_cam_t);

	updateUniform(3, l_mX);
	updateUniform(4, vec4(1.0f, 0.0f, 0.0f, 1.0f));
	drawMesh(l_MDC);

	updateUniform(3, l_mY);
	updateUniform(4, vec4(0.0f, 1.0f, 0.0f, 1.0f));
	drawMesh(l_MDC);

	updateUniform(3, l_mZ);
	updateUniform(4, vec4(0.0f, 0.0f, 1.0f, 1.0f));
	drawMesh(l_MDC);

	return true;
}

bool GLDebuggerPass::drawWireframeForMainCameraFrustum()
{
	auto l_MDC = getGLMeshDataComponent(MeshShapeType::SPHERE);

	auto l_r = InnoMath::generateIdentityMatrix<float>();
	auto l_pos = g_pModuleManager->getRenderingFrontend()->getCameraGPUData().globalPos;
	auto l_t = InnoMath::toTranslationMatrix(l_pos);
	auto l_s = InnoMath::toScaleMatrix(vec4(12.8f, 12.8f, 12.8f, 1.0f));
	auto l_m = l_r * l_t * l_s;

	updateUniform(3, l_m);
	// albedo
	updateUniform(4, vec4(0.6f, 0.6f, 0.1f, 1.0f));

	drawMesh(l_MDC);

	return true;
}

bool GLDebuggerPass::drawWireframeForDebugObjects()
{
	auto l_MDC = getGLMeshDataComponent(MeshShapeType::SPHERE);

	static bool l_drawPointLightRange = false;

	if (l_drawPointLightRange)
	{
		for (auto i : g_pModuleManager->getRenderingFrontend()->getPointLightGPUData())
		{
			if (i.luminance.w > 0.0f)
			{
				auto l_t = InnoMath::toTranslationMatrix(i.pos);
				auto l_s = InnoMath::toScaleMatrix(vec4(i.luminance.w, i.luminance.w, i.luminance.w, 1.0f));
				auto l_m = l_t * l_s;
				updateUniform(3, l_m);
				drawMesh(l_MDC);
			}
		}
	}

	static bool l_drawSphereLightShape = false;

	if (l_drawSphereLightShape)
	{
		for (auto i : g_pModuleManager->getRenderingFrontend()->getSphereLightGPUData())
		{
			if (i.luminance.w > 0.0f)
			{
				auto l_t = InnoMath::toTranslationMatrix(i.pos);
				auto l_s = InnoMath::toScaleMatrix(vec4(i.luminance.w, i.luminance.w, i.luminance.w, 1.0f));
				auto l_m = l_t * l_s;
				updateUniform(3, l_m);
				drawMesh(l_MDC);
			}
		}
	}

	static bool l_drawCSMAABBRange = false;
	l_MDC = getGLMeshDataComponent(MeshShapeType::CUBE);

	if (l_drawCSMAABBRange)
	{
		auto l_SplitAABB = GetComponentManager(DirectionalLightComponent)->GetSplitAABB();

		for (auto i : l_SplitAABB)
		{
			auto l_t = InnoMath::toTranslationMatrix(i.m_center);
			auto l_extend = i.m_extend;
			auto l_s = InnoMath::toScaleMatrix(l_extend);
			auto l_m = l_t * l_s;
			updateUniform(3, l_m);
			drawMesh(l_MDC);
		}
	}

	static bool l_drawSkeleton = true;
	if (l_drawSkeleton)
	{
		auto l_visibleComponents = GetComponentManager(VisibleComponent)->GetAllComponents();

		for (auto i : l_visibleComponents)
		{
			if (i->m_meshUsageType == MeshUsageType::SKELETAL)
			{
				auto l_transformComponent = GetComponent(TransformComponent, i->m_parentEntity);
				auto l_m = l_transformComponent->m_globalTransformMatrix.m_transformationMat;
				for (auto j : i->m_modelMap)
				{
					auto l_SDC = j.first->m_SDC;
					auto l_rootOffsetMat = l_SDC->m_RootOffsetMatrix;

					for (auto k : l_SDC->m_Bones)
					{
						auto l_t = InnoMath::toTranslationMatrix(k.m_Pos);
						auto l_r = InnoMath::toRotationMatrix(k.m_Rot);
						auto l_bm = l_t * l_r;
						// Inverse-Joint-Matrix
						l_bm = l_bm.inverse();
						auto l_s = InnoMath::toScaleMatrix(vec4(0.01f, 0.01f, 0.01f, 1.0f));
						l_bm = l_bm * l_s;
						l_bm = l_m * l_bm;
						updateUniform(3, l_bm);
						drawMesh(reinterpret_cast<GLMeshDataComponent*>(l_MDC));
					}
				}
			}
		}
	}

	static bool l_drawDebugMesh = false;

	if (l_drawDebugMesh)
	{
		for (uint32_t i = 0; i < g_pModuleManager->getRenderingFrontend()->getDebuggerPassDrawCallCount(); i++)
		{
			auto l_debuggerPassGPUData = g_pModuleManager->getRenderingFrontend()->getDebuggerPassGPUData()[i];
			updateUniform(3, l_debuggerPassGPUData.m);
			drawMesh(reinterpret_cast<GLMeshDataComponent*>(l_debuggerPassGPUData.mesh));
		}
	}

	if (m_pickedVisibleComponent)
	{
		auto l_transformComponent = GetComponent(TransformComponent, m_pickedVisibleComponent->m_parentEntity);

		for (auto i : m_pickedVisibleComponent->m_modelMap)
		{
			updateUniform(3, l_transformComponent->m_globalTransformMatrix.m_transformationMat);
			drawMesh(reinterpret_cast<GLMeshDataComponent*>(i.first));
		}
	}

	return true;
}

bool GLDebuggerPass::drawMainView()
{
	updateUniform(0, g_pModuleManager->getRenderingFrontend()->getCameraGPUData().p_original);
	updateUniform(1, g_pModuleManager->getRenderingFrontend()->getCameraGPUData().r);
	updateUniform(2, g_pModuleManager->getRenderingFrontend()->getCameraGPUData().t);

	// albedo
	updateUniform(4, vec4(0.5f, 0.2f, 0.1f, 1.0f));

	drawWireframeForDebugObjects();

	return true;
}

bool GLDebuggerPass::drawRightView()
{
	auto l_p = g_pModuleManager->getRenderingFrontend()->getCameraGPUData().p_original;
	auto l_cam_r = InnoMath::toRotationMatrix(InnoMath::getQuatRotator(vec4(0.0f, 1.0f, 0.0f, 0.0f), 90.0f)).inverse();
	auto l_pos = vec4(1024.0f, 0.0f, 0.0f, 1.0f);
	auto l_cam_t = InnoMath::toTranslationMatrix(l_pos).inverse();

	updateUniform(0, l_p);
	updateUniform(1, l_cam_r);
	updateUniform(2, l_cam_t);

	// albedo
	updateUniform(4, vec4(1.0f, 1.0f, 1.0f, 1.0f));

	bindRenderPass(m_rightViewGLRPC);
	cleanRenderBuffers(m_rightViewGLRPC);

	drawWireframeForDebugObjects();

	drawWireframeForMainCameraFrustum();

	return true;
}

bool GLDebuggerPass::drawTopView()
{
	auto l_p = g_pModuleManager->getRenderingFrontend()->getCameraGPUData().p_original;
	auto l_qX = InnoMath::getQuatRotator(vec4(1.0f, 0.0f, 0.0f, 0.0f), -90.0f);
	auto l_qY = InnoMath::getQuatRotator(vec4(0.0f, 1.0f, 0.0f, 0.0f), 0.0f);
	auto l_cam_r = InnoMath::toRotationMatrix(l_qX.quatMul(l_qY)).inverse();
	auto l_pos = vec4(0.0f, 1024.0f, 0.0f, 1.0f);
	auto l_cam_t = InnoMath::toTranslationMatrix(l_pos).inverse();

	updateUniform(0, l_p);
	updateUniform(1, l_cam_r);
	updateUniform(2, l_cam_t);

	// albedo
	updateUniform(4, vec4(1.0f, 1.0f, 1.0f, 1.0f));

	bindRenderPass(m_topViewGLRPC);
	cleanRenderBuffers(m_topViewGLRPC);

	drawWireframeForDebugObjects();

	drawWireframeForMainCameraFrustum();

	return true;
}

bool GLDebuggerPass::drawFrontView()
{
	auto l_p = g_pModuleManager->getRenderingFrontend()->getCameraGPUData().p_original;
	auto l_cam_r = InnoMath::toRotationMatrix(InnoMath::getQuatRotator(vec4(1.0f, 0.0f, 0.0f, 0.0f), 0.0f)).inverse();
	auto l_pos = vec4(0.0f, 0.0f, 1024.0f, 1.0f);
	auto l_cam_t = InnoMath::toTranslationMatrix(l_pos).inverse();

	updateUniform(0, l_p);
	updateUniform(1, l_cam_r);
	updateUniform(2, l_cam_t);

	// albedo
	updateUniform(4, vec4(1.0f, 1.0f, 1.0f, 1.0f));

	bindRenderPass(m_frontViewGLRPC);
	cleanRenderBuffers(m_frontViewGLRPC);

	drawWireframeForDebugObjects();

	drawWireframeForMainCameraFrustum();

	return true;
}

bool GLDebuggerPass::drawTestSceneForSH()
{
	glDepthMask(GL_TRUE);
	glDepthFunc(GL_LEQUAL);

	auto l_m = InnoMath::generateIdentityMatrix<float>();
	updateUniform(0, l_m);

	updateUBO(m_SH9UBO, m_testSH9);

	bindRenderPass(m_SHVisualizationGLRPC);
	cleanRenderBuffers(m_SHVisualizationGLRPC);

	activateShaderProgram(m_cubemapVisualizationGLSPC);

	activateTexture(m_testSampleCubemap, 0);

	auto l_MDC = getGLMeshDataComponent(MeshShapeType::CUBE);
	drawMesh(l_MDC);

	activateShaderProgram(m_SHVisualizationGLSPC);

	l_MDC = getGLMeshDataComponent(MeshShapeType::SPHERE);
	drawMesh(l_MDC);

	return true;
}

bool GLDebuggerPass::drawProbes(GLRenderPassComponent* canvas)
{
	glDepthMask(GL_FALSE);
	glDepthFunc(GL_LESS);

	// copy depth buffer from G-Pass
	copyDepthBuffer(GLOpaquePass::getGLRPC(), canvas);

	static bool l_drawSkyVisibilitySH9 = false;

	auto l_probes = GLEnvironmentCapturePass::getProbes();
	std::vector<SH9> l_SH9s(l_probes.size());

	if (l_drawSkyVisibilitySH9)
	{
		for (size_t i = 0; i < l_probes.size(); i++)
		{
			l_SH9s[i] = l_probes[i].skyVisibility;
		}
	}
	else
	{
		for (size_t i = 0; i < l_probes.size(); i++)
		{
			l_SH9s[i] = l_probes[i].radiance;
		}
	}

	auto l_MDC = getGLMeshDataComponent(MeshShapeType::SPHERE);

	activateShaderProgram(m_SHVisualizationGLSPC);

	updateUBO(m_SH9UBO, l_SH9s);

	for (size_t i = 0; i < l_probes.size(); i++)
	{
		auto l_m = InnoMath::toTranslationMatrix(l_probes[i].pos);
		updateUniform(0, l_m);
		updateUniform(1, (uint32_t)i);

		drawMesh(l_MDC);
	}

	return true;
}

bool GLDebuggerPass::drawBricks()
{
	auto l_bricks = GLEnvironmentCapturePass::getBricks();

	auto l_MDC = getGLMeshDataComponent(MeshShapeType::CUBE);

	activateShaderProgram(m_wireframeOverlayGLRPC);

	updateUniform(0, g_pModuleManager->getRenderingFrontend()->getCameraGPUData().p_original);
	updateUniform(1, g_pModuleManager->getRenderingFrontend()->getCameraGPUData().r);
	updateUniform(2, g_pModuleManager->getRenderingFrontend()->getCameraGPUData().t);

	// albedo
	updateUniform(4, vec4(0.1f, 0.3f, 0.6f, 1.0f));

	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	for (size_t i = 0; i < l_bricks.size(); i++)
	{
		auto l_t = InnoMath::toTranslationMatrix(l_bricks[i].boundBox.m_center);
		auto l_scale = l_bricks[i].boundBox.m_extend / 2.0f;
		l_scale.w = 1.0f;
		auto l_s = InnoMath::toScaleMatrix(l_scale);
		auto l_m = l_t * l_s;
		updateUniform(3, l_m);

		drawMesh(l_MDC);
	}

	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	return true;
}

bool GLDebuggerPass::update(GLRenderPassComponent* canvas)
{
	bindRenderPass(canvas);

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_DEPTH_CLAMP);

	static bool l_drawSHTestScene = false;

	if (l_drawSHTestScene)
	{
		drawTestSceneForSH();
	}
	else
	{
		drawProbes(canvas);
	}

	static bool l_drawBricks = true;

	if (l_drawBricks)
	{
		drawBricks();
	}

	glDisable(GL_DEPTH_CLAMP);
	glDisable(GL_DEPTH_TEST);

	bindRenderPass(m_mainCanvasGLRPC);
	cleanRenderBuffers(m_mainCanvasGLRPC);

	activateShaderProgram(m_wireframeOverlayGLRPC);

	drawCoordinateAxis();

	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	drawMainView();
	drawRightView();
	drawTopView();
	drawFrontView();

	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	return true;
}

bool GLDebuggerPass::resize(uint32_t newSizeX, uint32_t newSizeY)
{
	resizeGLRenderPassComponent(m_mainCanvasGLRPC, newSizeX, newSizeY);

	return true;
}

bool GLDebuggerPass::reloadShader()
{
	deleteShaderProgram(m_wireframeOverlayGLRPC);

	initializeGLShaderProgramComponent(m_wireframeOverlayGLRPC, m_shaderFilePaths);

	deleteShaderProgram(m_cubemapVisualizationGLSPC);

	initializeGLShaderProgramComponent(m_cubemapVisualizationGLSPC, m_cubemapVisualizationPassShaderFilePaths);

	deleteShaderProgram(m_SHVisualizationGLSPC);

	initializeGLShaderProgramComponent(m_SHVisualizationGLSPC, m_SHVisualizationPassShaderFilePaths);

	return true;
}

GLRenderPassComponent * GLDebuggerPass::getGLRPC(uint32_t index)
{
	if (index == 0)
	{
		return m_mainCanvasGLRPC;
	}
	else if (index == 1)
	{
		return m_rightViewGLRPC;
	}
	else if (index == 2)
	{
		return m_topViewGLRPC;
	}
	else if (index == 3)
	{
		return m_frontViewGLRPC;
	}
	else
	{
		return m_SHVisualizationGLRPC;
	}
}