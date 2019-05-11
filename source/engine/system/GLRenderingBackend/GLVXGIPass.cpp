#include "GLVXGIPass.h"

#include "GLRenderingSystemUtilities.h"

#include "../../component/GLRenderingSystemComponent.h"
#include "../../component/RenderingFrontendSystemComponent.h"

#include "../ICoreSystem.h"

extern ICoreSystem* g_pCoreSystem;

using namespace GLRenderingSystemNS;

INNO_PRIVATE_SCOPE GLVXGIPass
{
	void initializeVoxelizationPass();
	void initializeIrradianceInjectionPass();
	void initializeVoxelVisualizationPass();

	void updateVoxelizationPass();
	void updateIrradianceInjectionPass();
	void updateVoxelVisualizationPass();

	EntityID m_entityID;

	GLRenderPassComponent* m_voxelizationPassGLRPC;
	GLShaderProgramComponent* m_voxelizationPassSPC;
	GLuint m_voxelizationPass_uni_m;
	std::vector<GLuint> m_voxelizationPass_uni_VP;
	std::vector<GLuint> m_voxelizationPass_uni_VP_inv;
	GLuint m_voxelizationPass_uni_volumeDimension;
	GLuint m_voxelizationPass_uni_voxelScale;
	GLuint m_voxelizationPass_uni_worldMinPoint;

	std::vector<mat4> m_VP;
	std::vector<mat4> m_VP_inv;
	unsigned int m_volumeDimension = 128;
	unsigned int m_voxelCount = m_volumeDimension * m_volumeDimension * m_volumeDimension;
	float m_volumeGridSize;

	GLRenderPassComponent* m_irradianceInjectionPassGLRPC;
	GLShaderProgramComponent* m_irradianceInjectionPassSPC;
	GLuint m_irradianceInjectionPass_uni_dirLight_direction;
	GLuint m_irradianceInjectionPass_uni_dirLight_luminance;
	GLuint m_irradianceInjectionPass_uni_volumeDimension;
	GLuint m_irradianceInjectionPass_uni_voxelSize;
	GLuint m_irradianceInjectionPass_uni_voxelScale;
	GLuint m_irradianceInjectionPass_uni_worldMinPoint;

	GLRenderPassComponent* m_voxelVisualizationGLRPC;
	GLShaderProgramComponent* m_voxelVisualizationPassSPC;
	GLuint m_voxelVisualizationPass_uni_p;
	GLuint m_voxelVisualizationPass_uni_r;
	GLuint m_voxelVisualizationPass_uni_t;
	GLuint m_voxelVisualizationPass_uni_m;
	GLuint m_voxelVisualizationPass_uni_volumeDimension;
	GLuint m_voxelVisualizationPass_uni_voxelSize;
	GLuint m_voxelVisualizationPass_uni_worldMinPoint;

	GLuint m_VAO;
}

void GLVXGIPass::initialize()
{
	m_entityID = InnoMath::createEntityID();

	initializeVoxelizationPass();

	initializeIrradianceInjectionPass();

	initializeVoxelVisualizationPass();
}

void GLVXGIPass::initializeVoxelizationPass()
{
	auto l_renderPassDesc = GLRenderingSystemComponent::get().m_deferredRenderPassDesc;

	l_renderPassDesc.RTNumber = 2;
	l_renderPassDesc.RTDesc.samplerType = TextureSamplerType::SAMPLER_3D;
	l_renderPassDesc.RTDesc.usageType = TextureUsageType::COLOR_ATTACHMENT;
	l_renderPassDesc.RTDesc.pixelDataFormat = TexturePixelDataFormat::RGBA;
	l_renderPassDesc.RTDesc.minFilterMethod = TextureFilterMethod::NEAREST;
	l_renderPassDesc.RTDesc.magFilterMethod = TextureFilterMethod::NEAREST;
	l_renderPassDesc.RTDesc.wrapMethod = TextureWrapMethod::REPEAT;
	l_renderPassDesc.RTDesc.width = m_volumeDimension;
	l_renderPassDesc.RTDesc.height = m_volumeDimension;
	l_renderPassDesc.RTDesc.depth = m_volumeDimension;
	l_renderPassDesc.RTDesc.pixelDataType = TexturePixelDataType::UBYTE;

	m_voxelizationPassGLRPC = addGLRenderPassComponent(m_entityID, "VoxelizationPassGLRPC/");
	m_voxelizationPassGLRPC->m_renderPassDesc = l_renderPassDesc;
	m_voxelizationPassGLRPC->m_renderPassDesc.useDepthAttachment = true;
	m_voxelizationPassGLRPC->m_drawColorBuffers = false;
	initializeGLRenderPassComponent(m_voxelizationPassGLRPC);

	// shader programs and shaders
	ShaderFilePaths m_voxelizationPassShaderFilePaths = {};

	////
	m_voxelizationPassShaderFilePaths.m_VSPath = "GL//GIVoxelizationPass.vert/";
	m_voxelizationPassShaderFilePaths.m_GSPath = "GL//GIVoxelizationPass.geom/";
	m_voxelizationPassShaderFilePaths.m_FSPath = "GL//GIVoxelizationPass.frag/";

	auto rhs = addGLShaderProgramComponent(m_entityID);
	initializeGLShaderProgramComponent(rhs, m_voxelizationPassShaderFilePaths);

	m_voxelizationPass_uni_m = getUniformLocation(
		rhs->m_program,
		"uni_m");

	m_voxelizationPass_uni_VP.reserve(3);
	m_voxelizationPass_uni_VP_inv.reserve(3);

	for (size_t i = 0; i < 3; i++)
	{
		m_voxelizationPass_uni_VP.emplace_back(
			getUniformLocation(rhs->m_program, "uni_VP[" + std::to_string(i) + "]")
		);
		m_voxelizationPass_uni_VP_inv.emplace_back(
			getUniformLocation(rhs->m_program, "uni_VP_inv[" + std::to_string(i) + "]")
		);
	}

	m_voxelizationPass_uni_volumeDimension = getUniformLocation(
		rhs->m_program,
		"uni_volumeDimension");
	m_voxelizationPass_uni_voxelScale = getUniformLocation(
		rhs->m_program,
		"uni_voxelScale");
	m_voxelizationPass_uni_worldMinPoint = getUniformLocation(
		rhs->m_program,
		"uni_worldMinPoint");

	m_voxelizationPassSPC = rhs;

	m_VP.reserve(3);
	m_VP_inv.reserve(3);

	for (size_t i = 0; i < 3; i++)
	{
		m_VP.emplace_back();
		m_VP_inv.emplace_back();
	}
}

void GLVXGIPass::initializeIrradianceInjectionPass()
{
	auto l_renderPassDesc = GLRenderingSystemComponent::get().m_deferredRenderPassDesc;

	l_renderPassDesc.RTDesc.samplerType = TextureSamplerType::SAMPLER_3D;
	l_renderPassDesc.RTDesc.usageType = TextureUsageType::COLOR_ATTACHMENT;
	l_renderPassDesc.RTDesc.pixelDataFormat = TexturePixelDataFormat::RGBA;
	l_renderPassDesc.RTDesc.minFilterMethod = TextureFilterMethod::NEAREST;
	l_renderPassDesc.RTDesc.magFilterMethod = TextureFilterMethod::NEAREST;
	l_renderPassDesc.RTDesc.wrapMethod = TextureWrapMethod::REPEAT;
	l_renderPassDesc.RTDesc.width = m_volumeDimension;
	l_renderPassDesc.RTDesc.height = m_volumeDimension;
	l_renderPassDesc.RTDesc.depth = m_volumeDimension;
	l_renderPassDesc.RTDesc.pixelDataType = TexturePixelDataType::UBYTE;
	l_renderPassDesc.useDepthAttachment = true;

	m_irradianceInjectionPassGLRPC = addGLRenderPassComponent(m_entityID, "IrradianceInjectionPassGLRPC/");
	m_irradianceInjectionPassGLRPC->m_renderPassDesc = l_renderPassDesc;
	m_irradianceInjectionPassGLRPC->m_drawColorBuffers = false;
	initializeGLRenderPassComponent(m_irradianceInjectionPassGLRPC);

	ShaderFilePaths m_irradianceInjectionPassShaderFilePaths = {};

	////
	m_irradianceInjectionPassShaderFilePaths.m_CSPath = "GL//GIIrradianceInjectionPass.comp/";

	auto rhs = addGLShaderProgramComponent(m_entityID);
	initializeGLShaderProgramComponent(rhs, m_irradianceInjectionPassShaderFilePaths);

	m_irradianceInjectionPass_uni_dirLight_direction = getUniformLocation(
		rhs->m_program,
		"uni_dirLight.direction");
	m_irradianceInjectionPass_uni_dirLight_luminance = getUniformLocation(
		rhs->m_program,
		"uni_dirLight.luminance");
	m_irradianceInjectionPass_uni_volumeDimension = getUniformLocation(
		rhs->m_program,
		"uni_volumeDimension");
	m_irradianceInjectionPass_uni_voxelSize = getUniformLocation(
		rhs->m_program,
		"uni_voxelSize");
	m_irradianceInjectionPass_uni_voxelScale = getUniformLocation(
		rhs->m_program,
		"uni_voxelScale");
	m_irradianceInjectionPass_uni_worldMinPoint = getUniformLocation(
		rhs->m_program,
		"uni_worldMinPoint");

	m_irradianceInjectionPassSPC = rhs;
}

void GLVXGIPass::initializeVoxelVisualizationPass()
{
	// generate and bind framebuffer
	m_voxelVisualizationGLRPC = addGLRenderPassComponent(m_entityID, "VoxelVisualizationPassGLRPC/");
	m_voxelVisualizationGLRPC->m_renderPassDesc = GLRenderingSystemComponent::get().m_deferredRenderPassDesc;
	initializeGLRenderPassComponent(m_voxelVisualizationGLRPC);

	ShaderFilePaths m_voxelVisualizationPassShaderFilePaths = {};

	////
	m_voxelVisualizationPassShaderFilePaths.m_VSPath = "GL//GIVoxelVisualizationPass.vert/";
	m_voxelVisualizationPassShaderFilePaths.m_GSPath = "GL//GIVoxelVisualizationPass.geom/";
	m_voxelVisualizationPassShaderFilePaths.m_FSPath = "GL//GIVoxelVisualizationPass.frag/";

	auto rhs = addGLShaderProgramComponent(m_entityID);
	initializeGLShaderProgramComponent(rhs, m_voxelVisualizationPassShaderFilePaths);

	m_voxelVisualizationPass_uni_p = getUniformLocation(
		rhs->m_program,
		"uni_p");
	m_voxelVisualizationPass_uni_r = getUniformLocation(
		rhs->m_program,
		"uni_r");
	m_voxelVisualizationPass_uni_t = getUniformLocation(
		rhs->m_program,
		"uni_t");
	m_voxelVisualizationPass_uni_m = getUniformLocation(
		rhs->m_program,
		"uni_m");

	m_voxelVisualizationPass_uni_volumeDimension = getUniformLocation(
		rhs->m_program,
		"uni_volumeDimension");
	m_voxelVisualizationPass_uni_voxelSize = getUniformLocation(
		rhs->m_program,
		"uni_voxelSize");
	m_voxelVisualizationPass_uni_worldMinPoint = getUniformLocation(
		rhs->m_program,
		"uni_worldMinPoint");

	m_voxelVisualizationPassSPC = rhs;

	glGenVertexArrays(1, &m_VAO);
}

void GLVXGIPass::update()
{
	updateVoxelizationPass();
	updateIrradianceInjectionPass();
}

void GLVXGIPass::draw()
{
	updateVoxelVisualizationPass();
}

void GLVXGIPass::updateVoxelizationPass()
{
	auto l_sceneAABB = g_pCoreSystem->getPhysicsSystem()->getSceneAABB();

	auto axisSize = l_sceneAABB.m_extend;
	auto center = l_sceneAABB.m_center;
	m_volumeGridSize = std::max(axisSize.x, std::max(axisSize.y, axisSize.z));
	auto l_voxelSize = m_volumeGridSize / m_volumeDimension;
	auto l_halfSize = m_volumeGridSize / 2.0f;

	// projection matrices
	auto l_p = InnoMath::generateOrthographicMatrix(-l_halfSize, l_halfSize, -l_halfSize, l_halfSize, 0.0f, m_volumeGridSize);

	// view matrices
	m_VP[0] = InnoMath::lookAt(center + vec4(l_halfSize, 0.0f, 0.0f, 0.0f),
		center, vec4(0.0f, 1.0f, 0.0f, 0.0f));
	m_VP[1] = InnoMath::lookAt(center + vec4(0.0f, l_halfSize, 0.0f, 0.0f),
		center, vec4(0.0f, 0.0f, -1.0f, 0.0f));
	m_VP[2] = InnoMath::lookAt(center + vec4(0.0f, 0.0f, l_halfSize, 0.0f),
		center, vec4(0.0f, 1.0f, 0.0f, 0.0f));

	for (size_t i = 0; i < 3; i++)
	{
		m_VP[i] = m_VP[i] * l_p;
		m_VP_inv[i] = m_VP[i].inverse();
	}

	activateRenderPass(m_voxelizationPassGLRPC);

	// disable status
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);

	glDepthMask(false);
	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// voxelization pass
	glBindImageTexture(0, m_voxelizationPassGLRPC->m_GLTDCs[0]->m_TO, 0, GL_TRUE, 0, GL_READ_WRITE, GL_RG32UI);
	glBindImageTexture(1, m_voxelizationPassGLRPC->m_GLTDCs[1]->m_TO, 0, GL_TRUE, 0, GL_READ_WRITE, GL_RG32UI);

	activateShaderProgram(m_voxelizationPassSPC);

	updateUniform(m_voxelizationPass_uni_volumeDimension, m_volumeDimension);
	updateUniform(m_voxelizationPass_uni_voxelScale, 1.0f / m_volumeGridSize);
	updateUniform(m_voxelizationPass_uni_worldMinPoint, l_sceneAABB.m_boundMin);

	for (size_t i = 0; i < m_VP.size(); i++)
	{
		updateUniform(m_voxelizationPass_uni_VP[i], m_VP[i]);
		updateUniform(m_voxelizationPass_uni_VP_inv[i], m_VP_inv[i]);
	}

	for (auto& l_visibleComponent : g_pCoreSystem->getGameSystem()->get<VisibleComponent>())
	{
		if (l_visibleComponent->m_visiblilityType == VisiblilityType::INNO_OPAQUE && l_visibleComponent->m_objectStatus == ObjectStatus::ALIVE)
		{
			updateUniform(
				m_voxelizationPass_uni_m,
				g_pCoreSystem->getGameSystem()->get<TransformComponent>(l_visibleComponent->m_parentEntity)->m_globalTransformMatrix.m_transformationMat);

			// draw each graphic data of visibleComponent
			for (auto& l_modelPair : l_visibleComponent->m_modelMap)
			{
				// draw meshes
				auto l_MDC = l_modelPair.first;

				if (l_MDC)
				{
					// draw meshes
					drawMesh(reinterpret_cast<GLMeshDataComponent*>(l_MDC));
				}
			}
		}
	}

	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);
	// reset status
	glDepthMask(true);
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
}

void GLVXGIPass::updateIrradianceInjectionPass()
{
	glDepthMask(false);
	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
	auto l_sceneAABB = g_pCoreSystem->getPhysicsSystem()->getSceneAABB();

	auto axisSize = l_sceneAABB.m_extend;
	m_volumeGridSize = std::max(axisSize.x, std::max(axisSize.y, axisSize.z));
	auto l_voxelSize = m_volumeGridSize / m_volumeDimension;

	activateRenderPass(m_irradianceInjectionPassGLRPC);

	activateTexture(m_voxelizationPassGLRPC->m_GLTDCs[0], 0);
	activateTexture(m_voxelizationPassGLRPC->m_GLTDCs[1], 1);
	glBindImageTexture(2, m_irradianceInjectionPassGLRPC->m_GLTDCs[0]->m_TO, 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA8);

	activateShaderProgram(m_irradianceInjectionPassSPC);

	updateUniform(
		m_irradianceInjectionPass_uni_dirLight_direction,
		RenderingFrontendSystemComponent::get().m_sunGPUData.dir);
	updateUniform(
		m_irradianceInjectionPass_uni_dirLight_luminance,
		RenderingFrontendSystemComponent::get().m_sunGPUData.luminance);

	updateUniform(m_irradianceInjectionPass_uni_volumeDimension, m_volumeDimension);
	updateUniform(m_irradianceInjectionPass_uni_voxelSize, l_voxelSize);
	updateUniform(m_irradianceInjectionPass_uni_voxelScale, 1.0f / m_volumeGridSize);
	updateUniform(m_irradianceInjectionPass_uni_worldMinPoint, l_sceneAABB.m_boundMin);

	auto l_workGroups = static_cast<unsigned>(std::ceil(m_volumeDimension / 8.0f));
	glDispatchCompute(l_workGroups, l_workGroups, l_workGroups);

	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);

	// reset status
	glDepthMask(true);
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
}

void GLVXGIPass::updateVoxelVisualizationPass()
{
	glDepthMask(true);
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

	auto l_sceneAABB = g_pCoreSystem->getPhysicsSystem()->getSceneAABB();

	auto axisSize = l_sceneAABB.m_extend;
	m_volumeGridSize = std::max(axisSize.x, std::max(axisSize.y, axisSize.z));
	auto l_voxelSize = m_volumeGridSize / m_volumeDimension;

	// voxel visualization pass
	auto l_ms = InnoMath::toScaleMatrix(vec4(l_voxelSize, l_voxelSize, l_voxelSize, 1.0f));
	auto l_mt = InnoMath::toTranslationMatrix(l_sceneAABB.m_boundMin);

	auto l_m = l_mt * l_ms;

	activateRenderPass(m_voxelVisualizationGLRPC);

	activateShaderProgram(m_voxelVisualizationPassSPC);

	updateUniform(m_voxelVisualizationPass_uni_p, RenderingFrontendSystemComponent::get().m_cameraGPUData.p_original);
	updateUniform(m_voxelVisualizationPass_uni_r, RenderingFrontendSystemComponent::get().m_cameraGPUData.r);
	updateUniform(m_voxelVisualizationPass_uni_t, RenderingFrontendSystemComponent::get().m_cameraGPUData.t);
	updateUniform(m_voxelVisualizationPass_uni_m, l_m);

	updateUniform(m_voxelVisualizationPass_uni_volumeDimension, m_volumeDimension);
	updateUniform(m_voxelVisualizationPass_uni_voxelSize, l_voxelSize);
	updateUniform(m_voxelVisualizationPass_uni_worldMinPoint, l_sceneAABB.m_boundMin);

	glBindImageTexture(3, m_irradianceInjectionPassGLRPC->m_GLTDCs[0]->m_TO, 0, GL_TRUE, 0, GL_READ_ONLY, GL_RGBA8);

	glBindVertexArray(m_VAO);
	//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glDrawArrays(GL_POINTS, 0, m_voxelCount);
	//glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

GLRenderPassComponent * GLVXGIPass::getGLRPC()
{
	return m_voxelVisualizationGLRPC;
}