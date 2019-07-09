#include "GLVXGIPass.h"
#include "../../Common/CommonMacro.inl"
#include "../../ComponentManager/ITransformComponentManager.h"
#include "../../ComponentManager/IVisibleComponentManager.h"

#include "GLRenderingBackendUtilities.h"

#include "../../Component/GLRenderingBackendComponent.h"

#include "../../ModuleManager/IModuleManager.h"

extern IModuleManager* g_pModuleManager;

using namespace GLRenderingBackendNS;

INNO_PRIVATE_SCOPE GLVXGIPass
{
void initializeVoxelTestPass();
void initializeVoxelizationPass();
//void initializeVoxelVisualizationPass();

void updateVoxelTestPass();
void updateVoxelizationPass();
//void updateVoxelVisualizationPass();

EntityID m_entityID;
std::vector<mat4> m_VP;
std::vector<mat4> m_VP_inv;
unsigned int m_volumeDimension = 128;
unsigned int m_voxelCount = m_volumeDimension * m_volumeDimension * m_volumeDimension;
float m_volumeEdgeSize;

GLTextureDataComponent* m_voxelTestPassGLTDC;
GLShaderProgramComponent* m_voxelTestPassGLSPC;

GLTextureDataComponent* m_voxelizationPassGLTDC;
GLShaderProgramComponent* m_voxelizationPassGLSPC;

//GLRenderPassComponent* m_voxelVisualizationGLRPC;
//GLShaderProgramComponent* m_voxelVisualizationPassGLSPC;

GLuint m_VAO;
}

void GLVXGIPass::initialize()
{
	m_entityID = InnoMath::createEntityID();

	initializeVoxelTestPass();

	initializeVoxelizationPass();

	//initializeVoxelVisualizationPass();

	glGenVertexArrays(1, &m_VAO);
}

void GLVXGIPass::initializeVoxelTestPass()
{
	m_voxelTestPassGLTDC = addGLTextureDataComponent();
	m_voxelTestPassGLTDC->m_textureDataDesc.samplerType = TextureSamplerType::SAMPLER_3D;
	m_voxelTestPassGLTDC->m_textureDataDesc.usageType = TextureUsageType::RAW_IMAGE;
	m_voxelTestPassGLTDC->m_textureDataDesc.pixelDataFormat = TexturePixelDataFormat::RGBA;
	m_voxelTestPassGLTDC->m_textureDataDesc.minFilterMethod = TextureFilterMethod::NEAREST;
	m_voxelTestPassGLTDC->m_textureDataDesc.magFilterMethod = TextureFilterMethod::NEAREST;
	m_voxelTestPassGLTDC->m_textureDataDesc.wrapMethod = TextureWrapMethod::REPEAT;
	m_voxelTestPassGLTDC->m_textureDataDesc.width = m_volumeDimension;
	m_voxelTestPassGLTDC->m_textureDataDesc.height = m_volumeDimension;
	m_voxelTestPassGLTDC->m_textureDataDesc.depth = m_volumeDimension;
	m_voxelTestPassGLTDC->m_textureDataDesc.pixelDataType = TexturePixelDataType::UBYTE;
	m_voxelTestPassGLTDC->m_textureData = nullptr;

	initializeGLTextureDataComponent(m_voxelTestPassGLTDC);

	// shader programs and shaders
	ShaderFilePaths m_voxelTestPassShaderFilePaths = {};

	////
	m_voxelTestPassShaderFilePaths.m_VSPath = "GL//GIVoxelTestPass.vert/";
	m_voxelTestPassShaderFilePaths.m_FSPath = "GL//GIVoxelTestPass.frag/";

	auto rhs = addGLShaderProgramComponent(m_entityID);
	initializeGLShaderProgramComponent(rhs, m_voxelTestPassShaderFilePaths);

	m_voxelTestPassGLSPC = rhs;
}

void GLVXGIPass::initializeVoxelizationPass()
{
	m_voxelizationPassGLTDC = addGLTextureDataComponent();
	m_voxelizationPassGLTDC->m_textureDataDesc.samplerType = TextureSamplerType::SAMPLER_3D;
	m_voxelizationPassGLTDC->m_textureDataDesc.usageType = TextureUsageType::RAW_IMAGE;
	m_voxelizationPassGLTDC->m_textureDataDesc.pixelDataFormat = TexturePixelDataFormat::R;
	m_voxelizationPassGLTDC->m_textureDataDesc.minFilterMethod = TextureFilterMethod::NEAREST;
	m_voxelizationPassGLTDC->m_textureDataDesc.magFilterMethod = TextureFilterMethod::NEAREST;
	m_voxelizationPassGLTDC->m_textureDataDesc.wrapMethod = TextureWrapMethod::REPEAT;
	m_voxelizationPassGLTDC->m_textureDataDesc.width = m_volumeDimension;
	m_voxelizationPassGLTDC->m_textureDataDesc.height = m_volumeDimension;
	m_voxelizationPassGLTDC->m_textureDataDesc.depth = m_volumeDimension;
	m_voxelizationPassGLTDC->m_textureDataDesc.pixelDataType = TexturePixelDataType::UINT32;
	m_voxelizationPassGLTDC->m_textureData = nullptr;

	initializeGLTextureDataComponent(m_voxelizationPassGLTDC);

	// shader programs and shaders
	ShaderFilePaths m_voxelizationPassShaderFilePaths = {};

	////
	m_voxelizationPassShaderFilePaths.m_VSPath = "GL//GIVoxelizationPass.vert/";
	m_voxelizationPassShaderFilePaths.m_GSPath = "GL//GIVoxelizationPass.geom/";
	m_voxelizationPassShaderFilePaths.m_FSPath = "GL//GIVoxelizationPass.frag/";

	auto rhs = addGLShaderProgramComponent(m_entityID);
	initializeGLShaderProgramComponent(rhs, m_voxelizationPassShaderFilePaths);

	m_voxelizationPassGLSPC = rhs;

	m_VP.reserve(3);
	m_VP_inv.reserve(3);

	for (size_t i = 0; i < 3; i++)
	{
		m_VP.emplace_back();
		m_VP_inv.emplace_back();
	}
}

//void GLVXGIPass::initializeVoxelVisualizationPass()
//{
//	// generate and bind framebuffer
//	m_voxelVisualizationGLRPC = addGLRenderPassComponent(m_entityID, "VoxelVisualizationPassGLRPC/");
//	m_voxelVisualizationGLRPC->m_renderPassDesc = GLRenderingBackendComponent::get().m_deferredRenderPassDesc;
//	initializeGLRenderPassComponent(m_voxelVisualizationGLRPC);
//
//	ShaderFilePaths m_voxelVisualizationPassShaderFilePaths = {};
//
//	////
//	m_voxelVisualizationPassShaderFilePaths.m_VSPath = "GL//GIVoxelVisualizationPass.vert/";
//	m_voxelVisualizationPassShaderFilePaths.m_GSPath = "GL//GIVoxelVisualizationPass.geom/";
//	m_voxelVisualizationPassShaderFilePaths.m_FSPath = "GL//GIVoxelVisualizationPass.frag/";
//
//	auto rhs = addGLShaderProgramComponent(m_entityID);
//	initializeGLShaderProgramComponent(rhs, m_voxelVisualizationPassShaderFilePaths);
//
//	m_voxelVisualizationPassGLSPC = rhs;
//}

void GLVXGIPass::update()
{
	updateVoxelTestPass();
	updateVoxelizationPass();
}

void GLVXGIPass::draw()
{
	//updateVoxelVisualizationPass();
}

void GLVXGIPass::updateVoxelTestPass()
{
	glBindImageTexture(0, m_voxelTestPassGLTDC->m_TO, 0, GL_TRUE, 0, GL_READ_WRITE, GL_RGBA8);

	activateShaderProgram(m_voxelTestPassGLSPC);

	updateUniform(0, m_volumeDimension);

	glBindVertexArray(m_VAO);
	glDrawArrays(GL_POINTS, 0, m_voxelCount);

	glBindImageTexture(0, 0, 0, GL_TRUE, 0, GL_READ_WRITE, GL_RGBA8);
}

void GLVXGIPass::updateVoxelizationPass()
{
	auto l_sceneAABB = g_pModuleManager->getPhysicsSystem()->getTotalSceneAABB();

	auto l_axisSize = l_sceneAABB.m_extend;
	auto l_center = l_sceneAABB.m_center;
	m_volumeEdgeSize = std::max(l_axisSize.x, std::max(l_axisSize.y, l_axisSize.z));
	auto l_halfSize = m_volumeEdgeSize / 2.0f;

	// projection matrices
	auto l_p = InnoMath::generateOrthographicMatrix(-l_halfSize, l_halfSize, -l_halfSize, l_halfSize, 0.0f, m_volumeEdgeSize);

	// view matrices
	m_VP[0] = InnoMath::lookAt(l_center + vec4(l_halfSize, 0.0f, 0.0f, 0.0f),
		l_center, vec4(0.0f, 1.0f, 0.0f, 0.0f));
	m_VP[1] = InnoMath::lookAt(l_center + vec4(0.0f, l_halfSize, 0.0f, 0.0f),
		l_center, vec4(0.0f, 0.0f, 1.0f, 0.0f));
	m_VP[2] = InnoMath::lookAt(l_center + vec4(0.0f, 0.0f, l_halfSize, 0.0f),
		l_center, vec4(0.0f, 1.0f, 0.0f, 0.0f));

	for (size_t i = 0; i < 3; i++)
	{
		m_VP[i] = l_p * m_VP[i];
		m_VP_inv[i] = m_VP[i].inverse();
	}

	// voxelization pass
	glBindImageTexture(0, m_voxelizationPassGLTDC->m_TO, 0, GL_TRUE, 0, GL_READ_WRITE, GL_R32UI);

	activateShaderProgram(m_voxelizationPassGLSPC);

	updateUniform(6, m_volumeDimension);
	updateUniform(7, 1.0f / m_volumeEdgeSize);
	updateUniform(8, l_sceneAABB.m_boundMin);

	for (size_t i = 0; i < m_VP.size(); i++)
	{
		updateUniform(GLint(i), m_VP[i]);
		updateUniform(GLint(3 + i), m_VP_inv[i]);
	}

	updateUBO(GLRenderingBackendComponent::get().m_meshUBO, g_pModuleManager->getRenderingFrontend()->getGIPassMeshGPUData());
	updateUBO(GLRenderingBackendComponent::get().m_materialUBO, g_pModuleManager->getRenderingFrontend()->getGIPassMaterialGPUData());

	unsigned int l_offset = 0;

	auto l_totalDrawCallCount = g_pModuleManager->getRenderingFrontend()->getGIPassDrawCallCount();
	for (unsigned int i = 0; i < l_totalDrawCallCount; i++)
	{
		auto l_GIPassGPUData = g_pModuleManager->getRenderingFrontend()->getGIPassGPUData()[i];

		bindUBO(GLRenderingBackendComponent::get().m_meshUBO, 1, l_offset * sizeof(MeshGPUData), sizeof(MeshGPUData));
		bindUBO(GLRenderingBackendComponent::get().m_materialUBO, 2, l_offset * sizeof(MaterialGPUData), sizeof(MaterialGPUData));

		drawMesh(reinterpret_cast<GLMeshDataComponent*>(l_GIPassGPUData.MDC));

		l_offset++;
	}

	glBindImageTexture(0, 0, 0, GL_TRUE, 0, GL_READ_WRITE, GL_R32UI);

	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);
}

//void GLVXGIPass::updateVoxelVisualizationPass()
//{
//
//	auto l_sceneAABB = g_pModuleManager->getPhysicsSystem()->getTotalSceneAABB();
//
//	auto axisSize = l_sceneAABB.m_extend;
//	m_volumeGridSize = std::max(axisSize.x, std::max(axisSize.y, axisSize.z));
//	auto l_voxelSize = m_volumeGridSize / m_volumeDimension;
//
//	// voxel visualization pass
//	auto l_ms = InnoMath::toScaleMatrix(vec4(l_voxelSize, l_voxelSize, l_voxelSize, 1.0f));
//	auto l_mt = InnoMath::toTranslationMatrix(l_sceneAABB.m_boundMin);
//
//	auto l_m = l_mt * l_ms;
//
//	activateRenderPass(m_voxelVisualizationGLRPC);
//
//	activateShaderProgram(m_voxelVisualizationPassGLSPC);
//
//	updateUniform(4, g_pModuleManager->getRenderingFrontend()->getCameraGPUData().p_original);
//	updateUniform(5, g_pModuleManager->getRenderingFrontend()->getCameraGPUData().r);
//	updateUniform(6, g_pModuleManager->getRenderingFrontend()->getCameraGPUData().t);
//	updateUniform(7, l_m);
//
//	updateUniform(1, m_volumeDimension);
//	updateUniform(2, l_voxelSize);
//	updateUniform(3, l_sceneAABB.m_boundMin);
//
//	glBindImageTexture(3, m_voxelizationPassGLRPC->m_GLTDCs[0]->m_TO, 0, GL_TRUE, 0, GL_READ_ONLY, GL_RGBA8);
//
//	glBindVertexArray(m_VAO);
//	//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
//	glDrawArrays(GL_POINTS, 0, m_voxelCount);
//	//glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
//}

GLRenderPassComponent * GLVXGIPass::getGLRPC()
{
	return nullptr;
}