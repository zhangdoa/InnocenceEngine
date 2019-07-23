#include "GLSHPass.h"

#include "GLRenderingBackendUtilities.h"
#include "../../Component/GLRenderingBackendComponent.h"

using namespace GLRenderingBackendNS;

#include "../../ModuleManager/IModuleManager.h"

extern IModuleManager* g_pModuleManager;

namespace GLSHPass
{
	SH9 samplesToSH(const std::vector<vec4>& samples);
	std::vector<vec4> readbackCubemapSamples(GLRenderPassComponent* GLRPC, GLTextureDataComponent* GLTDC);

	EntityID m_entityID;

	GLRenderPassComponent* m_readbackCanvasGLRPC;
}

bool GLSHPass::initialize()
{
	m_entityID = InnoMath::createEntityID();

	auto l_renderPassDesc = GLRenderingBackendComponent::get().m_deferredRenderPassDesc;
	l_renderPassDesc.RTNumber = 0;
	m_readbackCanvasGLRPC = addGLRenderPassComponent(m_entityID, "SHPassCanvasGLRPC/");
	m_readbackCanvasGLRPC->m_renderPassDesc = l_renderPassDesc;
	m_readbackCanvasGLRPC->m_drawColorBuffers = false;

	initializeGLRenderPassComponent(m_readbackCanvasGLRPC);

	return true;
}

SH9 GLSHPass::getSH9(GLTextureDataComponent * GLTDC)
{
	auto l_textureSamples = readCubemapSamples(m_readbackCanvasGLRPC, GLTDC);
	auto l_result = samplesToSH(l_textureSamples);
	return l_result;
}

SH9 getSH9(vec4 normal, vec4 radiance)
{
	float Y00 = 0.282095f;
	float Y11 = 0.488603f * normal.x;
	float Y10 = 0.488603f * normal.z;
	float Y1_1 = 0.488603f * normal.y;
	float Y21 = 1.092548f * normal.x*normal.z;
	float Y2_1 = 1.092548f * normal.y*normal.z;
	float Y2_2 = 1.092548f * normal.y*normal.x;
	float Y20 = 0.946176f * normal.z * normal.z - 0.315392f;
	float Y22 = 0.546274f * (normal.x*normal.x - normal.y*normal.y);

	SH9 l_result;

	l_result.L00 = radiance * Y00;
	l_result.L11 = radiance * Y11;
	l_result.L10 = radiance * Y10;
	l_result.L1_1 = radiance * Y1_1;
	l_result.L21 = radiance * Y21;
	l_result.L2_1 = radiance * Y2_1;
	l_result.L2_2 = radiance * Y2_2;
	l_result.L20 = radiance * Y20;
	l_result.L22 = radiance * Y22;

	return l_result;
}

SH9 GLSHPass::samplesToSH(const std::vector<vec4>& samples)
{
	auto l_totalSampleCount = samples.size();
	auto l_sampleCountPerFace = l_totalSampleCount / 6;
	auto l_resolution = (unsigned int)std::sqrt(l_sampleCountPerFace);
	auto l_texelSize = 1.0f / (float)l_resolution;

	SH9 l_result;

	////// +X
	for (size_t i = 0; i < l_sampleCountPerFace; i++)
	{
		auto l_x = 1.0f;
		auto l_y = (float)(i % l_resolution) * l_texelSize;
		l_y = l_y * 2.0f - 1.0f;
		auto l_z = (float)((i / l_resolution) % l_resolution) * l_texelSize;
		l_z = l_z * 2.0f - 1.0f;

		auto l_normal = vec4(l_x, l_y, l_z, 0.0f);
		l_normal = l_normal.normalize();

		l_result += getSH9(l_normal, samples[i]);
	}

	////// -X
	for (size_t i = 0; i < l_sampleCountPerFace; i++)
	{
		auto l_x = -1.0f;
		auto l_y = (float)(i % l_resolution) * l_texelSize;
		l_y = l_y * 2.0f - 1.0f;
		auto l_z = (float)((i / l_resolution) % l_resolution) * l_texelSize;
		l_z = l_z * 2.0f - 1.0f;

		auto l_normal = vec4(l_x, l_y, l_z, 0.0f);
		l_normal = l_normal.normalize();

		l_result += getSH9(l_normal, samples[i + l_sampleCountPerFace * 1]);
	}

	////// +Y
	for (size_t i = 0; i < l_sampleCountPerFace; i++)
	{
		auto l_x = (float)(i % l_resolution) * l_texelSize;
		l_x = l_x * 2.0f - 1.0f;
		auto l_y = 1.0f;
		auto l_z = (float)((i / l_resolution) % l_resolution) * l_texelSize;
		l_z = l_z * 2.0f - 1.0f;

		auto l_normal = vec4(l_x, l_y, l_z, 0.0f);
		l_normal = l_normal.normalize();

		l_result += getSH9(l_normal, samples[i + l_sampleCountPerFace * 2]);
	}

	////// -Y
	for (size_t i = 0; i < l_sampleCountPerFace; i++)
	{
		auto l_x = (float)(i % l_resolution) * l_texelSize;
		l_x = l_x * 2.0f - 1.0f;
		auto l_y = -1.0f;
		auto l_z = (float)((i / l_resolution) % l_resolution) * l_texelSize;
		l_z = l_z * 2.0f - 1.0f;

		auto l_normal = vec4(l_x, l_y, l_z, 0.0f);
		l_normal = l_normal.normalize();

		l_result += getSH9(l_normal, samples[i + l_sampleCountPerFace * 3]);
	}

	////// +Z
	for (size_t i = 0; i < l_sampleCountPerFace; i++)
	{
		auto l_x = (float)(i % l_resolution) * l_texelSize;
		l_x = l_x * 2.0f - 1.0f;
		auto l_y = (float)((i / l_resolution) % l_resolution) * l_texelSize;
		l_y = l_y * 2.0f - 1.0f;
		auto l_z = 1.0f;

		auto l_normal = vec4(l_x, l_y, l_z, 0.0f);
		l_normal = l_normal.normalize();

		l_result += getSH9(l_normal, samples[i + l_sampleCountPerFace * 4]);
	}

	////// -Z
	for (size_t i = 0; i < l_sampleCountPerFace; i++)
	{
		auto l_x = (float)(i % l_resolution) * l_texelSize;
		l_x = l_x * 2.0f - 1.0f;
		auto l_y = (float)((i / l_resolution) % l_resolution) * l_texelSize;
		l_y = l_y * 2.0f - 1.0f;
		auto l_z = -1.0f;

		auto l_normal = vec4(l_x, l_y, l_z, 0.0f);
		l_normal = l_normal.normalize();

		l_result += getSH9(l_normal, samples[i + l_sampleCountPerFace * 5]);
	}

	l_result /= (float)l_totalSampleCount;

	return l_result;
}