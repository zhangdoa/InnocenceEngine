#include "GLRenderingServer.h"
#include "../../Component/GLMeshDataComponent.h"
#include "../../Component/GLTextureDataComponent.h"
#include "../../Component/GLMaterialDataComponent.h"
#include "../../Component/GLRenderPassDataComponent.h"
#include "../../Component/GLShaderProgramComponent.h"
#include "../../Component/GLSamplerDataComponent.h"
#include "../../Component/GLGPUBufferDataComponent.h"

#include "GLHelper.h"

using namespace GLHelper;

#include "../CommonFunctionDefinationMacro.inl"

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

	GLResourceBinder* addResourcesBinder();
	GLPipelineStateObject* addPSO();

	bool resizeImpl();

	ObjectStatus m_ObjectStatus = ObjectStatus::Terminated;

	IObjectPool* m_MeshDataComponentPool = 0;
	IObjectPool* m_MaterialDataComponentPool = 0;
	IObjectPool* m_TextureDataComponentPool = 0;
	IObjectPool* m_RenderPassDataComponentPool = 0;
	IObjectPool* m_ResourcesBinderPool = 0;
	IObjectPool* m_PSOPool = 0;
	IObjectPool* m_ShaderProgramComponentPool = 0;
	IObjectPool* m_SamplerDataComponentPool = 0;
	IObjectPool* m_GPUBufferDataComponentPool = 0;

	std::unordered_set<MeshDataComponent*> m_initializedMeshes;
	std::unordered_set<TextureDataComponent*> m_initializedTextures;
	std::unordered_set<MaterialDataComponent*> m_initializedMaterials;
	std::vector<GLRenderPassDataComponent*> m_RPDCs;

	GLRenderPassDataComponent* m_userPipelineOutput = 0;
	GLRenderPassDataComponent* m_SwapChainRPDC = 0;
	GLShaderProgramComponent* m_SwapChainSPC = 0;
	GLSamplerDataComponent* m_SwapChainSDC = 0;

	std::atomic_bool m_needResize = false;
}

GLResourceBinder* GLRenderingServerNS::addResourcesBinder()
{
	auto l_BinderRawPtr = m_ResourcesBinderPool->Spawn();
	auto l_Binder = new(l_BinderRawPtr)GLResourceBinder();
	return l_Binder;
}

GLPipelineStateObject* GLRenderingServerNS::addPSO()
{
	auto l_PSORawPtr = m_PSOPool->Spawn();
	auto l_PSO = new(l_PSORawPtr)GLPipelineStateObject();
	return l_PSO;
}

bool GLRenderingServerNS::resizeImpl()
{
	auto l_renderingServer = reinterpret_cast<GLRenderingServer*>(g_pModuleManager->getRenderingServer());
	auto l_screenResolution = g_pModuleManager->getRenderingFrontend()->getScreenResolution();

	for (auto i : m_RPDCs)
	{
		if (!i->m_RenderPassDesc.m_IsOffScreen)
		{
			if (i->m_RBO)
			{
				glDeleteRenderbuffers(1, &i->m_RBO);
			}
			glDeleteFramebuffers(1, &i->m_FBO);

			for (auto j : i->m_RenderTargets)
			{
				l_renderingServer->DeleteTextureDataComponent(j);
			}

			i->m_RenderTargets.clear();

			i->m_RenderTargetsResourceBinders.clear();

			if (i->m_DepthStencilRenderTarget)
			{
				l_renderingServer->DeleteTextureDataComponent(i->m_DepthStencilRenderTarget);
			}

			m_PSOPool->Destroy(i->m_PipelineStateObject);

			i->m_RenderPassDesc.m_RenderTargetDesc.Width = l_screenResolution.x;
			i->m_RenderPassDesc.m_RenderTargetDesc.Height = l_screenResolution.y;

			i->m_RenderPassDesc.m_GraphicsPipelineDesc.m_ViewportDesc.m_Width = (float)l_screenResolution.x;
			i->m_RenderPassDesc.m_GraphicsPipelineDesc.m_ViewportDesc.m_Height = (float)l_screenResolution.y;

			CreateFramebuffer(i);

			ReserveRenderTargets(i, l_renderingServer);

			CreateRenderTargets(i, l_renderingServer);

			i->m_RenderTargetsResourceBinders.resize(i->m_RenderPassDesc.m_RenderTargetCount);

			CreateResourcesBinder(i);

			i->m_PipelineStateObject = addPSO();

			CreateStateObjects(i);
		}

		m_PSOPool->Destroy(m_SwapChainRPDC->m_PipelineStateObject);

		m_SwapChainRPDC->m_RenderPassDesc.m_RenderTargetDesc.Width = l_screenResolution.x;
		m_SwapChainRPDC->m_RenderPassDesc.m_RenderTargetDesc.Height = l_screenResolution.y;

		m_SwapChainRPDC->m_RenderPassDesc.m_GraphicsPipelineDesc.m_ViewportDesc.m_Width = (float)l_screenResolution.x;
		m_SwapChainRPDC->m_RenderPassDesc.m_GraphicsPipelineDesc.m_ViewportDesc.m_Height = (float)l_screenResolution.y;

		m_SwapChainRPDC->m_PipelineStateObject = addPSO();

		CreateStateObjects(m_SwapChainRPDC);
	}

	return true;
}

using namespace GLRenderingServerNS;

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
	m_SamplerDataComponentPool = InnoMemory::CreateObjectPool(sizeof(GLSamplerDataComponent), 256);
	m_GPUBufferDataComponentPool = InnoMemory::CreateObjectPool(sizeof(GLGPUBufferDataComponent), 256);

	m_RPDCs.reserve(128);

	auto l_GLRenderingServerSetupTask = g_pModuleManager->getTaskSystem()->submit("GLRenderingServerSetupTask", 2, nullptr,
		[&]() {
		glEnable(GL_DEBUG_OUTPUT);
		glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
		glDebugMessageCallback(MessageCallback, 0);

		glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
		glEnable(GL_PROGRAM_POINT_SIZE);
	}
	);

	l_GLRenderingServerSetupTask->Wait();

	m_SwapChainRPDC = reinterpret_cast<GLRenderPassDataComponent*>(AddRenderPassDataComponent("SwapChain/"));
	m_SwapChainSPC = reinterpret_cast<GLShaderProgramComponent*>(AddShaderProgramComponent("SwapChain/"));
	m_SwapChainSDC = reinterpret_cast<GLSamplerDataComponent*>(AddSamplerDataComponent("SwapChain/"));

	m_ObjectStatus = ObjectStatus::Created;
	InnoLogger::Log(LogLevel::Success, "GLRenderingServer setup finished.");

	return true;
}

bool GLRenderingServer::Initialize()
{
	if (m_ObjectStatus == ObjectStatus::Created)
	{
		m_SwapChainSPC->m_ShaderFilePaths.m_VSPath = "2DImageProcess.vert/";
		m_SwapChainSPC->m_ShaderFilePaths.m_PSPath = "swapChain.frag/";

		auto l_GLRenderingServerInitializeTask = g_pModuleManager->getTaskSystem()->submit("GLRenderingServerInitializeTask", 2, nullptr,
			[&]() {
			InitializeShaderProgramComponent(m_SwapChainSPC);

			InitializeSamplerDataComponent(m_SwapChainSDC);

			auto l_RenderPassDesc = g_pModuleManager->getRenderingFrontend()->getDefaultRenderPassDesc();

			l_RenderPassDesc.m_RenderTargetCount = 1;

			m_SwapChainRPDC->m_RenderPassDesc = l_RenderPassDesc;
			m_SwapChainRPDC->m_RenderPassDesc.m_RenderTargetDesc.PixelDataType = TexturePixelDataType::UBYTE;
			m_SwapChainRPDC->m_RenderPassDesc.m_GraphicsPipelineDesc.m_RasterizerDesc.m_UseCulling = false;

			m_SwapChainRPDC->m_ResourceBinderLayoutDescs.resize(2);
			m_SwapChainRPDC->m_ResourceBinderLayoutDescs[0].m_ResourceBinderType = ResourceBinderType::Image;
			m_SwapChainRPDC->m_ResourceBinderLayoutDescs[0].m_DescriptorSetIndex = 0;
			m_SwapChainRPDC->m_ResourceBinderLayoutDescs[0].m_DescriptorIndex = 0;
			m_SwapChainRPDC->m_ResourceBinderLayoutDescs[0].m_IndirectBinding = true;

			m_SwapChainRPDC->m_ResourceBinderLayoutDescs[1].m_ResourceBinderType = ResourceBinderType::Sampler;
			m_SwapChainRPDC->m_ResourceBinderLayoutDescs[1].m_DescriptorSetIndex = 1;
			m_SwapChainRPDC->m_ResourceBinderLayoutDescs[1].m_DescriptorIndex = 0;
			m_SwapChainRPDC->m_ResourceBinderLayoutDescs[1].m_IndirectBinding = true;

			m_SwapChainRPDC->m_ShaderProgram = m_SwapChainSPC;
			m_SwapChainRPDC->m_FBO = 0;
			m_SwapChainRPDC->m_RBO = 0;

			m_SwapChainRPDC->m_PipelineStateObject = addPSO();

			CreateStateObjects(m_SwapChainRPDC);

			m_SwapChainRPDC->m_ObjectStatus = ObjectStatus::Activated;
		});

		l_GLRenderingServerInitializeTask->Wait();
	}

	return true;
}

bool GLRenderingServer::Terminate()
{
	m_ObjectStatus = ObjectStatus::Terminated;
	InnoLogger::Log(LogLevel::Success, "GLRenderingServer has been terminated.");

	return true;
}

ObjectStatus GLRenderingServer::GetStatus()
{
	return m_ObjectStatus;
}

AddComponent(GL, MeshData);
AddComponent(GL, TextureData);
AddComponent(GL, MaterialData);
AddComponent(GL, RenderPassData);
AddComponent(GL, ShaderProgram);
AddComponent(GL, SamplerData);
AddComponent(GL, GPUBufferData);

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
	auto l_VAOName = std::string(l_rhs->m_ComponentName.c_str());
	l_VAOName += "_VAO";
	glObjectLabel(GL_VERTEX_ARRAY, l_rhs->m_VAO, (GLsizei)l_VAOName.size(), l_VAOName.c_str());
	auto l_VBOName = std::string(l_rhs->m_ComponentName.c_str());
	l_VBOName += "_VBO";
	glObjectLabel(GL_BUFFER, l_rhs->m_VBO, (GLsizei)l_VBOName.size(), l_VBOName.c_str());
	auto l_IBOName = std::string(l_rhs->m_ComponentName.c_str());
	l_IBOName += "_IBO";
	glObjectLabel(GL_BUFFER, l_rhs->m_IBO, (GLsizei)l_IBOName.size(), l_IBOName.c_str());
#endif

	// position Vec4
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);

	// texture coordinate Vec2
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)16);

	// pad1 Vec2
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)24);

	// normal Vec4
	glEnableVertexAttribArray(3);
	glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)32);

	// pad2 Vec4
	glEnableVertexAttribArray(4);
	glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)48);

	InnoLogger::Log(LogLevel::Verbose, "GLRenderingServer: VAO ", l_rhs->m_VAO, " is initialized.");

	glBufferData(GL_ARRAY_BUFFER, l_rhs->m_vertices.size() * sizeof(Vertex), &l_rhs->m_vertices[0], GL_STATIC_DRAW);

	InnoLogger::Log(LogLevel::Verbose, "GLRenderingServer: VBO ", l_rhs->m_VBO, " is initialized.");

	glBufferData(GL_ELEMENT_ARRAY_BUFFER, l_rhs->m_indices.size() * sizeof(Index), &l_rhs->m_indices[0], GL_STATIC_DRAW);

	InnoLogger::Log(LogLevel::Verbose, "GLRenderingServer: IBO ", l_rhs->m_IBO, " is initialized.");

	l_rhs->m_ObjectStatus = ObjectStatus::Activated;

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

	l_rhs->m_GLTextureDesc = GetGLTextureDesc(l_rhs->m_TextureDesc);

	glGenTextures(1, &l_rhs->m_TO);

	glBindTexture(l_rhs->m_GLTextureDesc.TextureSamplerType, l_rhs->m_TO);

#ifdef _DEBUG
	auto l_TOName = std::string(l_rhs->m_ComponentName.c_str());
	l_TOName += "_TO";
	glObjectLabel(GL_TEXTURE, l_rhs->m_TO, (GLsizei)l_TOName.size(), l_TOName.c_str());
#endif

	glTexParameterfv(l_rhs->m_GLTextureDesc.TextureSamplerType, GL_TEXTURE_BORDER_COLOR, l_rhs->m_GLTextureDesc.BorderColor);

	if (l_rhs->m_GLTextureDesc.TextureSamplerType == GL_TEXTURE_1D)
	{
		glTexImage1D(GL_TEXTURE_1D, 0, l_rhs->m_GLTextureDesc.InternalFormat, l_rhs->m_GLTextureDesc.Width, 0, l_rhs->m_GLTextureDesc.PixelDataFormat, l_rhs->m_GLTextureDesc.PixelDataType, l_rhs->m_TextureData);
	}
	else if (l_rhs->m_GLTextureDesc.TextureSamplerType == GL_TEXTURE_2D)
	{
		glTexImage2D(GL_TEXTURE_2D, 0, l_rhs->m_GLTextureDesc.InternalFormat, l_rhs->m_GLTextureDesc.Width, l_rhs->m_GLTextureDesc.Height, 0, l_rhs->m_GLTextureDesc.PixelDataFormat, l_rhs->m_GLTextureDesc.PixelDataType, l_rhs->m_TextureData);
	}
	else if (l_rhs->m_GLTextureDesc.TextureSamplerType == GL_TEXTURE_3D)
	{
		glTexImage3D(GL_TEXTURE_3D, 0, l_rhs->m_GLTextureDesc.InternalFormat, l_rhs->m_GLTextureDesc.Width, l_rhs->m_GLTextureDesc.Height, l_rhs->m_GLTextureDesc.DepthOrArraySize, 0, l_rhs->m_GLTextureDesc.PixelDataFormat, l_rhs->m_GLTextureDesc.PixelDataType, l_rhs->m_TextureData);
	}
	else if (l_rhs->m_GLTextureDesc.TextureSamplerType == GL_TEXTURE_1D_ARRAY)
	{
		glTexImage2D(GL_TEXTURE_1D_ARRAY, 0, l_rhs->m_GLTextureDesc.InternalFormat, l_rhs->m_GLTextureDesc.Width, l_rhs->m_GLTextureDesc.Height, 0, l_rhs->m_GLTextureDesc.PixelDataFormat, l_rhs->m_GLTextureDesc.PixelDataType, l_rhs->m_TextureData);
	}
	else if (l_rhs->m_GLTextureDesc.TextureSamplerType == GL_TEXTURE_2D_ARRAY)
	{
		glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, l_rhs->m_GLTextureDesc.InternalFormat, l_rhs->m_GLTextureDesc.Width, l_rhs->m_GLTextureDesc.Height, l_rhs->m_GLTextureDesc.DepthOrArraySize, 0, l_rhs->m_GLTextureDesc.PixelDataFormat, l_rhs->m_GLTextureDesc.PixelDataType, l_rhs->m_TextureData);
	}
	else if (l_rhs->m_GLTextureDesc.TextureSamplerType == GL_TEXTURE_CUBE_MAP)
	{
		if (l_rhs->m_TextureData)
		{
			for (uint32_t i = 0; i < 6; i++)
			{
				char* l_textureData = reinterpret_cast<char*>(const_cast<void*>(l_rhs->m_TextureData));
				auto l_offset = i * l_rhs->m_GLTextureDesc.Width * l_rhs->m_GLTextureDesc.Height * l_rhs->m_GLTextureDesc.PixelDataSize;
				glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, l_rhs->m_GLTextureDesc.InternalFormat, l_rhs->m_GLTextureDesc.Width, l_rhs->m_GLTextureDesc.Height, 0, l_rhs->m_GLTextureDesc.PixelDataFormat, l_rhs->m_GLTextureDesc.PixelDataType, l_textureData + l_offset);
			}
		}
		else
		{
			for (uint32_t i = 0; i < 6; i++)
			{
				glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, l_rhs->m_GLTextureDesc.InternalFormat, l_rhs->m_GLTextureDesc.Width, l_rhs->m_GLTextureDesc.Height, 0, l_rhs->m_GLTextureDesc.PixelDataFormat, l_rhs->m_GLTextureDesc.PixelDataType, l_rhs->m_TextureData);
			}
		}
	}

	// should generate mipmap or not
	if (l_rhs->m_TextureDesc.UseMipMap)
	{
		glGenerateMipmap(l_rhs->m_GLTextureDesc.TextureSamplerType);
	}

	InnoLogger::Log(LogLevel::Verbose, "GLRenderingServer: TO ", l_rhs->m_TO, " is initialized.");

	auto l_resourceBinder = addResourcesBinder();
	l_resourceBinder->m_TO = l_rhs;
	l_resourceBinder->m_ResourceBinderType = ResourceBinderType::Image;
	l_rhs->m_ResourceBinder = l_resourceBinder;

	l_rhs->m_ObjectStatus = ObjectStatus::Activated;

	m_initializedTextures.emplace(l_rhs);

	return true;
}

bool GLRenderingServer::InitializeMaterialDataComponent(MaterialDataComponent * rhs)
{
	if (m_initializedMaterials.find(rhs) != m_initializedMaterials.end())
	{
		return true;
	}

	auto l_rhs = reinterpret_cast<GLMaterialDataComponent*>(rhs);

	auto l_defaultMaterial = g_pModuleManager->getRenderingFrontend()->getDefaultMaterialDataComponent();

	for (size_t i = 0; i < 8; i++)
	{
		auto l_texture = reinterpret_cast<GLTextureDataComponent*>(l_rhs->m_TextureSlots[i].m_Texture);

		if (l_texture)
		{
			InitializeTextureDataComponent(l_texture);
			l_rhs->m_TextureSlots[i].m_Texture = l_texture;
			l_rhs->m_TextureSlots[i].m_Activate = true;
		}
		else
		{
			l_rhs->m_TextureSlots[i].m_Texture = l_defaultMaterial->m_TextureSlots[i].m_Texture;
		}
	}

	l_rhs->m_ObjectStatus = ObjectStatus::Activated;

	m_initializedMaterials.emplace(l_rhs);

	return true;
}

bool GLRenderingServer::InitializeRenderPassDataComponent(RenderPassDataComponent * rhs)
{
	auto l_rhs = reinterpret_cast<GLRenderPassDataComponent*>(rhs);

	CreateFramebuffer(l_rhs);

	ReserveRenderTargets(l_rhs, this);

	CreateRenderTargets(l_rhs, this);

	l_rhs->m_RenderTargetsResourceBinders.resize(l_rhs->m_RenderPassDesc.m_RenderTargetCount);

	CreateResourcesBinder(l_rhs);

	l_rhs->m_PipelineStateObject = addPSO();

	CreateStateObjects(l_rhs);

	m_RPDCs.emplace_back(l_rhs);

	l_rhs->m_ObjectStatus = ObjectStatus::Activated;

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
		AddShaderObject(l_rhs->m_VSID, GL_VERTEX_SHADER, l_rhs->m_ShaderFilePaths.m_VSPath);
		glAttachShader(l_rhs->m_ProgramID, l_rhs->m_VSID);
	}
	if (l_rhs->m_ShaderFilePaths.m_HSPath != "")
	{
		AddShaderObject(l_rhs->m_TCSID, GL_TESS_CONTROL_SHADER, l_rhs->m_ShaderFilePaths.m_HSPath);
		glAttachShader(l_rhs->m_ProgramID, l_rhs->m_TCSID);
	}
	if (l_rhs->m_ShaderFilePaths.m_DSPath != "")
	{
		AddShaderObject(l_rhs->m_TESID, GL_TESS_EVALUATION_SHADER, l_rhs->m_ShaderFilePaths.m_DSPath);
		glAttachShader(l_rhs->m_ProgramID, l_rhs->m_TESID);
	}
	if (l_rhs->m_ShaderFilePaths.m_GSPath != "")
	{
		AddShaderObject(l_rhs->m_GSID, GL_GEOMETRY_SHADER, l_rhs->m_ShaderFilePaths.m_GSPath);
		glAttachShader(l_rhs->m_ProgramID, l_rhs->m_GSID);
	}
	if (l_rhs->m_ShaderFilePaths.m_PSPath != "")
	{
		AddShaderObject(l_rhs->m_FSID, GL_FRAGMENT_SHADER, l_rhs->m_ShaderFilePaths.m_PSPath);
		glAttachShader(l_rhs->m_ProgramID, l_rhs->m_FSID);
	}
	if (l_rhs->m_ShaderFilePaths.m_CSPath != "")
	{
		AddShaderObject(l_rhs->m_CSID, GL_COMPUTE_SHADER, l_rhs->m_ShaderFilePaths.m_CSPath);
		glAttachShader(l_rhs->m_ProgramID, l_rhs->m_CSID);
	}

	LinkProgramObject(l_rhs->m_ProgramID);

	l_rhs->m_ObjectStatus = ObjectStatus::Activated;

	return true;
}

bool GLRenderingServer::InitializeSamplerDataComponent(SamplerDataComponent * rhs)
{
	auto l_rhs = reinterpret_cast<GLSamplerDataComponent*>(rhs);

	glCreateSamplers(1, &l_rhs->m_SO);

	auto l_textureWrapMethodU = GetTextureWrapMethod(l_rhs->m_SamplerDesc.m_WrapMethodU);
	auto l_textureWrapMethodV = GetTextureWrapMethod(l_rhs->m_SamplerDesc.m_WrapMethodV);
	auto l_textureWrapMethodW = GetTextureWrapMethod(l_rhs->m_SamplerDesc.m_WrapMethodW);
	auto l_minFilterParam = GetTextureFilterParam(l_rhs->m_SamplerDesc.m_MinFilterMethod, l_rhs->m_SamplerDesc.m_UseMipMap);
	auto l_magFilterParam = GetTextureFilterParam(l_rhs->m_SamplerDesc.m_MagFilterMethod, false);

	glSamplerParameteri(l_rhs->m_SO, GL_TEXTURE_WRAP_R, l_textureWrapMethodU);
	glSamplerParameteri(l_rhs->m_SO, GL_TEXTURE_WRAP_S, l_textureWrapMethodV);
	glSamplerParameteri(l_rhs->m_SO, GL_TEXTURE_WRAP_T, l_textureWrapMethodW);
	glSamplerParameteri(l_rhs->m_SO, GL_TEXTURE_MIN_FILTER, l_minFilterParam);
	glSamplerParameteri(l_rhs->m_SO, GL_TEXTURE_MAG_FILTER, l_magFilterParam);
	glSamplerParameterfv(l_rhs->m_SO, GL_TEXTURE_BORDER_COLOR, l_rhs->m_SamplerDesc.m_BorderColor);

	auto l_resourceBinder = addResourcesBinder();
	l_resourceBinder->m_SO = l_rhs->m_SO;
	l_resourceBinder->m_ResourceBinderType = ResourceBinderType::Sampler;

	l_rhs->m_ResourceBinder = l_resourceBinder;

	l_rhs->m_ObjectStatus = ObjectStatus::Activated;

	return true;
}

bool GLRenderingServer::InitializeGPUBufferDataComponent(GPUBufferDataComponent * rhs)
{
	auto l_rhs = reinterpret_cast<GLGPUBufferDataComponent*>(rhs);

	l_rhs->m_TotalSize = l_rhs->m_ElementCount * l_rhs->m_ElementSize;

	auto l_resourceBinder = addResourcesBinder();
	l_resourceBinder->m_ResourceBinderType = ResourceBinderType::Buffer;
	l_resourceBinder->m_GPUAccessibility = l_rhs->m_GPUAccessibility;
	l_resourceBinder->m_ElementCount = l_rhs->m_ElementCount;
	l_resourceBinder->m_ElementSize = l_rhs->m_ElementSize;
	l_resourceBinder->m_TotalSize = l_rhs->m_TotalSize;

	if (l_rhs->m_GPUAccessibility == Accessibility::ReadOnly)
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
	auto l_GPUBufferName = std::string(l_rhs->m_ComponentName.c_str());
	if (l_rhs->m_GPUAccessibility == Accessibility::ReadOnly)
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

	l_resourceBinder->m_BO = l_rhs->m_Handle;

	l_rhs->m_ResourceBinder = l_resourceBinder;

	l_rhs->m_ObjectStatus = ObjectStatus::Activated;

	return true;
}

bool GLRenderingServer::DeleteMeshDataComponent(MeshDataComponent * rhs)
{
	auto l_rhs = reinterpret_cast<GLMeshDataComponent*>(rhs);

	glDeleteVertexArrays(1, &l_rhs->m_VAO);
	glDeleteBuffers(1, &l_rhs->m_VBO);
	glDeleteBuffers(1, &l_rhs->m_IBO);

	m_MeshDataComponentPool->Destroy(l_rhs);

	m_initializedMeshes.erase(l_rhs);

	return true;
}

bool GLRenderingServer::DeleteTextureDataComponent(TextureDataComponent * rhs)
{
	auto l_rhs = reinterpret_cast<GLTextureDataComponent*>(rhs);

	glDeleteTextures(1, &l_rhs->m_TO);

	if (l_rhs->m_ResourceBinder)
	{
		m_ResourcesBinderPool->Destroy(l_rhs->m_ResourceBinder);
	}

	m_TextureDataComponentPool->Destroy(l_rhs);

	m_initializedTextures.erase(l_rhs);

	return true;
}

bool GLRenderingServer::DeleteMaterialDataComponent(MaterialDataComponent * rhs)
{
	auto l_rhs = reinterpret_cast<GLMaterialDataComponent*>(rhs);

	m_MaterialDataComponentPool->Destroy(l_rhs);

	m_initializedMaterials.erase(l_rhs);

	return true;
}

bool GLRenderingServer::DeleteRenderPassDataComponent(RenderPassDataComponent * rhs)
{
	auto l_rhs = reinterpret_cast<GLRenderPassDataComponent*>(rhs);
	auto l_PSO = reinterpret_cast<GLPipelineStateObject*>(l_rhs->m_PipelineStateObject);

	if (l_rhs->m_RBO)
	{
		glDeleteRenderbuffers(1, &l_rhs->m_RBO);
	}
	glDeleteFramebuffers(1, &l_rhs->m_FBO);

	if (l_rhs->m_DepthStencilRenderTarget)
	{
		DeleteTextureDataComponent(l_rhs->m_DepthStencilRenderTarget);
	}

	for (size_t i = 0; i < l_rhs->m_RenderTargets.size(); i++)
	{
		DeleteTextureDataComponent(l_rhs->m_RenderTargets[i]);
		m_ResourcesBinderPool->Destroy(l_rhs->m_RenderTargetsResourceBinders[i]);
	}

	m_RenderPassDataComponentPool->Destroy(l_rhs);

	return true;
}

bool GLRenderingServer::DeleteShaderProgramComponent(ShaderProgramComponent * rhs)
{
	auto l_rhs = reinterpret_cast<GLShaderProgramComponent*>(rhs);

	if (l_rhs->m_VSID)
	{
		glDeleteShader(l_rhs->m_VSID);
	}
	if (l_rhs->m_TCSID)
	{
		glDeleteShader(l_rhs->m_TCSID);
	}
	if (l_rhs->m_TESID)
	{
		glDeleteShader(l_rhs->m_TESID);
	}
	if (l_rhs->m_GSID)
	{
		glDeleteShader(l_rhs->m_GSID);
	}
	if (l_rhs->m_FSID)
	{
		glDeleteShader(l_rhs->m_FSID);
	}
	if (l_rhs->m_CSID)
	{
		glDeleteShader(l_rhs->m_CSID);
	}

	glDeleteProgram(l_rhs->m_ProgramID);

	m_ShaderProgramComponentPool->Destroy(l_rhs);

	return true;
}

bool GLRenderingServer::DeleteSamplerDataComponent(SamplerDataComponent * rhs)
{
	auto l_rhs = reinterpret_cast<GLSamplerDataComponent*>(rhs);

	glDeleteSamplers(1, &l_rhs->m_SO);

	if (l_rhs->m_ResourceBinder)
	{
		m_ResourcesBinderPool->Destroy(l_rhs->m_ResourceBinder);
	}

	m_SamplerDataComponentPool->Destroy(l_rhs);

	return true;
}

bool GLRenderingServer::DeleteGPUBufferDataComponent(GPUBufferDataComponent * rhs)
{
	auto l_rhs = reinterpret_cast<GLGPUBufferDataComponent*>(rhs);

	glDeleteBuffers(1, &l_rhs->m_Handle);

	if (l_rhs->m_ResourceBinder)
	{
		m_ResourcesBinderPool->Destroy(l_rhs->m_ResourceBinder);
	}

	m_GPUBufferDataComponentPool->Destroy(l_rhs);

	return true;
}

bool GLRenderingServer::UploadGPUBufferDataComponentImpl(GPUBufferDataComponent * rhs, const void * GPUBufferValue, size_t startOffset, size_t range)
{
	auto l_rhs = reinterpret_cast<GLGPUBufferDataComponent*>(rhs);

	glBindBuffer(l_rhs->m_BufferType, l_rhs->m_Handle);
	auto l_size = l_rhs->m_TotalSize;
	if (range != SIZE_MAX)
	{
		l_size = range * l_rhs->m_ElementSize;
	}
	glBufferSubData(l_rhs->m_BufferType, startOffset * l_rhs->m_ElementSize, l_size, GPUBufferValue);

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
	if (l_rhs->m_RenderPassDesc.m_RenderPassUsageType == RenderPassUsageType::Graphics)
	{
		auto l_GLPSO = reinterpret_cast<GLPipelineStateObject*>(l_rhs->m_PipelineStateObject);

		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, l_rhs->m_FBO);
		if (l_rhs->m_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_UseDepthBuffer)
		{
			glBindRenderbuffer(GL_RENDERBUFFER, l_rhs->m_RBO);
			glRenderbufferStorage(GL_RENDERBUFFER, l_rhs->m_renderBufferInternalFormat, (GLsizei)l_rhs->m_RenderPassDesc.m_GraphicsPipelineDesc.m_ViewportDesc.m_Width, (GLsizei)l_rhs->m_RenderPassDesc.m_GraphicsPipelineDesc.m_ViewportDesc.m_Height);
		}
	}

	auto l_shaderProgram = reinterpret_cast<GLShaderProgramComponent*>(l_rhs->m_ShaderProgram);

	glUseProgram(l_shaderProgram->m_ProgramID);

	return true;
}

bool GLRenderingServer::CleanRenderTargets(RenderPassDataComponent * rhs)
{
	if (rhs->m_RenderPassDesc.m_RenderPassUsageType == RenderPassUsageType::Graphics)
	{
		auto l_cleanColor = rhs->m_RenderPassDesc.m_GraphicsPipelineDesc.CleanColor;
		glClearColor(l_cleanColor[0], l_cleanColor[1], l_cleanColor[2], l_cleanColor[3]);
		glClear(GL_COLOR_BUFFER_BIT);
		glClear(GL_DEPTH_BUFFER_BIT);
		glClear(GL_STENCIL_BUFFER_BIT);
	}

	return true;
}

bool BindGPUBuffer(Accessibility accessibility, GLuint BO, size_t localSlot, size_t startOffset, size_t elementSize, size_t totalSize, size_t elementCount)
{
	GLenum l_bufferType;
	if (accessibility == Accessibility::ReadOnly)
	{
		l_bufferType = GL_UNIFORM_BUFFER;
	}
	else
	{
		l_bufferType = GL_SHADER_STORAGE_BUFFER;
	}

	GLsizei l_range;
	if (elementCount != SIZE_MAX)
	{
		l_range = (GLsizei)elementSize * (GLsizei)elementCount;
	}
	else
	{
		l_range = (GLsizei)totalSize;
	}

	glBindBufferRange(l_bufferType, (GLuint)localSlot, BO, startOffset * elementSize, l_range);

	return true;
}

bool GLRenderingServer::ActivateResourceBinder(RenderPassDataComponent * renderPass, ShaderStage shaderStage, IResourceBinder * binder, size_t globalSlot, size_t localSlot, Accessibility accessibility, size_t startOffset, size_t elementCount)
{
	auto l_resourceBinder = reinterpret_cast<GLResourceBinder*>(binder);

	if (l_resourceBinder)
	{
		switch (l_resourceBinder->m_ResourceBinderType)
		{
		case ResourceBinderType::Sampler:
			for (size_t i = 0; i < 16; i++)
			{
				glBindSampler((GLuint)i, l_resourceBinder->m_SO);
			}
			break;
		case ResourceBinderType::Image:
			if (accessibility == Accessibility::ReadOnly)
			{
				ActivateTexture(reinterpret_cast<GLTextureDataComponent*>(l_resourceBinder->m_TO), (uint32_t)localSlot);
			}
			else
			{
				BindTextureAsImage(reinterpret_cast<GLTextureDataComponent*>(l_resourceBinder->m_TO), (uint32_t)localSlot, accessibility);
			}
			break;
		case ResourceBinderType::Buffer:
			BindGPUBuffer(l_resourceBinder->m_GPUAccessibility, l_resourceBinder->m_BO, localSlot, startOffset, l_resourceBinder->m_ElementSize, l_resourceBinder->m_TotalSize, elementCount);
			break;
		default:
			break;
		}
	}

	return true;
}

bool GLRenderingServer::DispatchDrawCall(RenderPassDataComponent * renderPass, MeshDataComponent* mesh, size_t instanceCount)
{
	auto l_GLPSO = reinterpret_cast<GLPipelineStateObject*>(renderPass->m_PipelineStateObject);
	auto l_mesh = reinterpret_cast<GLMeshDataComponent*>(mesh);

	glBindVertexArray(l_mesh->m_VAO);
	glDrawElementsInstanced(l_GLPSO->m_GLPrimitiveTopology, (GLsizei)l_mesh->m_indicesSize, GL_UNSIGNED_INT, 0, (GLsizei)instanceCount);

	return true;
}

bool GLRenderingServer::DeactivateResourceBinder(RenderPassDataComponent * renderPass, ShaderStage shaderStage, IResourceBinder * binder, size_t globalSlot, size_t localSlot, Accessibility accessibility, size_t startOffset, size_t elementCount)
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

bool GLRenderingServer::SetUserPipelineOutput(RenderPassDataComponent * rhs)
{
	m_userPipelineOutput = reinterpret_cast<GLRenderPassDataComponent*>(rhs);

	return true;
}

bool GLRenderingServer::Present()
{
	CommandListBegin(m_SwapChainRPDC, m_SwapChainRPDC->m_CurrentFrame);

	BindRenderPassDataComponent(m_SwapChainRPDC);

	CleanRenderTargets(m_SwapChainRPDC);

	ActivateResourceBinder(m_SwapChainRPDC, ShaderStage::Pixel, m_SwapChainSDC->m_ResourceBinder, 0, 1, Accessibility::ReadOnly, 0, SIZE_MAX);

	ActivateResourceBinder(m_SwapChainRPDC, ShaderStage::Pixel, m_userPipelineOutput->m_RenderTargetsResourceBinders[0], 0, 0, Accessibility::ReadOnly, 0, SIZE_MAX);

	auto l_mesh = g_pModuleManager->getRenderingFrontend()->getMeshDataComponent(MeshShapeType::Quad);

	DispatchDrawCall(m_SwapChainRPDC, l_mesh, 1);

	DeactivateResourceBinder(m_SwapChainRPDC, ShaderStage::Pixel, m_userPipelineOutput->m_RenderTargetsResourceBinders[0], 0, 0, Accessibility::ReadOnly, 0, SIZE_MAX);

	CommandListEnd(m_SwapChainRPDC);

	ExecuteCommandList(m_SwapChainRPDC);

	WaitForFrame(m_SwapChainRPDC);

	if (m_needResize)
	{
		resizeImpl();
		m_needResize = false;
	}

	return true;
}

bool GLRenderingServer::DispatchCompute(RenderPassDataComponent * renderPass, uint32_t threadGroupX, uint32_t threadGroupY, uint32_t threadGroupZ)
{
	glDispatchCompute(threadGroupX, threadGroupY, threadGroupZ);
	glMemoryBarrier(GL_ALL_BARRIER_BITS);

	return true;
}

bool GLRenderingServer::CopyDepthStencilBuffer(RenderPassDataComponent * src, RenderPassDataComponent * dest)
{
	auto l_src = reinterpret_cast<GLRenderPassDataComponent*>(src);
	auto l_dest = reinterpret_cast<GLRenderPassDataComponent*>(dest);

	glBindFramebuffer(GL_READ_FRAMEBUFFER, l_src->m_FBO);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, l_dest->m_FBO);
	glBlitFramebuffer(0, 0,
		l_src->m_RenderPassDesc.m_RenderTargetDesc.Width, l_src->m_RenderPassDesc.m_RenderTargetDesc.Height,
		0, 0,
		l_dest->m_RenderPassDesc.m_RenderTargetDesc.Width, l_dest->m_RenderPassDesc.m_RenderTargetDesc.Height,
		GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT, GL_NEAREST);

	return true;
}

bool GLRenderingServer::CopyColorBuffer(RenderPassDataComponent * src, size_t srcIndex, RenderPassDataComponent * dest, size_t destIndex)
{
	auto l_src = reinterpret_cast<GLRenderPassDataComponent*>(src);
	auto l_dest = reinterpret_cast<GLRenderPassDataComponent*>(dest);

	glBindFramebuffer(GL_READ_FRAMEBUFFER, l_src->m_FBO);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, l_dest->m_FBO);
	glReadBuffer(GL_COLOR_ATTACHMENT0 + (uint32_t)srcIndex);
	glDrawBuffer(GL_COLOR_ATTACHMENT0 + (uint32_t)destIndex);

	glBlitFramebuffer(0, 0,
		l_src->m_RenderPassDesc.m_RenderTargetDesc.Width, l_src->m_RenderPassDesc.m_RenderTargetDesc.Height,
		0, 0,
		l_dest->m_RenderPassDesc.m_RenderTargetDesc.Width, l_dest->m_RenderPassDesc.m_RenderTargetDesc.Height,
		GL_COLOR_BUFFER_BIT, GL_LINEAR);

	return true;
}

Vec4 GLRenderingServer::ReadRenderTargetSample(RenderPassDataComponent * rhs, size_t renderTargetIndex, size_t x, size_t y)
{
	return Vec4();
}

std::vector<Vec4> GLRenderingServer::ReadTextureBackToCPU(RenderPassDataComponent * canvas, TextureDataComponent * TDC)
{
	auto GLTDC = reinterpret_cast<GLTextureDataComponent*>(TDC);

	auto l_width = GLTDC->m_GLTextureDesc.Width;
	auto l_height = GLTDC->m_GLTextureDesc.Height;
	auto l_depthOrArraySize = GLTDC->m_GLTextureDesc.DepthOrArraySize;

	auto l_pixelDataFormat = GLTDC->m_GLTextureDesc.PixelDataFormat;
	auto l_pixelDataType = GLTDC->m_GLTextureDesc.PixelDataType;

	// @TODO: Support different pixel data type
	std::vector<Vec4> l_textureSamples;
	size_t l_sampleCount;

	GLenum l_attachmentType;

	if (GLTDC->m_TextureDesc.PixelDataFormat == TexturePixelDataFormat::Depth)
	{
		l_attachmentType = GL_DEPTH_ATTACHMENT;
	}
	else if (GLTDC->m_TextureDesc.PixelDataFormat == TexturePixelDataFormat::DepthStencil)
	{
		l_attachmentType = GL_DEPTH_STENCIL_ATTACHMENT;
	}
	else
	{
		l_attachmentType = GL_COLOR_ATTACHMENT0;
	}

	GLuint l_tempFramebuffer = 0;
	glGenFramebuffers(1, &l_tempFramebuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, l_tempFramebuffer);
	glReadBuffer(GL_COLOR_ATTACHMENT0);

	switch (GLTDC->m_TextureDesc.SamplerType)
	{
	case TextureSamplerType::Sampler1D:
		l_sampleCount = l_width;
		l_textureSamples.resize(l_sampleCount);
		glFramebufferTexture1D(GL_FRAMEBUFFER, l_attachmentType, GL_TEXTURE_1D, GLTDC->m_TO, 0);
		glReadPixels(0, 0, l_width, 0, l_pixelDataFormat, l_pixelDataType, &l_textureSamples[0]);
		break;
	case TextureSamplerType::Sampler2D:
		l_sampleCount = l_width * l_height;
		l_textureSamples.resize(l_sampleCount);
		glFramebufferTexture2D(GL_FRAMEBUFFER, l_attachmentType, GL_TEXTURE_2D, GLTDC->m_TO, 0);
		glReadPixels(0, 0, l_width, l_height, l_pixelDataFormat, l_pixelDataType, &l_textureSamples[0]);
		break;
	case TextureSamplerType::Sampler3D:
		l_sampleCount = l_width * l_height;
		l_textureSamples.resize(l_sampleCount * l_depthOrArraySize);
		for (uint32_t i = 0; i < (uint32_t)l_depthOrArraySize; i++)
		{
			glFramebufferTextureLayer(GL_FRAMEBUFFER, l_attachmentType, GLTDC->m_TO, 0, i);
			glReadPixels(0, 0, l_width, 0, l_pixelDataFormat, l_pixelDataType, &l_textureSamples[l_depthOrArraySize * l_sampleCount]);
		}
		break;
	case TextureSamplerType::Sampler1DArray:
		l_sampleCount = l_width;
		l_textureSamples.resize(l_sampleCount * l_depthOrArraySize);
		for (uint32_t i = 0; i < (uint32_t)l_depthOrArraySize; i++)
		{
			glFramebufferTextureLayer(GL_FRAMEBUFFER, l_attachmentType, GLTDC->m_TO, 0, i);
			glReadPixels(0, 0, l_width, 0, l_pixelDataFormat, l_pixelDataType, &l_textureSamples[l_depthOrArraySize * l_sampleCount]);
		}
		break;
	case TextureSamplerType::Sampler2DArray:
		l_sampleCount = l_width * l_height;
		l_textureSamples.resize(l_sampleCount * l_depthOrArraySize);
		for (uint32_t i = 0; i < (uint32_t)l_depthOrArraySize; i++)
		{
			glFramebufferTextureLayer(GL_FRAMEBUFFER, l_attachmentType, GLTDC->m_TO, 0, i);
			glReadPixels(0, 0, l_width, l_height, l_pixelDataFormat, l_pixelDataType, &l_textureSamples[l_depthOrArraySize * l_sampleCount]);
		}
		break;
	case TextureSamplerType::SamplerCubemap:
		l_sampleCount = l_width * l_height;
		l_textureSamples.resize(l_sampleCount * 6);
		for (uint32_t i = 0; i < 6; i++)
		{
			glFramebufferTexture2D(GL_FRAMEBUFFER, l_attachmentType, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, GLTDC->m_TO, 0);
			glReadPixels(0, 0, l_width, l_height, l_pixelDataFormat, l_pixelDataType, &l_textureSamples[i * l_sampleCount]);
		}
		break;
	default:
		break;
	}

	glReadBuffer(GL_NONE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glDeleteFramebuffers(1, &l_tempFramebuffer);

	return l_textureSamples;
}

bool GLRenderingServer::Resize()
{
	m_needResize = true;
	return true;
}