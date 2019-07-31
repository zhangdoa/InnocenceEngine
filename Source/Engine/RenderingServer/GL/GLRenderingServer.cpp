#include "GLRenderingServer.h"
#include "../../Component/GLMeshDataComponent.h"
#include "../../Component/GLTextureDataComponent.h"
#include "../../Component/GLMaterialDataComponent.h"
#include "../../Component/GLRenderPassDataComponent.h"
#include "../../Component/GLShaderProgramComponent.h"
#include "../../Component/GLGPUBufferDataComponent.h"

#include "GLHelper.h"

using namespace GLHelper;

#include "../../ModuleManager/IModuleManager.h"

extern IModuleManager* g_pModuleManager;

#include "../../Core/InnoLogger.h"
#include "../../Core/InnoMemory.h"

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
			LogLevel l_logLevel;
			const char* l_typeStr;
			if (type == GL_DEBUG_TYPE_ERROR)
			{
				l_logLevel = LogLevel::Error;
				l_typeStr = "GL_DEBUG_TYPE_ERROR: ";
			}
			else if (type == GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR)
			{
				l_logLevel = LogLevel::Error;
				l_typeStr = "GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR: ";
			}
			else if (type == GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR)
			{
				l_logLevel = LogLevel::Error;
				l_typeStr = "GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: ";
			}
			else if (type == GL_DEBUG_TYPE_PERFORMANCE)
			{
				l_logLevel = LogLevel::Error;
				l_typeStr = "GL_DEBUG_TYPE_PERFORMANCE: ";
			}
			else if (type == GL_DEBUG_TYPE_PORTABILITY)
			{
				l_logLevel = LogLevel::Error;
				l_typeStr = "GL_DEBUG_TYPE_PORTABILITY: ";
			}
			else if (type == GL_DEBUG_TYPE_OTHER)
			{
				l_logLevel = LogLevel::Error;
				l_typeStr = "GL_DEBUG_TYPE_OTHER: ";
			}
			else
			{
				l_logLevel = LogLevel::Verbose;
			}

			InnoLogger::Log(l_logLevel, "GLRenderServer: ", l_typeStr, "ID: ", id, ": ", message);
		}
	}

	ObjectStatus m_objectStatus = ObjectStatus::Terminated;

	IObjectPool* m_MeshDataComponentPool = 0;
	IObjectPool* m_MaterialDataComponentPool = 0;
	IObjectPool* m_TextureDataComponentPool = 0;
	IObjectPool* m_RenderPassDataComponentPool = 0;
	IObjectPool* m_ResourcesBinderPool = 0;
	IObjectPool* m_PSOPool = 0;
	IObjectPool* m_ShaderProgramComponentPool = 0;

	std::unordered_set<MeshDataComponent*> m_initializedMeshes;
	std::unordered_set<TextureDataComponent*> m_initializedTextures;
	std::unordered_set<MaterialDataComponent*> m_initializedMaterials;

	GLRenderPassDataComponent* m_SwapChainRPDC = 0;
}

using namespace GLRenderingServerNS;

GLResourceBinder* addResourcesBinder()
{
	auto l_BinderRawPtr = m_ResourcesBinderPool->Spawn();
	auto l_Binder = new(l_BinderRawPtr)GLResourceBinder();
	return l_Binder;
}

GLPipelineStateObject* addPSO()
{
	auto l_PSORawPtr = m_PSOPool->Spawn();
	auto l_PSO = new(l_PSORawPtr)GLPipelineStateObject();
	return l_PSO;
}

bool GLRenderingServer::Setup()
{
	auto l_renderingCapability = g_pModuleManager->getRenderingFrontend()->getRenderingCapability();

	m_MeshDataComponentPool = InnoMemory::CreateObjectPool(sizeof(GLMeshDataComponent), l_renderingCapability.maxMeshes);
	m_TextureDataComponentPool = InnoMemory::CreateObjectPool(sizeof(GLTextureDataComponent), l_renderingCapability.maxTextures);
	m_MaterialDataComponentPool = InnoMemory::CreateObjectPool(sizeof(GLMaterialDataComponent), l_renderingCapability.maxMaterials);
	m_RenderPassDataComponentPool = InnoMemory::CreateObjectPool(sizeof(GLRenderPassDataComponent), 128);
	m_ResourcesBinderPool = InnoMemory::CreateObjectPool(sizeof(GLResourceBinder), 16384);
	m_PSOPool = InnoMemory::CreateObjectPool(sizeof(GLPipelineStateObject), 128);
	m_ShaderProgramComponentPool = InnoMemory::CreateObjectPool(sizeof(GLShaderProgramComponent), 256);

	glEnable(GL_DEBUG_OUTPUT);
	glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
	glDebugMessageCallback(MessageCallback, 0);

	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
	glEnable(GL_PROGRAM_POINT_SIZE);

	m_SwapChainRPDC = reinterpret_cast<GLRenderPassDataComponent*>(AddRenderPassDataComponent("SwapChain/"));

	m_objectStatus = ObjectStatus::Created;
	InnoLogger::Log(LogLevel::Success, "GLRenderingServer setup finished.");

	return true;
}

bool GLRenderingServer::Initialize()
{
	if (m_objectStatus == ObjectStatus::Created)
	{
		auto l_RenderPassDesc = g_pModuleManager->getRenderingFrontend()->getDefaultRenderPassDesc();

		l_RenderPassDesc.m_RenderTargetCount = 1;

		m_SwapChainRPDC->m_RenderPassDesc = l_RenderPassDesc;
		m_SwapChainRPDC->m_RenderPassDesc.m_RenderTargetDesc.pixelDataType = TexturePixelDataType::UBYTE;

		m_SwapChainRPDC->m_FBO = 0;
		m_SwapChainRPDC->m_RBO = 0;

		ReserveRenderTargets(m_SwapChainRPDC, this);

		m_SwapChainRPDC->m_RenderTargetsResourceBinder = addResourcesBinder();

		CreateResourcesBinder(m_SwapChainRPDC);

		m_SwapChainRPDC->m_PipelineStateObject = addPSO();

		CreateStateObjects(m_SwapChainRPDC);

		m_SwapChainRPDC->m_objectStatus = ObjectStatus::Activated;
	}

	return true;
}

bool GLRenderingServer::Terminate()
{
	m_objectStatus = ObjectStatus::Terminated;
	InnoLogger::Log(LogLevel::Success, "GLRenderingServer has been terminated.");

	return true;
}

ObjectStatus GLRenderingServer::GetStatus()
{
	return m_objectStatus;
}

MeshDataComponent * GLRenderingServer::AddMeshDataComponent(const char * name)
{
	static std::atomic<unsigned int> l_count = 0;
	l_count++;
	auto l_rawPtr = m_MeshDataComponentPool->Spawn();
	auto l_result = new(l_rawPtr)GLMeshDataComponent();
	std::string l_name;
	if (strcmp(name, ""))
	{
		l_name = name;
	}
	else
	{
		l_name = ("Mesh_" + std::to_string(l_count) + "/");
	}
	auto l_parentEntity = g_pModuleManager->getEntityManager()->Spawn(ObjectSource::Runtime, ObjectUsage::Engine, l_name.c_str());
	l_result->m_parentEntity = l_parentEntity;
	l_result->m_componentName = l_name.c_str();
	return l_result;
}

TextureDataComponent * GLRenderingServer::AddTextureDataComponent(const char * name)
{
	static std::atomic<unsigned int> l_count = 0;
	l_count++;
	auto l_rawPtr = m_TextureDataComponentPool->Spawn();
	auto l_result = new(l_rawPtr)GLTextureDataComponent();
	std::string l_name;
	if (strcmp(name, ""))
	{
		l_name = name;
	}
	else
	{
		l_name = ("Texture_" + std::to_string(l_count) + "/");
	}
	auto l_parentEntity = g_pModuleManager->getEntityManager()->Spawn(ObjectSource::Runtime, ObjectUsage::Engine, l_name.c_str());
	l_result->m_parentEntity = l_parentEntity;
	l_result->m_componentName = l_name.c_str();
	return l_result;
}

MaterialDataComponent * GLRenderingServer::AddMaterialDataComponent(const char * name)
{
	static std::atomic<unsigned int> l_count = 0;
	l_count++;
	auto l_rawPtr = m_MaterialDataComponentPool->Spawn();
	auto l_result = new(l_rawPtr)GLMaterialDataComponent();
	std::string l_name;
	if (strcmp(name, ""))
	{
		l_name = name;
	}
	else
	{
		l_name = ("Material_" + std::to_string(l_count) + "/");
	}
	auto l_parentEntity = g_pModuleManager->getEntityManager()->Spawn(ObjectSource::Runtime, ObjectUsage::Engine, l_name.c_str());
	l_result->m_parentEntity = l_parentEntity;
	l_result->m_componentName = l_name.c_str();
	return l_result;
}

RenderPassDataComponent * GLRenderingServer::AddRenderPassDataComponent(const char * name)
{
	static std::atomic<unsigned int> l_count = 0;
	l_count++;
	auto l_rawPtr = m_RenderPassDataComponentPool->Spawn();
	auto l_result = new(l_rawPtr)GLRenderPassDataComponent();
	std::string l_name;
	if (strcmp(name, ""))
	{
		l_name = name;
	}
	else
	{
		l_name = ("RenderPass_" + std::to_string(l_count) + "/");
	}
	auto l_parentEntity = g_pModuleManager->getEntityManager()->Spawn(ObjectSource::Runtime, ObjectUsage::Engine, l_name.c_str());
	l_result->m_parentEntity = l_parentEntity;
	l_result->m_componentName = l_name.c_str();
	return l_result;
}

ShaderProgramComponent * GLRenderingServer::AddShaderProgramComponent(const char * name)
{
	static std::atomic<unsigned int> l_count = 0;
	l_count++;
	auto l_rawPtr = m_ShaderProgramComponentPool->Spawn();
	auto l_result = new(l_rawPtr)GLShaderProgramComponent();
	std::string l_name;
	if (strcmp(name, ""))
	{
		l_name = name;
	}
	else
	{
		l_name = ("ShaderProgram_" + std::to_string(l_count) + "/");
	}
	auto l_parentEntity = g_pModuleManager->getEntityManager()->Spawn(ObjectSource::Runtime, ObjectUsage::Engine, l_name.c_str());
	l_result->m_parentEntity = l_parentEntity;
	l_result->m_componentName = l_name.c_str();
	return l_result;
}

GPUBufferDataComponent * GLRenderingServer::AddGPUBufferDataComponent(const char * name)
{
	static std::atomic<unsigned int> l_count = 0;
	l_count++;
	auto l_rawPtr = m_ShaderProgramComponentPool->Spawn();
	auto l_result = new(l_rawPtr)GLGPUBufferDataComponent();
	std::string l_name;
	if (strcmp(name, ""))
	{
		l_name = name;
	}
	else
	{
		l_name = ("GPUBufferData_" + std::to_string(l_count) + "/");
	}
	auto l_parentEntity = g_pModuleManager->getEntityManager()->Spawn(ObjectSource::Runtime, ObjectUsage::Engine, l_name.c_str());
	l_result->m_parentEntity = l_parentEntity;
	l_result->m_componentName = l_name.c_str();
	return l_result;
}

bool GLRenderingServer::InitializeMeshDataComponent(MeshDataComponent * rhs)
{
	if (m_initializedMeshes.find(rhs) != m_initializedMeshes.end())
	{
		return true;
	}

	auto l_rhs = reinterpret_cast<GLMeshDataComponent*>(rhs);

	glGenVertexArrays(1, &l_rhs->m_VAO);
	glBindVertexArray(l_rhs->m_VAO);

	glGenBuffers(1, &l_rhs->m_VBO);
	glBindBuffer(GL_ARRAY_BUFFER, l_rhs->m_VBO);

	glGenBuffers(1, &l_rhs->m_IBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, l_rhs->m_IBO);

#ifdef _DEBUG
	auto l_VAOName = std::string(l_rhs->m_componentName.c_str());
	l_VAOName += "_VAO";
	glObjectLabel(GL_VERTEX_ARRAY, l_rhs->m_VAO, (GLsizei)l_VAOName.size(), l_VAOName.c_str());
	auto l_VBOName = std::string(l_rhs->m_componentName.c_str());
	l_VBOName += "_VBO";
	glObjectLabel(GL_BUFFER, l_rhs->m_VBO, (GLsizei)l_VBOName.size(), l_VBOName.c_str());
	auto l_IBOName = std::string(l_rhs->m_componentName.c_str());
	l_IBOName += "_IBO";
	glObjectLabel(GL_BUFFER, l_rhs->m_IBO, (GLsizei)l_IBOName.size(), l_IBOName.c_str());
#endif

	// position vec4
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);

	// texture coordinate vec2
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)16);

	// pad1 vec2
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)24);

	// normal vec4
	glEnableVertexAttribArray(3);
	glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)32);

	// pad2 vec4
	glEnableVertexAttribArray(4);
	glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)48);

	InnoLogger::Log(LogLevel::Verbose, "GLRenderingServer: VAO ", l_rhs->m_VAO, " is initialized.");

	glBufferData(GL_ARRAY_BUFFER, l_rhs->m_vertices.size() * sizeof(Vertex), &l_rhs->m_vertices[0], GL_STATIC_DRAW);

	InnoLogger::Log(LogLevel::Verbose, "GLRenderingServer: VBO ", l_rhs->m_VBO, " is initialized.");

	glBufferData(GL_ELEMENT_ARRAY_BUFFER, l_rhs->m_indices.size() * sizeof(Index), &l_rhs->m_indices[0], GL_STATIC_DRAW);

	InnoLogger::Log(LogLevel::Verbose, "GLRenderingServer: IBO ", l_rhs->m_IBO, " is initialized.");

	l_rhs->m_objectStatus = ObjectStatus::Activated;

	m_initializedMeshes.emplace(l_rhs);

	return true;
}

bool GLRenderingServer::InitializeTextureDataComponent(TextureDataComponent * rhs)
{
	if (m_initializedTextures.find(rhs) != m_initializedTextures.end())
	{
		return true;
	}

	auto l_rhs = reinterpret_cast<GLTextureDataComponent*>(rhs);

	l_rhs->m_GLTextureDataDesc = GetGLTextureDataDesc(l_rhs->m_textureDataDesc);

	glGenTextures(1, &l_rhs->m_TO);

	glBindTexture(l_rhs->m_GLTextureDataDesc.textureSamplerType, l_rhs->m_TO);

#ifdef _DEBUG
	auto l_TOName = std::string(l_rhs->m_componentName.c_str());
	l_TOName += "_TO";
	glObjectLabel(GL_TEXTURE, l_rhs->m_TO, (GLsizei)l_TOName.size(), l_TOName.c_str());
#endif

	glTexParameteri(l_rhs->m_GLTextureDataDesc.textureSamplerType, GL_TEXTURE_WRAP_R, l_rhs->m_GLTextureDataDesc.textureWrapMethod);
	glTexParameteri(l_rhs->m_GLTextureDataDesc.textureSamplerType, GL_TEXTURE_WRAP_S, l_rhs->m_GLTextureDataDesc.textureWrapMethod);
	glTexParameteri(l_rhs->m_GLTextureDataDesc.textureSamplerType, GL_TEXTURE_WRAP_T, l_rhs->m_GLTextureDataDesc.textureWrapMethod);

	glTexParameterfv(l_rhs->m_GLTextureDataDesc.textureSamplerType, GL_TEXTURE_BORDER_COLOR, l_rhs->m_GLTextureDataDesc.borderColor);

	glTexParameteri(l_rhs->m_GLTextureDataDesc.textureSamplerType, GL_TEXTURE_MIN_FILTER, l_rhs->m_GLTextureDataDesc.minFilterParam);
	glTexParameteri(l_rhs->m_GLTextureDataDesc.textureSamplerType, GL_TEXTURE_MAG_FILTER, l_rhs->m_GLTextureDataDesc.magFilterParam);

	if (l_rhs->m_GLTextureDataDesc.textureSamplerType == GL_TEXTURE_1D)
	{
		glTexImage1D(GL_TEXTURE_1D, 0, l_rhs->m_GLTextureDataDesc.internalFormat, l_rhs->m_GLTextureDataDesc.width, 0, l_rhs->m_GLTextureDataDesc.pixelDataFormat, l_rhs->m_GLTextureDataDesc.pixelDataType, l_rhs->m_textureData);
	}
	else if (l_rhs->m_GLTextureDataDesc.textureSamplerType == GL_TEXTURE_2D)
	{
		glTexImage2D(GL_TEXTURE_2D, 0, l_rhs->m_GLTextureDataDesc.internalFormat, l_rhs->m_GLTextureDataDesc.width, l_rhs->m_GLTextureDataDesc.height, 0, l_rhs->m_GLTextureDataDesc.pixelDataFormat, l_rhs->m_GLTextureDataDesc.pixelDataType, l_rhs->m_textureData);
	}
	else if (l_rhs->m_GLTextureDataDesc.textureSamplerType == GL_TEXTURE_3D)
	{
		glTexImage3D(GL_TEXTURE_3D, 0, l_rhs->m_GLTextureDataDesc.internalFormat, l_rhs->m_GLTextureDataDesc.width, l_rhs->m_GLTextureDataDesc.height, l_rhs->m_GLTextureDataDesc.depth, 0, l_rhs->m_GLTextureDataDesc.pixelDataFormat, l_rhs->m_GLTextureDataDesc.pixelDataType, l_rhs->m_textureData);
	}
	else if (l_rhs->m_GLTextureDataDesc.textureSamplerType == GL_TEXTURE_CUBE_MAP)
	{
		if (l_rhs->m_textureData)
		{
			for (unsigned int i = 0; i < 6; i++)
			{
				char* l_textureData = reinterpret_cast<char*>(const_cast<void*>(l_rhs->m_textureData));
				auto l_offset = i * l_rhs->m_GLTextureDataDesc.width * l_rhs->m_GLTextureDataDesc.height * l_rhs->m_GLTextureDataDesc.pixelDataSize;
				glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, l_rhs->m_GLTextureDataDesc.internalFormat, l_rhs->m_GLTextureDataDesc.width, l_rhs->m_GLTextureDataDesc.height, 0, l_rhs->m_GLTextureDataDesc.pixelDataFormat, l_rhs->m_GLTextureDataDesc.pixelDataType, l_textureData + l_offset);
			}
		}
		else
		{
			for (unsigned int i = 0; i < 6; i++)
			{
				glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, l_rhs->m_GLTextureDataDesc.internalFormat, l_rhs->m_GLTextureDataDesc.width, l_rhs->m_GLTextureDataDesc.height, 0, l_rhs->m_GLTextureDataDesc.pixelDataFormat, l_rhs->m_GLTextureDataDesc.pixelDataType, l_rhs->m_textureData);
			}
		}
	}

	// should generate mipmap or not
	if (l_rhs->m_GLTextureDataDesc.minFilterParam == GL_LINEAR_MIPMAP_LINEAR)
	{
		glGenerateMipmap(l_rhs->m_GLTextureDataDesc.textureSamplerType);
	}

	InnoLogger::Log(LogLevel::Verbose, "GLRenderingServer: TO ", l_rhs->m_TO, " is initialized.");

	l_rhs->m_objectStatus = ObjectStatus::Activated;

	m_initializedTextures.emplace(l_rhs);

	return true;
}

bool GLRenderingServer::InitializeMaterialDataComponent(MaterialDataComponent * rhs)
{
	if (m_initializedMaterials.find(rhs) != m_initializedMaterials.end())
	{
		return true;
	}

	if (rhs->m_normalTexture)
	{
		InitializeTextureDataComponent(rhs->m_normalTexture);
	}
	if (rhs->m_albedoTexture)
	{
		InitializeTextureDataComponent(rhs->m_albedoTexture);
	}
	if (rhs->m_metallicTexture)
	{
		InitializeTextureDataComponent(rhs->m_metallicTexture);
	}
	if (rhs->m_roughnessTexture)
	{
		InitializeTextureDataComponent(rhs->m_roughnessTexture);
	}
	if (rhs->m_aoTexture)
	{
		InitializeTextureDataComponent(rhs->m_aoTexture);
	}

	rhs->m_objectStatus = ObjectStatus::Activated;

	m_initializedMaterials.emplace(rhs);

	return true;
}

bool GLRenderingServer::InitializeRenderPassDataComponent(RenderPassDataComponent * rhs)
{
	auto l_rhs = reinterpret_cast<GLRenderPassDataComponent*>(rhs);

	CreateFramebuffer(l_rhs);

	ReserveRenderTargets(l_rhs, this);

	CreateRenderTargets(l_rhs, this);

	l_rhs->m_RenderTargetsResourceBinder = addResourcesBinder();

	CreateResourcesBinder(l_rhs);

	l_rhs->m_PipelineStateObject = addPSO();

	CreateStateObjects(l_rhs);

	l_rhs->m_objectStatus = ObjectStatus::Activated;

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);

	return true;
}

bool GLRenderingServer::InitializeShaderProgramComponent(ShaderProgramComponent * rhs)
{
	auto l_rhs = reinterpret_cast<GLShaderProgramComponent*>(rhs);

	l_rhs->m_ProgramID = glCreateProgram();

	if (l_rhs->m_ShaderFilePaths.m_VSPath != "")
	{
		AddShaderHandle(l_rhs->m_ProgramID, l_rhs->m_VSID, GL_VERTEX_SHADER, l_rhs->m_ShaderFilePaths.m_VSPath);
	}
	if (l_rhs->m_ShaderFilePaths.m_TCSPath != "")
	{
		AddShaderHandle(l_rhs->m_ProgramID, l_rhs->m_TCSID, GL_TESS_CONTROL_SHADER, l_rhs->m_ShaderFilePaths.m_TCSPath);
	}
	if (l_rhs->m_ShaderFilePaths.m_TESPath != "")
	{
		AddShaderHandle(l_rhs->m_ProgramID, l_rhs->m_TESID, GL_TESS_EVALUATION_SHADER, l_rhs->m_ShaderFilePaths.m_TESPath);
	}
	if (l_rhs->m_ShaderFilePaths.m_GSPath != "")
	{
		AddShaderHandle(l_rhs->m_ProgramID, l_rhs->m_GSID, GL_GEOMETRY_SHADER, l_rhs->m_ShaderFilePaths.m_GSPath);
	}
	if (l_rhs->m_ShaderFilePaths.m_FSPath != "")
	{
		AddShaderHandle(l_rhs->m_ProgramID, l_rhs->m_FSID, GL_FRAGMENT_SHADER, l_rhs->m_ShaderFilePaths.m_FSPath);
	}
	if (l_rhs->m_ShaderFilePaths.m_CSPath != "")
	{
		AddShaderHandle(l_rhs->m_ProgramID, l_rhs->m_CSID, GL_COMPUTE_SHADER, l_rhs->m_ShaderFilePaths.m_CSPath);
	}

	l_rhs->m_objectStatus = ObjectStatus::Activated;

	return true;
}

bool GLRenderingServer::InitializeGPUBufferDataComponent(GPUBufferDataComponent * rhs)
{
	auto l_rhs = reinterpret_cast<GLGPUBufferDataComponent*>(rhs);

	l_rhs->m_TotalSize = l_rhs->m_ElementCount * l_rhs->m_ElementSize;

	if (l_rhs->m_GPUBufferAccessibility == GPUBufferAccessibility::ReadOnly)
	{
		l_rhs->m_BufferType = GL_UNIFORM_BUFFER;
	}
	else
	{
		l_rhs->m_BufferType = GL_SHADER_STORAGE_BUFFER;
	}

	glGenBuffers(1, &l_rhs->m_Handle);
	glBindBuffer(l_rhs->m_BufferType, l_rhs->m_Handle);
	glBufferData(l_rhs->m_BufferType, l_rhs->m_TotalSize, l_rhs->m_InitialData, GL_DYNAMIC_DRAW);
	glBindBufferRange(l_rhs->m_BufferType, (GLuint)l_rhs->m_BindingPoint, l_rhs->m_Handle, 0, l_rhs->m_TotalSize);

#ifdef _DEBUG
	auto l_GPUBufferName = std::string(l_rhs->m_componentName.c_str());
	if (l_rhs->m_GPUBufferAccessibility == GPUBufferAccessibility::ReadOnly)
	{
		l_GPUBufferName += "_UBO";
	}
	else
	{
		l_GPUBufferName += "_SSBO";
	}
	glObjectLabel(GL_BUFFER, l_rhs->m_Handle, (GLsizei)l_GPUBufferName.size(), l_GPUBufferName.c_str());
#endif

	glBindBuffer(l_rhs->m_BufferType, 0);

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

bool GLRenderingServer::DeleteGPUBufferDataComponent(GPUBufferDataComponent * rhs)
{
	return false;
}

bool GLRenderingServer::UploadGPUBufferDataComponentImpl(GPUBufferDataComponent * rhs, const void * GPUBufferValue)
{
	auto l_rhs = reinterpret_cast<GLGPUBufferDataComponent*>(rhs);

	glBindBuffer(l_rhs->m_BufferType, l_rhs->m_Handle);
	glBufferSubData(l_rhs->m_BufferType, 0, l_rhs->m_TotalSize, GPUBufferValue);

	return true;
}

bool GLRenderingServer::CommandListBegin(RenderPassDataComponent * rhs, size_t frameIndex)
{
	auto l_rhs = reinterpret_cast<GLRenderPassDataComponent*>(rhs);
	auto l_GLPSO = reinterpret_cast<GLPipelineStateObject*>(l_rhs->m_PipelineStateObject);

	for (size_t i = 0; i < l_GLPSO->m_Activate.size(); i++)
	{
		l_GLPSO->m_Activate[i]();
	}

	return true;
}

bool GLRenderingServer::BindRenderPassDataComponent(RenderPassDataComponent * rhs)
{
	auto l_rhs = reinterpret_cast<GLRenderPassDataComponent*>(rhs);
	auto l_GLPSO = reinterpret_cast<GLPipelineStateObject*>(l_rhs->m_PipelineStateObject);

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, l_rhs->m_FBO);
	if (l_rhs->m_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_UseDepthBuffer)
	{
		glBindRenderbuffer(GL_RENDERBUFFER, l_rhs->m_RBO);
		glRenderbufferStorage(GL_RENDERBUFFER, l_rhs->m_renderBufferInternalFormat, (GLsizei)l_rhs->m_RenderPassDesc.m_GraphicsPipelineDesc.m_ViewportDesc.m_Width, (GLsizei)l_rhs->m_RenderPassDesc.m_GraphicsPipelineDesc.m_ViewportDesc.m_Height);
	}

	return true;
}

bool GLRenderingServer::CleanRenderTargets(RenderPassDataComponent * rhs)
{
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	glClear(GL_DEPTH_BUFFER_BIT);
	glClear(GL_STENCIL_BUFFER_BIT);

	return true;
}

bool GLRenderingServer::ActivateResourceBinder(RenderPassDataComponent * renderPass, ShaderType shaderType, IResourceBinder * binder, size_t bindingSlot)
{
	auto l_binder = reinterpret_cast<GLResourceBinder*>(binder);

	switch (l_binder->m_ResourceBinderType)
	{
	case ResourceBinderType::Sampler:
		break;
	case ResourceBinderType::Image:
		for (size_t i = 0; i < l_binder->m_Resources.size(); i++)
		{
			ActivateTexture(reinterpret_cast<GLTextureDataComponent*>(l_binder->m_Resources[i]), (int)i);
		}
		break;
	case ResourceBinderType::ROBuffer:
		break;
	case ResourceBinderType::ROBufferArray:
		break;
	case ResourceBinderType::RWBuffer:
		break;
	case ResourceBinderType::RWBufferArray:
		break;
	default:
		break;
	}

	return true;
}

bool GLRenderingServer::BindGPUBufferDataComponent(RenderPassDataComponent * renderPass, GPUBufferDataComponent * GPUBuffer, ShaderType shaderType, GPUBufferAccessibility accessibility, size_t startOffset, size_t range)
{
	auto l_GPUBuffer = reinterpret_cast<GLGPUBufferDataComponent*>(GPUBuffer);

	glBindBufferRange(l_GPUBuffer->m_BufferType, (GLuint)l_GPUBuffer->m_BindingPoint, l_GPUBuffer->m_Handle, startOffset * l_GPUBuffer->m_ElementSize, range);

	return true;
}

bool GLRenderingServer::BindShaderProgramComponent(ShaderProgramComponent * rhs)
{
	auto l_rhs = reinterpret_cast<GLShaderProgramComponent*>(rhs);

	glUseProgram(l_rhs->m_ProgramID);

	return true;
}

bool GLRenderingServer::BindMaterialDataComponent(RenderPassDataComponent * renderPass, ShaderType shaderType, MaterialDataComponent * rhs)
{
	if (rhs->m_normalTexture)
	{
		ActivateTexture(reinterpret_cast<GLTextureDataComponent*>(rhs->m_normalTexture), 0);
	}
	if (rhs->m_albedoTexture)
	{
		ActivateTexture(reinterpret_cast<GLTextureDataComponent*>(rhs->m_albedoTexture), 1);
	}
	if (rhs->m_metallicTexture)
	{
		ActivateTexture(reinterpret_cast<GLTextureDataComponent*>(rhs->m_metallicTexture), 2);
	}
	if (rhs->m_roughnessTexture)
	{
		ActivateTexture(reinterpret_cast<GLTextureDataComponent*>(rhs->m_roughnessTexture), 3);
	}
	if (rhs->m_aoTexture)
	{
		ActivateTexture(reinterpret_cast<GLTextureDataComponent*>(rhs->m_aoTexture), 4);
	}

	return true;
}

bool GLRenderingServer::DispatchDrawCall(RenderPassDataComponent* renderPass, MeshDataComponent * mesh)
{
	auto l_GLPSO = reinterpret_cast<GLPipelineStateObject*>(renderPass->m_PipelineStateObject);
	auto l_mesh = reinterpret_cast<GLMeshDataComponent*>(mesh);

	glBindVertexArray(l_mesh->m_VAO);
	glDrawElements(l_GLPSO->m_GLPrimitiveTopology, (GLsizei)l_mesh->m_indicesSize, GL_UNSIGNED_INT, 0);

	return true;
}

bool GLRenderingServer::DeactivateResourceBinder(RenderPassDataComponent * renderPass, ShaderType shaderType, IResourceBinder * binder, size_t bindingSlot)
{
	return true;
}

bool GLRenderingServer::UnbindMaterialDataComponent(RenderPassDataComponent * renderPass, ShaderType shaderType, MaterialDataComponent * rhs)
{
	return true;
}

bool GLRenderingServer::CommandListEnd(RenderPassDataComponent * rhs)
{
	auto l_rhs = reinterpret_cast<GLRenderPassDataComponent*>(rhs);
	auto l_GLPSO = reinterpret_cast<GLPipelineStateObject*>(l_rhs->m_PipelineStateObject);

	for (size_t i = 0; i < l_GLPSO->m_Deactivate.size(); i++)
	{
		l_GLPSO->m_Deactivate[i]();
	}

	return true;
}

bool GLRenderingServer::ExecuteCommandList(RenderPassDataComponent * rhs)
{
	return true;
}

bool GLRenderingServer::WaitForFrame(RenderPassDataComponent * rhs)
{
	return true;
}

RenderPassDataComponent * GLRenderingServer::GetSwapChainRPDC()
{
	return m_SwapChainRPDC;
}

bool GLRenderingServer::Present()
{
	g_pModuleManager->getWindowSystem()->getWindowSurface()->swapBuffer();

	return true;
}

bool GLRenderingServer::CopyDepthBuffer(RenderPassDataComponent * src, RenderPassDataComponent * dest)
{
	auto l_src = reinterpret_cast<GLRenderPassDataComponent*>(src);
	auto l_dest = reinterpret_cast<GLRenderPassDataComponent*>(dest);

	glBindFramebuffer(GL_READ_FRAMEBUFFER, l_src->m_FBO);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, l_dest->m_FBO);
	glBlitFramebuffer(0, 0,
		l_src->m_RenderPassDesc.m_RenderTargetDesc.width, l_src->m_RenderPassDesc.m_RenderTargetDesc.height,
		0, 0,
		l_dest->m_RenderPassDesc.m_RenderTargetDesc.width, l_dest->m_RenderPassDesc.m_RenderTargetDesc.height,
		GL_DEPTH_BUFFER_BIT, GL_NEAREST);

	return true;
}

bool GLRenderingServer::CopyStencilBuffer(RenderPassDataComponent * src, RenderPassDataComponent * dest)
{
	auto l_src = reinterpret_cast<GLRenderPassDataComponent*>(src);
	auto l_dest = reinterpret_cast<GLRenderPassDataComponent*>(dest);

	glBindFramebuffer(GL_READ_FRAMEBUFFER, l_src->m_FBO);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, l_dest->m_FBO);
	glBlitFramebuffer(0, 0,
		l_src->m_RenderPassDesc.m_RenderTargetDesc.width, l_src->m_RenderPassDesc.m_RenderTargetDesc.height,
		0, 0,
		l_dest->m_RenderPassDesc.m_RenderTargetDesc.width, l_dest->m_RenderPassDesc.m_RenderTargetDesc.height,
		GL_STENCIL_BUFFER_BIT, GL_NEAREST);

	return true;
}

bool GLRenderingServer::CopyColorBuffer(RenderPassDataComponent * src, size_t srcIndex, RenderPassDataComponent * dest, size_t destIndex)
{
	auto l_src = reinterpret_cast<GLRenderPassDataComponent*>(src);
	auto l_dest = reinterpret_cast<GLRenderPassDataComponent*>(dest);

	glBindFramebuffer(GL_READ_FRAMEBUFFER, l_src->m_FBO);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, l_dest->m_FBO);
	glReadBuffer(GL_COLOR_ATTACHMENT0 + (unsigned int)srcIndex);
	glDrawBuffer(GL_COLOR_ATTACHMENT0 + (unsigned int)destIndex);

	glBlitFramebuffer(0, 0,
		l_src->m_RenderPassDesc.m_RenderTargetDesc.width, l_src->m_RenderPassDesc.m_RenderTargetDesc.height,
		0, 0,
		l_dest->m_RenderPassDesc.m_RenderTargetDesc.width, l_dest->m_RenderPassDesc.m_RenderTargetDesc.height,
		GL_COLOR_BUFFER_BIT, GL_LINEAR);

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