#include "GLRenderingServer.h"
#include "../../Component/GLMeshComponent.h"
#include "../../Component/GLTextureComponent.h"
#include "../../Component/GLMaterialComponent.h"
#include "../../Component/GLRenderPassComponent.h"
#include "../../Component/GLShaderProgramComponent.h"
#include "../../Component/GLSamplerComponent.h"
#include "../../Component/GLGPUBufferComponent.h"

#include "../CommonFunctionDefinationMacro.inl"

#include "../../Interface/IEngine.h"

using namespace Inno;
extern IEngine* g_Engine;

#include "GLHelper.h"
using namespace GLHelper;

#include "../../Core/InnoLogger.h"
#include "../../Core/InnoMemory.h"
#include "../../Core/InnoRandomizer.h"
#include "../../Template/ObjectPool.h"

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

	bool resizeImpl();

	ObjectStatus m_ObjectStatus = ObjectStatus::Terminated;

	TObjectPool<GLMeshComponent>* m_MeshComponentPool = 0;
	TObjectPool<GLMaterialComponent>* m_MaterialComponentPool = 0;
	TObjectPool<GLTextureComponent>* m_TextureComponentPool = 0;
	TObjectPool<GLRenderPassComponent>* m_RenderPassComponentPool = 0;
	TObjectPool<GLPipelineStateObject>* m_PSOPool = 0;
	TObjectPool<GLShaderProgramComponent>* m_ShaderProgramComponentPool = 0;
	TObjectPool<GLSamplerComponent>* m_SamplerComponentPool = 0;
	TObjectPool<GLGPUBufferComponent>* m_GPUBufferComponentPool = 0;

	std::unordered_set<MeshComponent*> m_initializedMeshes;
	std::unordered_set<TextureComponent*> m_initializedTextures;
	std::unordered_set<MaterialComponent*> m_initializedMaterials;
	std::vector<GLRenderPassComponent*> m_RenderPassComps;

	GPUResourceComponent* m_userPipelineOutput = 0;
	GLRenderPassComponent* m_SwapChainRenderPassComp = 0;
	GLShaderProgramComponent* m_SwapChainSPC = 0;
	GLSamplerComponent* m_SwapChainSamplerComp = 0;

	std::atomic_bool m_needResize = false;
}

using namespace GLRenderingServerNS;

GLPipelineStateObject* addPSO()
{
	return m_PSOPool->Spawn();
}

bool GLRenderingServerNS::resizeImpl()
{
	auto l_renderingServer = reinterpret_cast<GLRenderingServer*>(g_Engine->getRenderingServer());
	auto l_screenResolution = g_Engine->getRenderingFrontend()->getScreenResolution();

	for (auto i : m_RenderPassComps)
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
				l_renderingServer->DeleteTextureComponent(j);
			}

			i->m_RenderTargets.clear();

			if (i->m_DepthStencilRenderTarget)
			{
				l_renderingServer->DeleteTextureComponent(i->m_DepthStencilRenderTarget);
			}

			m_PSOPool->Destroy(reinterpret_cast<GLPipelineStateObject*>(i->m_PipelineStateObject));

			i->m_RenderPassDesc.m_RenderTargetDesc.Width = l_screenResolution.x;
			i->m_RenderPassDesc.m_RenderTargetDesc.Height = l_screenResolution.y;

			i->m_RenderPassDesc.m_GraphicsPipelineDesc.m_ViewportDesc.m_Width = (float)l_screenResolution.x;
			i->m_RenderPassDesc.m_GraphicsPipelineDesc.m_ViewportDesc.m_Height = (float)l_screenResolution.y;

			CreateFramebuffer(i);

			ReserveRenderTargets(i, l_renderingServer);

			CreateRenderTargets(i, l_renderingServer);

			i->m_PipelineStateObject = addPSO();

			CreateStateObjects(i);
		}

		m_PSOPool->Destroy(reinterpret_cast<GLPipelineStateObject*>(m_SwapChainRenderPassComp->m_PipelineStateObject));

		m_SwapChainRenderPassComp->m_RenderPassDesc.m_RenderTargetDesc.Width = l_screenResolution.x;
		m_SwapChainRenderPassComp->m_RenderPassDesc.m_RenderTargetDesc.Height = l_screenResolution.y;

		m_SwapChainRenderPassComp->m_RenderPassDesc.m_GraphicsPipelineDesc.m_ViewportDesc.m_Width = (float)l_screenResolution.x;
		m_SwapChainRenderPassComp->m_RenderPassDesc.m_GraphicsPipelineDesc.m_ViewportDesc.m_Height = (float)l_screenResolution.y;

		m_SwapChainRenderPassComp->m_PipelineStateObject = addPSO();

		CreateStateObjects(m_SwapChainRenderPassComp);
	}

	return true;
}

using namespace GLRenderingServerNS;

bool GLRenderingServer::Setup(ISystemConfig* systemConfig)
{
	auto l_renderingCapability = g_Engine->getRenderingFrontend()->getRenderingCapability();

	m_MeshComponentPool = TObjectPool<GLMeshComponent>::Create(l_renderingCapability.maxMeshes);
	m_TextureComponentPool = TObjectPool<GLTextureComponent>::Create(l_renderingCapability.maxTextures);
	m_MaterialComponentPool = TObjectPool<GLMaterialComponent>::Create(l_renderingCapability.maxMaterials);
	m_RenderPassComponentPool = TObjectPool<GLRenderPassComponent>::Create(128);
	m_PSOPool = TObjectPool<GLPipelineStateObject>::Create(128);
	m_ShaderProgramComponentPool = TObjectPool<GLShaderProgramComponent>::Create(256);
	m_SamplerComponentPool = TObjectPool<GLSamplerComponent>::Create(256);
	m_GPUBufferComponentPool = TObjectPool<GLGPUBufferComponent>::Create(256);

	m_RenderPassComps.reserve(128);

	auto l_GLRenderingServerSetupTask = g_Engine->getTaskSystem()->Submit("GLRenderingServerSetupTask", 2, nullptr,
		[&]() {
			glEnable(GL_DEBUG_OUTPUT);
			glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
			glDebugMessageCallback(MessageCallback, 0);

			glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
			glEnable(GL_PROGRAM_POINT_SIZE);
		}
	);

	l_GLRenderingServerSetupTask.m_Future->Get();

	m_SwapChainRenderPassComp = reinterpret_cast<GLRenderPassComponent*>(AddRenderPassComponent("SwapChain/"));
	m_SwapChainSPC = reinterpret_cast<GLShaderProgramComponent*>(AddShaderProgramComponent("SwapChain/"));
	m_SwapChainSamplerComp = reinterpret_cast<GLSamplerComponent*>(AddSamplerComponent("SwapChain/"));

	m_ObjectStatus = ObjectStatus::Created;
	InnoLogger::Log(LogLevel::Success, "GLRenderingServer Setup finished.");

	return true;
}

bool GLRenderingServer::Initialize()
{
	if (m_ObjectStatus == ObjectStatus::Created)
	{
		m_SwapChainSPC->m_ShaderFilePaths.m_VSPath = "2DImageProcess.vert/";
		m_SwapChainSPC->m_ShaderFilePaths.m_PSPath = "swapChain.frag/";

		auto l_GLRenderingServerInitializeTask = g_Engine->getTaskSystem()->Submit("GLRenderingServerInitializeTask", 2, nullptr,
			[&]() {
				InitializeShaderProgramComponent(m_SwapChainSPC);

				InitializeSamplerComponent(m_SwapChainSamplerComp);

				auto l_RenderPassDesc = g_Engine->getRenderingFrontend()->getDefaultRenderPassDesc();

				l_RenderPassDesc.m_RenderTargetCount = 1;

				m_SwapChainRenderPassComp->m_RenderPassDesc = l_RenderPassDesc;
				m_SwapChainRenderPassComp->m_RenderPassDesc.m_RenderTargetDesc.PixelDataType = TexturePixelDataType::UByte;
				m_SwapChainRenderPassComp->m_RenderPassDesc.m_GraphicsPipelineDesc.m_RasterizerDesc.m_UseCulling = false;

				m_SwapChainRenderPassComp->m_ResourceBindingLayoutDescs.resize(2);
				m_SwapChainRenderPassComp->m_ResourceBindingLayoutDescs[0].m_GPUResourceType = GPUResourceType::Image;
				m_SwapChainRenderPassComp->m_ResourceBindingLayoutDescs[0].m_DescriptorSetIndex = 0;
				m_SwapChainRenderPassComp->m_ResourceBindingLayoutDescs[0].m_DescriptorIndex = 0;
				m_SwapChainRenderPassComp->m_ResourceBindingLayoutDescs[0].m_IndirectBinding = true;

				m_SwapChainRenderPassComp->m_ResourceBindingLayoutDescs[1].m_GPUResourceType = GPUResourceType::Sampler;
				m_SwapChainRenderPassComp->m_ResourceBindingLayoutDescs[1].m_DescriptorSetIndex = 0;
				m_SwapChainRenderPassComp->m_ResourceBindingLayoutDescs[1].m_DescriptorIndex = 1;
				m_SwapChainRenderPassComp->m_ResourceBindingLayoutDescs[1].m_IndirectBinding = true;

				m_SwapChainRenderPassComp->m_ShaderProgram = m_SwapChainSPC;
				m_SwapChainRenderPassComp->m_FBO = 0;
				m_SwapChainRenderPassComp->m_RBO = 0;

				m_SwapChainRenderPassComp->m_PipelineStateObject = addPSO();

				CreateStateObjects(m_SwapChainRenderPassComp);

				m_SwapChainRenderPassComp->m_ObjectStatus = ObjectStatus::Activated;
			});

		l_GLRenderingServerInitializeTask.m_Future->Get();
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

AddComponent(GL, Mesh);
AddComponent(GL, Texture);
AddComponent(GL, Material);
AddComponent(GL, RenderPass);
AddComponent(GL, ShaderProgram);
AddComponent(GL, Sampler);
AddComponent(GL, GPUBuffer);

bool GLRenderingServer::InitializeMeshComponent(MeshComponent* rhs)
{
	if (m_initializedMeshes.find(rhs) != m_initializedMeshes.end())
	{
		return true;
	}

	auto l_rhs = reinterpret_cast<GLMeshComponent*>(rhs);

	glGenVertexArrays(1, &l_rhs->m_VAO);
	glBindVertexArray(l_rhs->m_VAO);

	glGenBuffers(1, &l_rhs->m_VBO);
	glBindBuffer(GL_ARRAY_BUFFER, l_rhs->m_VBO);

	glGenBuffers(1, &l_rhs->m_IBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, l_rhs->m_IBO);

#ifdef INNO_DEBUG
	auto l_VAOName = std::string(l_rhs->m_InstanceName.c_str());
	l_VAOName += "_VAO";
	glObjectLabel(GL_VERTEX_ARRAY, l_rhs->m_VAO, (GLsizei)l_VAOName.size(), l_VAOName.c_str());
	auto l_VBOName = std::string(l_rhs->m_InstanceName.c_str());
	l_VBOName += "_VBO";
	glObjectLabel(GL_BUFFER, l_rhs->m_VBO, (GLsizei)l_VBOName.size(), l_VBOName.c_str());
	auto l_IBOName = std::string(l_rhs->m_InstanceName.c_str());
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

	glBufferData(GL_ARRAY_BUFFER, l_rhs->m_Vertices.size() * sizeof(Vertex), &l_rhs->m_Vertices[0], GL_STATIC_DRAW);

	InnoLogger::Log(LogLevel::Verbose, "GLRenderingServer: VBO ", l_rhs->m_VBO, " is initialized.");

	glBufferData(GL_ELEMENT_ARRAY_BUFFER, l_rhs->m_Indices.size() * sizeof(Index), &l_rhs->m_Indices[0], GL_STATIC_DRAW);

	InnoLogger::Log(LogLevel::Verbose, "GLRenderingServer: IBO ", l_rhs->m_IBO, " is initialized.");

	l_rhs->m_ObjectStatus = ObjectStatus::Activated;

	m_initializedMeshes.emplace(l_rhs);

	return true;
}

bool GLRenderingServer::InitializeTextureComponent(TextureComponent* rhs)
{
	if (m_initializedTextures.find(rhs) != m_initializedTextures.end())
	{
		return true;
	}

	auto l_rhs = reinterpret_cast<GLTextureComponent*>(rhs);

	l_rhs->m_GLTextureDesc = GetGLTextureDesc(l_rhs->m_TextureDesc);

	glGenTextures(1, &l_rhs->m_TO);

	glBindTexture(l_rhs->m_GLTextureDesc.TextureSampler, l_rhs->m_TO);

#ifdef INNO_DEBUG
	auto l_TOName = std::string(l_rhs->m_InstanceName.c_str());
	l_TOName += "_TO";
	glObjectLabel(GL_TEXTURE, l_rhs->m_TO, (GLsizei)l_TOName.size(), l_TOName.c_str());
#endif

	glTexParameterfv(l_rhs->m_GLTextureDesc.TextureSampler, GL_TEXTURE_BORDER_COLOR, l_rhs->m_GLTextureDesc.BorderColor);

	if (l_rhs->m_GLTextureDesc.TextureSampler == GL_TEXTURE_1D)
	{
		glTexImage1D(GL_TEXTURE_1D, 0, l_rhs->m_GLTextureDesc.InternalFormat, l_rhs->m_GLTextureDesc.Width, 0, l_rhs->m_GLTextureDesc.PixelDataFormat, l_rhs->m_GLTextureDesc.PixelDataType, l_rhs->m_TextureData);
	}
	else if (l_rhs->m_GLTextureDesc.TextureSampler == GL_TEXTURE_2D)
	{
		glTexImage2D(GL_TEXTURE_2D, 0, l_rhs->m_GLTextureDesc.InternalFormat, l_rhs->m_GLTextureDesc.Width, l_rhs->m_GLTextureDesc.Height, 0, l_rhs->m_GLTextureDesc.PixelDataFormat, l_rhs->m_GLTextureDesc.PixelDataType, l_rhs->m_TextureData);
	}
	else if (l_rhs->m_GLTextureDesc.TextureSampler == GL_TEXTURE_3D)
	{
		glTexImage3D(GL_TEXTURE_3D, 0, l_rhs->m_GLTextureDesc.InternalFormat, l_rhs->m_GLTextureDesc.Width, l_rhs->m_GLTextureDesc.Height, l_rhs->m_GLTextureDesc.DepthOrArraySize, 0, l_rhs->m_GLTextureDesc.PixelDataFormat, l_rhs->m_GLTextureDesc.PixelDataType, l_rhs->m_TextureData);
	}
	else if (l_rhs->m_GLTextureDesc.TextureSampler == GL_TEXTURE_1D_ARRAY)
	{
		glTexImage2D(GL_TEXTURE_1D_ARRAY, 0, l_rhs->m_GLTextureDesc.InternalFormat, l_rhs->m_GLTextureDesc.Width, l_rhs->m_GLTextureDesc.Height, 0, l_rhs->m_GLTextureDesc.PixelDataFormat, l_rhs->m_GLTextureDesc.PixelDataType, l_rhs->m_TextureData);
	}
	else if (l_rhs->m_GLTextureDesc.TextureSampler == GL_TEXTURE_2D_ARRAY)
	{
		glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, l_rhs->m_GLTextureDesc.InternalFormat, l_rhs->m_GLTextureDesc.Width, l_rhs->m_GLTextureDesc.Height, l_rhs->m_GLTextureDesc.DepthOrArraySize, 0, l_rhs->m_GLTextureDesc.PixelDataFormat, l_rhs->m_GLTextureDesc.PixelDataType, l_rhs->m_TextureData);
	}
	else if (l_rhs->m_GLTextureDesc.TextureSampler == GL_TEXTURE_CUBE_MAP)
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
		glGenerateMipmap(l_rhs->m_GLTextureDesc.TextureSampler);
	}

	InnoLogger::Log(LogLevel::Verbose, "GLRenderingServer: TO ", l_rhs->m_TO, " is initialized.");

	l_rhs->m_GPUResourceType = GPUResourceType::Image;
	l_rhs->m_ObjectStatus = ObjectStatus::Activated;

	m_initializedTextures.emplace(l_rhs);

	return true;
}

bool GLRenderingServer::InitializeMaterialComponent(MaterialComponent* rhs)
{
	if (m_initializedMaterials.find(rhs) != m_initializedMaterials.end())
	{
		return true;
	}

	auto l_rhs = reinterpret_cast<GLMaterialComponent*>(rhs);

	auto l_defaultMaterial = g_Engine->getRenderingFrontend()->getDefaultMaterialComponent();

	for (size_t i = 0; i < 8; i++)
	{
		auto l_texture = reinterpret_cast<GLTextureComponent*>(l_rhs->m_TextureSlots[i].m_Texture);

		if (l_texture)
		{
			InitializeTextureComponent(l_texture);
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

bool GLRenderingServer::InitializeRenderPassComponent(RenderPassComponent* rhs)
{
	auto l_rhs = reinterpret_cast<GLRenderPassComponent*>(rhs);

	CreateFramebuffer(l_rhs);

	ReserveRenderTargets(l_rhs, this);

	CreateRenderTargets(l_rhs, this);

	l_rhs->m_PipelineStateObject = addPSO();

	CreateStateObjects(l_rhs);

	m_RenderPassComps.emplace_back(l_rhs);

	l_rhs->m_ObjectStatus = ObjectStatus::Activated;

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);

	return true;
}

bool GLRenderingServer::InitializeShaderProgramComponent(ShaderProgramComponent* rhs)
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

bool GLRenderingServer::InitializeSamplerComponent(SamplerComponent* rhs)
{
	auto l_rhs = reinterpret_cast<GLSamplerComponent*>(rhs);

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

	l_rhs->m_GPUResourceType = GPUResourceType::Sampler;
	l_rhs->m_ObjectStatus = ObjectStatus::Activated;

	return true;
}

bool GLRenderingServer::InitializeGPUBufferComponent(GPUBufferComponent* rhs)
{
	auto l_rhs = reinterpret_cast<GLGPUBufferComponent*>(rhs);

	l_rhs->m_TotalSize = l_rhs->m_ElementCount * l_rhs->m_ElementSize;

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
	glBindBufferRange(l_rhs->m_BufferType, 0, l_rhs->m_Handle, 0, l_rhs->m_TotalSize);

#ifdef INNO_DEBUG
	auto l_GPUBufferName = std::string(l_rhs->m_InstanceName.c_str());
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

	l_rhs->m_GPUResourceType = GPUResourceType::Buffer;
	l_rhs->m_ObjectStatus = ObjectStatus::Activated;

	return true;
}

bool GLRenderingServer::DeleteMeshComponent(MeshComponent* rhs)
{
	auto l_rhs = reinterpret_cast<GLMeshComponent*>(rhs);

	glDeleteVertexArrays(1, &l_rhs->m_VAO);
	glDeleteBuffers(1, &l_rhs->m_VBO);
	glDeleteBuffers(1, &l_rhs->m_IBO);

	m_MeshComponentPool->Destroy(l_rhs);

	m_initializedMeshes.erase(l_rhs);

	return true;
}

bool GLRenderingServer::DeleteTextureComponent(TextureComponent* rhs)
{
	auto l_rhs = reinterpret_cast<GLTextureComponent*>(rhs);

	glDeleteTextures(1, &l_rhs->m_TO);

	m_TextureComponentPool->Destroy(l_rhs);

	m_initializedTextures.erase(l_rhs);

	return true;
}

bool GLRenderingServer::DeleteMaterialComponent(MaterialComponent* rhs)
{
	auto l_rhs = reinterpret_cast<GLMaterialComponent*>(rhs);

	m_MaterialComponentPool->Destroy(reinterpret_cast<GLMaterialComponent*>(l_rhs));

	m_initializedMaterials.erase(l_rhs);

	return true;
}

bool GLRenderingServer::DeleteRenderPassComponent(RenderPassComponent* rhs)
{
	auto l_rhs = reinterpret_cast<GLRenderPassComponent*>(rhs);
	auto l_PSO = reinterpret_cast<GLPipelineStateObject*>(l_rhs->m_PipelineStateObject);

	if (l_rhs->m_RBO)
	{
		glDeleteRenderbuffers(1, &l_rhs->m_RBO);
	}
	glDeleteFramebuffers(1, &l_rhs->m_FBO);

	if (l_rhs->m_DepthStencilRenderTarget)
	{
		DeleteTextureComponent(l_rhs->m_DepthStencilRenderTarget);
	}

	for (size_t i = 0; i < l_rhs->m_RenderTargets.size(); i++)
	{
		DeleteTextureComponent(l_rhs->m_RenderTargets[i]);
	}

	m_RenderPassComponentPool->Destroy(l_rhs);

	return true;
}

bool GLRenderingServer::DeleteShaderProgramComponent(ShaderProgramComponent* rhs)
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

bool GLRenderingServer::DeleteSamplerComponent(SamplerComponent* rhs)
{
	auto l_rhs = reinterpret_cast<GLSamplerComponent*>(rhs);

	glDeleteSamplers(1, &l_rhs->m_SO);

	m_SamplerComponentPool->Destroy(l_rhs);

	return true;
}

bool GLRenderingServer::DeleteGPUBufferComponent(GPUBufferComponent* rhs)
{
	auto l_rhs = reinterpret_cast<GLGPUBufferComponent*>(rhs);

	glDeleteBuffers(1, &l_rhs->m_Handle);

	m_GPUBufferComponentPool->Destroy(l_rhs);

	return true;
}

bool GLRenderingServer::ClearTextureComponent(TextureComponent* rhs)
{
	auto l_rhs = reinterpret_cast<GLTextureComponent*>(rhs);

	glBindTexture(l_rhs->m_GLTextureDesc.TextureSampler, l_rhs->m_TO);
	glClearTexImage(l_rhs->m_TO, 0, l_rhs->m_GLTextureDesc.PixelDataFormat, l_rhs->m_GLTextureDesc.PixelDataType, &l_rhs->m_TextureDesc.ClearColor[0]);

	return true;
}

bool GLRenderingServer::CopyTextureComponent(TextureComponent* lhs, TextureComponent* rhs)
{
	auto l_src = reinterpret_cast<GLTextureComponent*>(lhs);
	auto l_dest = reinterpret_cast<GLTextureComponent*>(rhs);

	glCopyImageSubData(l_src->m_TO, l_src->m_GLTextureDesc.TextureSampler, 0, 0, 0, 0, l_dest->m_TO, l_dest->m_GLTextureDesc.TextureSampler, 0, 0, 0, 0, l_src->m_GLTextureDesc.Width, l_src->m_GLTextureDesc.Height, l_src->m_GLTextureDesc.DepthOrArraySize);

	return true;
}

bool GLRenderingServer::UploadGPUBufferComponentImpl(GPUBufferComponent* rhs, const void* GPUBufferValue, size_t startOffset, size_t range)
{
	auto l_rhs = reinterpret_cast<GLGPUBufferComponent*>(rhs);

	glBindBuffer(l_rhs->m_BufferType, l_rhs->m_Handle);
	auto l_size = l_rhs->m_TotalSize;
	if (range != SIZE_MAX)
	{
		l_size = range * l_rhs->m_ElementSize;
	}
	glBufferSubData(l_rhs->m_BufferType, startOffset * l_rhs->m_ElementSize, l_size, GPUBufferValue);

	return true;
}

bool GLRenderingServer::UpdateMeshComponent(MeshComponent* rhs)
{
	return true;
}

bool GLRenderingServer::ClearGPUBufferComponent(GPUBufferComponent* rhs)
{
	const GLuint zero = 0;
	auto l_rhs = reinterpret_cast<GLGPUBufferComponent*>(rhs);
	glBindBuffer(l_rhs->m_BufferType, l_rhs->m_Handle);
	glClearBufferData(l_rhs->m_BufferType, GL_R32UI, GL_RED, GL_UNSIGNED_INT, &zero);

	return true;
}

bool GLRenderingServer::CommandListBegin(RenderPassComponent* rhs, size_t frameIndex)
{
	auto l_rhs = reinterpret_cast<GLRenderPassComponent*>(rhs);
	auto l_GLPSO = reinterpret_cast<GLPipelineStateObject*>(l_rhs->m_PipelineStateObject);

	for (size_t i = 0; i < l_GLPSO->m_Activate.size(); i++)
	{
		l_GLPSO->m_Activate[i]();
	}

	return true;
}

bool GLRenderingServer::BindRenderPassComponent(RenderPassComponent* rhs)
{
	auto l_rhs = reinterpret_cast<GLRenderPassComponent*>(rhs);
	if (l_rhs->m_RenderPassDesc.m_GPUEngineType == GPUEngineType::Graphics)
	{
		auto l_GLPSO = reinterpret_cast<GLPipelineStateObject*>(l_rhs->m_PipelineStateObject);

		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, l_rhs->m_FBO);
		if (l_rhs->m_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_DepthEnable)
		{
			glBindRenderbuffer(GL_RENDERBUFFER, l_rhs->m_RBO);
			glRenderbufferStorage(GL_RENDERBUFFER, l_rhs->m_renderBufferInternalFormat, (GLsizei)l_rhs->m_RenderPassDesc.m_GraphicsPipelineDesc.m_ViewportDesc.m_Width, (GLsizei)l_rhs->m_RenderPassDesc.m_GraphicsPipelineDesc.m_ViewportDesc.m_Height);
		}
	}

	auto l_shaderProgram = reinterpret_cast<GLShaderProgramComponent*>(l_rhs->m_ShaderProgram);

	glUseProgram(l_shaderProgram->m_ProgramID);

	return true;
}

bool GLRenderingServer::CleanRenderTargets(RenderPassComponent* rhs)
{
	if (rhs->m_RenderPassDesc.m_GPUEngineType == GPUEngineType::Graphics)
	{
		auto l_cleanColor = rhs->m_RenderPassDesc.m_RenderTargetDesc.ClearColor;
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

bool GLRenderingServer::BindGPUResource(RenderPassComponent* renderPass, ShaderStage shaderStage, GPUResourceComponent* resource, size_t resourceBindingLayoutDescIndex, Accessibility accessibility, size_t startOffset, size_t elementCount)
{
	if (resource)
	{
		auto l_localSlot = renderPass->m_ResourceBindingLayoutDescs[resourceBindingLayoutDescIndex].m_DescriptorIndex;
		switch (resource->m_GPUResourceType)
		{
		case GPUResourceType::Sampler:
			for (size_t i = 0; i < 16; i++)
			{
				glBindSampler((GLuint)i, reinterpret_cast<GLGPUBufferComponent*>(resource)->m_Handle);
			}
			break;
		case GPUResourceType::Image:
			if (accessibility == Accessibility::ReadOnly)
			{
				ActivateTexture(reinterpret_cast<GLTextureComponent*>(resource), (uint32_t)l_localSlot);
			}
			else
			{
				BindTextureAsImage(reinterpret_cast<GLTextureComponent*>(resource), (uint32_t)l_localSlot, accessibility);
			}
			break;
		case GPUResourceType::Buffer:
			BindGPUBuffer(resource->m_GPUAccessibility, reinterpret_cast<GLGPUBufferComponent*>(resource)->m_Handle, l_localSlot, startOffset, reinterpret_cast<GLGPUBufferComponent*>(resource)->m_ElementSize, reinterpret_cast<GLGPUBufferComponent*>(resource)->m_TotalSize, elementCount);
			break;
		default:
			break;
		}
	}

	return true;
}

bool GLRenderingServer::DrawIndexedInstanced(RenderPassComponent* renderPass, MeshComponent* mesh, size_t instanceCount)
{
	auto l_GLPSO = reinterpret_cast<GLPipelineStateObject*>(renderPass->m_PipelineStateObject);
	auto l_mesh = reinterpret_cast<GLMeshComponent*>(mesh);

	glBindVertexArray(l_mesh->m_VAO);
	glDrawElementsInstanced(l_GLPSO->m_GLPrimitiveTopology, (GLsizei)l_mesh->m_IndexCount, GL_UNSIGNED_INT, 0, (GLsizei)instanceCount);

	return true;
}

bool GLRenderingServer::DrawInstanced(RenderPassComponent* renderPass, size_t instanceCount)
{
	auto l_GLPSO = reinterpret_cast<GLPipelineStateObject*>(renderPass->m_PipelineStateObject);

	glBindVertexArray(0);
	glDrawArrays(l_GLPSO->m_GLPrimitiveTopology, 0, (GLsizei)instanceCount);

	return true;
}

bool GLRenderingServer::UnbindGPUResource(RenderPassComponent* renderPass, ShaderStage shaderStage, GPUResourceComponent* resource, size_t resourceBindingLayoutDescIndex, Accessibility accessibility, size_t startOffset, size_t elementCount)
{
	return true;
}

bool GLRenderingServer::CommandListEnd(RenderPassComponent* rhs)
{
	auto l_rhs = reinterpret_cast<GLRenderPassComponent*>(rhs);
	auto l_GLPSO = reinterpret_cast<GLPipelineStateObject*>(l_rhs->m_PipelineStateObject);

	for (size_t i = 0; i < l_GLPSO->m_Deactivate.size(); i++)
	{
		l_GLPSO->m_Deactivate[i]();
	}

	return true;
}

bool GLRenderingServer::ExecuteCommandList(RenderPassComponent* rhs, GPUEngineType GPUEngineType)
{
	return true;
}

bool GLRenderingServer::WaitCommandQueue(RenderPassComponent* rhs, GPUEngineType queueType, GPUEngineType semaphoreType)
{
	return true;
}

bool GLRenderingServer::WaitFence(RenderPassComponent* rhs, GPUEngineType GPUEngineType)
{
	return true;
}

bool GLRenderingServer::SetUserPipelineOutput(GPUResourceComponent* rhs)
{
	m_userPipelineOutput = rhs;

	return true;
}

GPUResourceComponent* GLRenderingServer::GetUserPipelineOutput()
{
	return m_userPipelineOutput;
}

bool GLRenderingServer::Present()
{
	CommandListBegin(m_SwapChainRenderPassComp, m_SwapChainRenderPassComp->m_CurrentFrame);

	BindRenderPassComponent(m_SwapChainRenderPassComp);

	CleanRenderTargets(m_SwapChainRenderPassComp);

	BindGPUResource(m_SwapChainRenderPassComp, ShaderStage::Pixel, m_SwapChainSamplerComp, 1, Accessibility::ReadOnly, 0, SIZE_MAX);

	BindGPUResource(m_SwapChainRenderPassComp, ShaderStage::Pixel, m_userPipelineOutput, 0, Accessibility::ReadOnly, 0, SIZE_MAX);

	auto l_mesh = g_Engine->getRenderingFrontend()->getMeshComponent(ProceduralMeshShape::Square);

	DrawIndexedInstanced(m_SwapChainRenderPassComp, l_mesh, 1);

	UnbindGPUResource(m_SwapChainRenderPassComp, ShaderStage::Pixel, m_userPipelineOutput, 0, Accessibility::ReadOnly, 0, SIZE_MAX);

	CommandListEnd(m_SwapChainRenderPassComp);

	ExecuteCommandList(m_SwapChainRenderPassComp, GPUEngineType::Graphics);

	WaitFence(m_SwapChainRenderPassComp, GPUEngineType::Graphics);

	if (m_needResize)
	{
		resizeImpl();
		m_needResize = false;
	}

	return true;
}

bool GLRenderingServer::Dispatch(RenderPassComponent* renderPass, uint32_t threadGroupX, uint32_t threadGroupY, uint32_t threadGroupZ)
{
	glDispatchCompute(threadGroupX, threadGroupY, threadGroupZ);
	glMemoryBarrier(GL_ALL_BARRIER_BITS);

	return true;
}

Vec4 GLRenderingServer::ReadRenderTargetSample(RenderPassComponent* rhs, size_t renderTargetIndex, size_t x, size_t y)
{
	return Vec4();
}

std::vector<Vec4> GLRenderingServer::ReadTextureBackToCPU(RenderPassComponent* canvas, TextureComponent* TextureComp)
{
	auto GLTextureComp = reinterpret_cast<GLTextureComponent*>(TextureComp);

	auto l_width = GLTextureComp->m_GLTextureDesc.Width;
	auto l_height = GLTextureComp->m_GLTextureDesc.Height;
	auto l_depthOrArraySize = GLTextureComp->m_GLTextureDesc.DepthOrArraySize;

	auto l_pixelDataFormat = GLTextureComp->m_GLTextureDesc.PixelDataFormat;
	auto l_pixelDataType = GLTextureComp->m_GLTextureDesc.PixelDataType;

	// @TODO: Support different pixel data type
	std::vector<Vec4> l_textureSamples;
	size_t l_sampleCount;

	GLenum l_attachmentType;

	if (GLTextureComp->m_TextureDesc.PixelDataFormat == TexturePixelDataFormat::Depth)
	{
		l_attachmentType = GL_DEPTH_ATTACHMENT;
	}
	else if (GLTextureComp->m_TextureDesc.PixelDataFormat == TexturePixelDataFormat::DepthStencil)
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

	switch (GLTextureComp->m_TextureDesc.Sampler)
	{
	case TextureSampler::Sampler1D:
		l_sampleCount = l_width;
		l_textureSamples.resize(l_sampleCount);
		glFramebufferTexture1D(GL_FRAMEBUFFER, l_attachmentType, GL_TEXTURE_1D, GLTextureComp->m_TO, 0);
		glReadPixels(0, 0, l_width, 0, l_pixelDataFormat, l_pixelDataType, &l_textureSamples[0]);
		break;
	case TextureSampler::Sampler2D:
		l_sampleCount = l_width * l_height;
		l_textureSamples.resize(l_sampleCount);
		glFramebufferTexture2D(GL_FRAMEBUFFER, l_attachmentType, GL_TEXTURE_2D, GLTextureComp->m_TO, 0);
		glReadPixels(0, 0, l_width, l_height, l_pixelDataFormat, l_pixelDataType, &l_textureSamples[0]);
		break;
	case TextureSampler::Sampler3D:
		l_sampleCount = l_width * l_height;
		l_textureSamples.resize(l_sampleCount * l_depthOrArraySize);
		for (uint32_t i = 0; i < (uint32_t)l_depthOrArraySize; i++)
		{
			glFramebufferTextureLayer(GL_FRAMEBUFFER, l_attachmentType, GLTextureComp->m_TO, 0, i);
			glReadPixels(0, 0, l_width, 0, l_pixelDataFormat, l_pixelDataType, &l_textureSamples[l_depthOrArraySize * l_sampleCount]);
		}
		break;
	case TextureSampler::Sampler1DArray:
		l_sampleCount = l_width;
		l_textureSamples.resize(l_sampleCount * l_depthOrArraySize);
		for (uint32_t i = 0; i < (uint32_t)l_depthOrArraySize; i++)
		{
			glFramebufferTextureLayer(GL_FRAMEBUFFER, l_attachmentType, GLTextureComp->m_TO, 0, i);
			glReadPixels(0, 0, l_width, 0, l_pixelDataFormat, l_pixelDataType, &l_textureSamples[l_depthOrArraySize * l_sampleCount]);
		}
		break;
	case TextureSampler::Sampler2DArray:
		l_sampleCount = l_width * l_height;
		l_textureSamples.resize(l_sampleCount * l_depthOrArraySize);
		for (uint32_t i = 0; i < (uint32_t)l_depthOrArraySize; i++)
		{
			glFramebufferTextureLayer(GL_FRAMEBUFFER, l_attachmentType, GLTextureComp->m_TO, 0, i);
			glReadPixels(0, 0, l_width, l_height, l_pixelDataFormat, l_pixelDataType, &l_textureSamples[l_depthOrArraySize * l_sampleCount]);
		}
		break;
	case TextureSampler::SamplerCubemap:
		l_sampleCount = l_width * l_height;
		l_textureSamples.resize(l_sampleCount * 6);
		for (uint32_t i = 0; i < 6; i++)
		{
			glFramebufferTexture2D(GL_FRAMEBUFFER, l_attachmentType, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, GLTextureComp->m_TO, 0);
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

bool GLRenderingServer::GenerateMipmap(TextureComponent* rhs)
{
	auto l_rhs = reinterpret_cast<GLTextureComponent*>(rhs);
	glGenerateMipmap(l_rhs->m_GLTextureDesc.TextureSampler);

	return true;
}

bool GLRenderingServer::Resize()
{
	m_needResize = true;
	return true;
}

bool GLRenderingServer::BeginCapture()
{
	return false;
}

bool GLRenderingServer::EndCapture()
{
	return false;
}