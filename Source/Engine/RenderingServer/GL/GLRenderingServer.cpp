#include "GLRenderingServer.h"
#include "../../Component/GLMeshDataComponent.h"
#include "../../Component/GLTextureDataComponent.h"
#include "../../Component/GLMaterialDataComponent.h"
#include "../../Component/GLRenderPassDataComponent.h"
#include "../../Component/GLShaderProgramComponent.h"

#include "../../ModuleManager/IModuleManager.h"

extern IModuleManager* g_pModuleManager;

namespace GLRenderingServerNS
{
	void MessageCallback(GLenum source,
		GLenum type,
		GLuint id,
		GLenum severity,
		GLsizei length,
		const GLchar* message,
		const void* userParam)
	{
		if (severity == GL_DEBUG_SEVERITY_HIGH)
		{
			LogType l_logType;
			std::string l_typeStr;
			if (type == GL_DEBUG_TYPE_ERROR)
			{
				l_logType = LogType::INNO_ERROR;
				l_typeStr = "GL_DEBUG_TYPE_ERROR: ID: ";
			}
			else if (type == GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR)
			{
				l_logType = LogType::INNO_ERROR;
				l_typeStr = "GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR: ID: ";
			}
			else if (type == GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR)
			{
				l_logType = LogType::INNO_ERROR;
				l_typeStr = "GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: ID: ";
			}
			else if (type == GL_DEBUG_TYPE_PERFORMANCE)
			{
				l_logType = LogType::INNO_ERROR;
				l_typeStr = "GL_DEBUG_TYPE_PERFORMANCE: ID: ";
			}
			else if (type == GL_DEBUG_TYPE_PORTABILITY)
			{
				l_logType = LogType::INNO_ERROR;
				l_typeStr = "GL_DEBUG_TYPE_PORTABILITY: ID: ";
			}
			else if (type == GL_DEBUG_TYPE_OTHER)
			{
				l_logType = LogType::INNO_ERROR;
				l_typeStr = "GL_DEBUG_TYPE_OTHER: ID: ";
			}
			else
			{
				l_logType = LogType::INNO_DEV_VERBOSE;
			}

			std::string l_message = message;
			g_pModuleManager->getLogSystem()->printLog(l_logType, "GLRenderServer: " + l_typeStr + std::to_string(id) + ": " + l_message);
		}
	}

	ObjectStatus m_objectStatus = ObjectStatus::Terminated;

	std::function<void()> f_sceneLoadingFinishCallback;

	bool m_isBaked = true;
	bool m_visualizeVXGI = false;

	std::function<void()> f_toggleVisualizeVXGI;

	void* m_MeshDataComponentPool;
	void* m_MaterialDataComponentPool;
	void* m_TextureDataComponentPool;

	ThreadSafeQueue<GLMeshDataComponent*> m_uninitializedMeshes;
	ThreadSafeQueue<GLMaterialDataComponent*> m_uninitializedMaterials;
}

using namespace GLRenderingServerNS;

bool GLRenderingServer::Setup()
{
	if (g_pModuleManager->getRenderingFrontend()->getRenderingConfig().MSAAdepth)
	{
		glEnable(GL_MULTISAMPLE);
	}

	glEnable(GL_DEBUG_OUTPUT);
	glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
	glDebugMessageCallback(MessageCallback, 0);

	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
	glEnable(GL_PROGRAM_POINT_SIZE);

	m_objectStatus = ObjectStatus::Created;
	g_pModuleManager->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "GLRenderingBackend setup finished.");

	return true;
}

bool GLRenderingServer::Initialize()
{
	return true;
}

bool GLRenderingServer::Terminate()
{
	return true;
}

ObjectStatus GLRenderingServer::GetStatus()
{
	return ObjectStatus();
}

MeshDataComponent * GLRenderingServer::AddMeshDataComponent(const char * name)
{
	return nullptr;
}

TextureDataComponent * GLRenderingServer::AddTextureDataComponent(const char * name)
{
	return nullptr;
}

MaterialDataComponent * GLRenderingServer::AddMaterialDataComponent(const char * name)
{
	return nullptr;
}

RenderPassDataComponent * GLRenderingServer::AddRenderPassDataComponent(const char * name)
{
	return nullptr;
}

ShaderProgramComponent * GLRenderingServer::AddShaderProgramComponent(const char * name)
{
	return nullptr;
}

GPUBufferDataComponent * GLRenderingServer::AddGPUBufferDataComponent(const char * name)
{
	return nullptr;
}

bool GLRenderingServer::InitializeMeshDataComponent(MeshDataComponent * rhs)
{
	return true;
}

bool GLRenderingServer::InitializeTextureDataComponent(TextureDataComponent * rhs)
{
	return true;
}

bool GLRenderingServer::InitializeMaterialDataComponent(MaterialDataComponent * rhs)
{
	return true;
}

bool GLRenderingServer::InitializeRenderPassDataComponent(RenderPassDataComponent * rhs)
{
	return true;
}

bool GLRenderingServer::InitializeShaderProgramComponent(ShaderProgramComponent * rhs)
{
	return true;
}

bool GLRenderingServer::InitializeGPUBufferDataComponent(GPUBufferDataComponent * rhs)
{
	return true;
}

bool GLRenderingServer::DeleteMeshDataComponent(MeshDataComponent * rhs)
{
	return true;
}

bool GLRenderingServer::DeleteTextureDataComponent(TextureDataComponent * rhs)
{
	return true;
}

bool GLRenderingServer::DeleteMaterialDataComponent(MaterialDataComponent * rhs)
{
	return true;
}

bool GLRenderingServer::DeleteRenderPassDataComponent(RenderPassDataComponent * rhs)
{
	return true;
}

bool GLRenderingServer::DeleteShaderProgramComponent(ShaderProgramComponent * rhs)
{
	return true;
}

void GLRenderingServer::RegisterMeshDataComponent(MeshDataComponent * rhs)
{
}

void GLRenderingServer::RegisterMaterialDataComponent(MaterialDataComponent * rhs)
{
}

MeshDataComponent * GLRenderingServer::GetMeshDataComponent(MeshShapeType meshShapeType)
{
	return nullptr;
}

TextureDataComponent * GLRenderingServer::GetTextureDataComponent(TextureUsageType textureUsageType)
{
	return nullptr;
}

TextureDataComponent * GLRenderingServer::GetTextureDataComponent(WorldEditorIconType iconType)
{
	return nullptr;
}

bool GLRenderingServer::UploadGPUBufferDataComponentImpl(GPUBufferDataComponent * rhs, const void * GPUBufferValue)
{
	return true;
}

bool GLRenderingServer::CommandListBegin(RenderPassDataComponent * rhs, size_t frameIndex)
{
	return true;
}

bool GLRenderingServer::BindRenderPassDataComponent(RenderPassDataComponent * rhs)
{
	return true;
}

bool GLRenderingServer::CleanRenderTargets(RenderPassDataComponent * rhs)
{
	return true;
}

bool GLRenderingServer::BindGPUBuffer(ShaderType shaderType, GPUBufferAccessibility accessibility, GPUBufferDataComponent * GPUBufferDataComponent, size_t startOffset, size_t range)
{
	return true;
}

bool GLRenderingServer::BindShaderProgramComponent(ShaderProgramComponent * rhs)
{
	return true;
}

bool GLRenderingServer::BindMaterialDataComponent(MaterialDataComponent * rhs)
{
	return true;
}

bool GLRenderingServer::DispatchDrawCall(MeshDataComponent * rhs)
{
	return true;
}

bool GLRenderingServer::UnbindMaterialDataComponent(RenderPassDataComponent * rhs)
{
	return true;
}

bool GLRenderingServer::CommandListEnd(RenderPassDataComponent * rhs, size_t frameIndex)
{
	return true;
}

bool GLRenderingServer::ExecuteCommandList(RenderPassDataComponent * rhs, size_t frameIndex)
{
	return true;
}

bool GLRenderingServer::WaitForFrame(RenderPassDataComponent * rhs, size_t frameIndex)
{
	return true;
}

bool GLRenderingServer::Present()
{
	return true;
}

bool GLRenderingServer::CopyDepthBuffer(RenderPassDataComponent * src, RenderPassDataComponent * dest)
{
	return true;
}

bool GLRenderingServer::CopyStencilBuffer(RenderPassDataComponent * src, RenderPassDataComponent * dest)
{
	return true;
}

bool GLRenderingServer::CopyColorBuffer(RenderPassDataComponent * src, size_t srcIndex, RenderPassDataComponent * dest, size_t destIndex)
{
	return true;
}

vec4 GLRenderingServer::ReadRenderTargetSample(RenderPassDataComponent * rhs, size_t renderTargetIndex, size_t x, size_t y)
{
	return vec4();
}

std::vector<vec4> GLRenderingServer::ReadTextureBackToCPU(RenderPassDataComponent * canvas, TextureDataComponent * TDC)
{
	return std::vector<vec4>();
}

bool GLRenderingServer::Resize()
{
	return true;
}

bool GLRenderingServer::ReloadShader(RenderPassType renderPassType)
{
	return true;
}

bool GLRenderingServer::BakeGIData()
{
	return true;
}