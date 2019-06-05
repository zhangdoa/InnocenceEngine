#include "GLVXGIPass.h"

#include "GLRenderingBackendUtilities.h"

#include "../../../Component/GLRenderingBackendComponent.h"
#include "../../../Component/RenderingFrontendComponent.h"

#include "../../ICoreSystem.h"

extern ICoreSystem* g_pCoreSystem;

using namespace GLRenderingBackendNS;

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

	std::vector<mat4> m_VP;
	std::vector<mat4> m_VP_inv;
	unsigned int m_volumeDimension = 128;
	unsigned int m_voxelCount = m_volumeDimension * m_volumeDimension * m_volumeDimension;
	float m_volumeGridSize;

	GLRenderPassComponent* m_irradianceInjectionPassGLRPC;
	GLShaderProgramComponent* m_irradianceInjectionPassSPC;

	GLRenderPassComponent* m_voxelVisualizationGLRPC;
	GLShaderProgramComponent* m_voxelVisualizationPassSPC;

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
	auto l_renderPassDesc = GLRenderingBackendComponent::get().m_deferredRenderPassDesc;

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
	auto l_renderPassDesc = GLRenderingBackendComponent::get().m_deferredRenderPassDesc;

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

	m_irradianceInjectionPassSPC = rhs;
}

void GLVXGIPass::initializeVoxelVisualizationPass()
{
	// generate and bind framebuffer
	m_voxelVisualizationGLRPC = addGLRenderPassComponent(m_entityID, "VoxelVisualizationPassGLRPC/");
	m_voxelVisualizationGLRPC->m_renderPassDesc = GLRenderingBackendComponent::get().m_deferredRenderPassDesc;
	initializeGLRenderPassComponent(m_voxelVisualizationGLRPC);

	ShaderFilePaths m_voxelVisualizationPassShaderFilePaths = {};

	////
	m_voxelVisualizationPassShaderFilePaths.m_VSPath = "GL//GIVoxelVisualizationPass.vert/";
	m_voxelVisualizationPassShaderFilePaths.m_GSPath = "GL//GIVoxelVisualizationPass.geom/";
	m_voxelVisualizationPassShaderFilePaths.m_FSPath = "GL//GIVoxelVisualizationPass.frag/";

	auto rhs = addGLShaderProgramComponent(m_entityID);
	initializeGLShaderProgramComponent(rhs, m_voxelVisualizationPassShaderFilePaths);

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

	updateUniform(7, m_volumeDimension);
	updateUniform(8, 1.0f / m_volumeGridSize);
	updateUniform(9, l_sceneAABB.m_boundMin);

	for (size_t i = 0; i < m_VP.size(); i++)
	{
		updateUniform(GLint(1 + i), m_VP[i]);
		updateUniform(GLint(4 + i), m_VP_inv[i]);
	}

	for (auto& l_visibleComponent : g_pCoreSystem->getGameSystem()->get<VisibleComponent>())
	{
		if (l_visibleComponent->m_visiblilityType == VisiblilityType::INNO_OPAQUE && l_visibleComponent->m_objectStatus == ObjectStatus::Activated)
		{
			updateUniform(
				0,
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

	updateUniform(0, m_volumeDimension);
	updateUniform(1, l_voxelSize);
	updateUniform(2, 1.0f / m_volumeGridSize);
	updateUniform(3, l_sceneAABB.m_boundMin);

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

	updateUniform(4, RenderingFrontendComponent::get().m_cameraGPUData.p_original);
	updateUniform(5, RenderingFrontendComponent::get().m_cameraGPUData.r);
	updateUniform(6, RenderingFrontendComponent::get().m_cameraGPUData.t);
	updateUniform(7, l_m);

	updateUniform(1, m_volumeDimension);
	updateUniform(2, l_voxelSize);
	updateUniform(3, l_sceneAABB.m_boundMin);

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