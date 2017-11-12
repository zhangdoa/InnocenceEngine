#include "../../main/stdafx.h"
#include "GraphicData.h"

MeshData::MeshData()
{
}


MeshData::~MeshData()
{
}

void MeshData::init()
{
	m_GLMeshData.init();
}

void MeshData::draw()
{
	m_GLMeshData.draw(m_intices);
}

void MeshData::shutdown()
{
	m_GLMeshData.shutdown();
}

std::vector<GLVertexData>& MeshData::getVertices()
{
	return m_vertices;
}

std::vector<unsigned int>& MeshData::getIntices()
{
	return m_intices;
}

void MeshData::sendDataToGPU()
{
	m_GLMeshData.sendDataToGPU(m_vertices, m_intices, false);
}

void MeshData::addTestCube()
{
	GLVertexData l_VertexData_1;
	l_VertexData_1.m_pos = glm::vec3(0.5f, 0.5f, 0.5f);
	l_VertexData_1.m_texCoord = glm::vec2(1.0f, 1.0f);
	l_VertexData_1.m_normal = glm::vec3(0.0f, 0.0f, 1.0f);

	GLVertexData l_VertexData_2;
	l_VertexData_2.m_pos = glm::vec3(0.5f, -0.5f, 0.5f);
	l_VertexData_2.m_texCoord = glm::vec2(1.0f, 0.0f);
	l_VertexData_2.m_normal = glm::vec3(0.0f, 0.0f, 1.0f);

	GLVertexData l_VertexData_3;
	l_VertexData_3.m_pos = glm::vec3(-0.5f, -0.5f, 0.5f);
	l_VertexData_3.m_texCoord = glm::vec2(0.0f, 0.0f);
	l_VertexData_3.m_normal = glm::vec3(0.0f, 0.0f, 1.0f);

	GLVertexData l_VertexData_4;
	l_VertexData_4.m_pos = glm::vec3(-0.5f, 0.5f, 0.5f);
	l_VertexData_4.m_texCoord = glm::vec2(0.0f, 1.0f);
	l_VertexData_4.m_normal = glm::vec3(0.0f, 0.0f, 1.0f);

	GLVertexData l_VertexData_5;
	l_VertexData_5.m_pos = glm::vec3(0.5f, 0.5f, -0.5f);
	l_VertexData_5.m_texCoord = glm::vec2(1.0f, 1.0f);
	l_VertexData_5.m_normal = glm::vec3(0.0f, 0.0f, -1.0f);

	GLVertexData l_VertexData_6;
	l_VertexData_6.m_pos = glm::vec3(0.5f, -0.5f, -0.5f);
	l_VertexData_6.m_texCoord = glm::vec2(1.0f, 0.0f);
	l_VertexData_6.m_normal = glm::vec3(0.0f, 0.0f, -1.0f);

	GLVertexData l_VertexData_7;
	l_VertexData_7.m_pos = glm::vec3(-0.5f, -0.5f, -0.5f);
	l_VertexData_7.m_texCoord = glm::vec2(0.0f, 0.0f);
	l_VertexData_7.m_normal = glm::vec3(0.0f, 0.0f, -1.0f);

	GLVertexData l_VertexData_8;
	l_VertexData_8.m_pos = glm::vec3(-0.5f, 0.5f, -0.5f);
	l_VertexData_8.m_texCoord = glm::vec2(0.0f, 1.0f);
	l_VertexData_8.m_normal = glm::vec3(0.0f, 0.0f, -1.0f);

	m_vertices = { l_VertexData_1, l_VertexData_2, l_VertexData_3, l_VertexData_4, l_VertexData_5, l_VertexData_6, l_VertexData_7, l_VertexData_8 };

	m_intices = { 0, 1, 3, 1, 2, 3,
		4, 5, 0, 5, 1, 0,
		7, 6, 4, 6, 5, 4,
		3, 2, 7, 2, 6 ,7,
		4, 0, 7, 0, 3, 7,
		1, 5, 2, 5, 6, 2 };
}

void MeshData::addTestSkybox()
{
	GLVertexData l_VertexData_1;
	l_VertexData_1.m_pos = glm::vec3(1.0f, 1.0f, 1.0f);

	GLVertexData l_VertexData_2;
	l_VertexData_2.m_pos = glm::vec3(1.0f, -1.0f, 1.0f);

	GLVertexData l_VertexData_3;
	l_VertexData_3.m_pos = glm::vec3(-1.0f, -1.0f, 1.0f);

	GLVertexData l_VertexData_4;
	l_VertexData_4.m_pos = glm::vec3(-1.0f, 1.0f, 1.0f);

	GLVertexData l_VertexData_5;
	l_VertexData_5.m_pos = glm::vec3(1.0f, 1.0f, -1.0f);

	GLVertexData l_VertexData_6;
	l_VertexData_6.m_pos = glm::vec3(1.0f, -1.0f, -1.0f);

	GLVertexData l_VertexData_7;
	l_VertexData_7.m_pos = glm::vec3(-1.0f, -1.0f, -1.0f);

	GLVertexData l_VertexData_8;
	l_VertexData_8.m_pos = glm::vec3(-1.0f, 1.0f, -1.0f);

	m_vertices = { l_VertexData_1, l_VertexData_2, l_VertexData_3, l_VertexData_4, l_VertexData_5, l_VertexData_6, l_VertexData_7, l_VertexData_8 };

	m_intices = { 0, 1, 3, 1, 2, 3,
		4, 5, 0, 5, 1, 0,
		7, 6, 4, 6, 5, 4,
		3, 2, 7, 2, 6 ,7,
		4, 0, 7, 0, 3, 7,
		1, 5, 2, 5, 6, 2 };
}

void MeshData::addTestBillboard()
{
	GLVertexData l_VertexData_1;
	l_VertexData_1.m_pos = glm::vec3(0.2f, 0.2f, 0.0f);
	l_VertexData_1.m_texCoord = glm::vec2(1.0f, 1.0f);
	l_VertexData_1.m_normal = glm::vec3(0.0f, 0.0f, 1.0f);

	GLVertexData l_VertexData_2;
	l_VertexData_2.m_pos = glm::vec3(0.2f, -0.2f, 0.0f);
	l_VertexData_2.m_texCoord = glm::vec2(1.0f, 0.0f);
	l_VertexData_2.m_normal = glm::vec3(0.0f, 0.0f, 1.0f);

	GLVertexData l_VertexData_3;
	l_VertexData_3.m_pos = glm::vec3(-0.2f, -0.2f, 0.0f);
	l_VertexData_3.m_texCoord = glm::vec2(0.0f, 0.0f);
	l_VertexData_3.m_normal = glm::vec3(0.0f, 0.0f, 1.0f);

	GLVertexData l_VertexData_4;
	l_VertexData_4.m_pos = glm::vec3(-0.2f, 0.2f, 0.0f);
	l_VertexData_4.m_texCoord = glm::vec2(0.0f, 1.0f);
	l_VertexData_4.m_normal = glm::vec3(0.0f, 0.0f, 1.0f);

	m_vertices = { l_VertexData_1, l_VertexData_2, l_VertexData_3, l_VertexData_4 };
	m_intices = { 0, 1, 3, 1, 2, 3 };
}

TextureData::TextureData()
{
}

TextureData::~TextureData()
{
}

void TextureData::init()
{
	m_GLTextureData.init(m_textureType, m_textureWrapMethod);
}

void TextureData::draw()
{
	m_GLTextureData.draw(m_textureType);
}

void TextureData::shutdown()
{
	m_GLTextureData.shutdown();
}

void TextureData::sendDataToGPU(int textureIndex, int textureFormat, int textureWidth, int textureHeight, void * textureData) const
{
	m_GLTextureData.sendDataToGPU(m_textureType, textureIndex, textureFormat, textureWidth, textureHeight, textureData);
}

void TextureData::setTextureType(textureType textureType)
{
	m_textureType = textureType;
}

textureType TextureData::getTextureType() const
{
	return m_textureType;
}

void TextureData::setTextureWrapMethod(textureWrapMethod textureWrapMethod)
{
	m_textureWrapMethod = textureWrapMethod;
}

const textureWrapMethod & TextureData::getTextureWrapMethod() const
{
	return m_textureWrapMethod;
}

GraphicData::GraphicData()
{
}

GraphicData::~GraphicData()
{
}

void GraphicData::init()
{
	if (m_visiblilityType == visiblilityType::SKYBOX)
	{
		m_meshData.addTestSkybox();
	}
	if (m_visiblilityType == visiblilityType::BILLBOARD)
	{
		m_meshData.addTestBillboard();
	}
	m_meshData.init();
	m_meshData.sendDataToGPU();
}

void GraphicData::draw()
{
	for (size_t i = 0; i < m_textureData.size(); i++)
	{
		m_textureData[i].draw();
	}
	m_meshData.draw();
}

void GraphicData::shutdown()
{
}

const visiblilityType & GraphicData::getVisiblilityType() const
{
	return m_visiblilityType;
}

void GraphicData::setVisiblilityType(visiblilityType visiblilityType)
{
	m_visiblilityType = visiblilityType;
}

const textureWrapMethod & GraphicData::getTextureWrapMethod() const
{
	return m_textureWrapMethod;
}

void GraphicData::setTextureWrapMethod(textureWrapMethod textureWrapMethod)
{
	m_textureWrapMethod = textureWrapMethod;
}

MeshData & GraphicData::getMeshData()
{
	return m_meshData;
}

std::vector<TextureData>& GraphicData::getTextureData()
{
	return m_textureData;
}

ShadowMapData::ShadowMapData()
{
}

ShadowMapData::~ShadowMapData()
{
}

void ShadowMapData::init()
{
	//generate depth map frame buffer
	glGenFramebuffers(1, &depthMapFBO);

	//generate depth map texture
	glGenTextures(1, &m_textureID);
	glBindTexture(GL_TEXTURE_2D, m_textureID);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, m_shadowMapWidth, m_shadowMapHeight, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	//bind frame buffer
	glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_textureID, 0);

	//no need for color
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void ShadowMapData::draw()
{
	glViewport(0, 0, m_shadowMapWidth, m_shadowMapHeight);
	glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
	glClear(GL_DEPTH_BUFFER_BIT);

	switch (m_shadowProjectionType)
	{
	case shadowProjectionType::ORTHOGRAPHIC: m_projectionMatrix = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, 1.0f, 10.0f); break;
	case shadowProjectionType::PERSPECTIVE: m_projectionMatrix = glm::perspective(glm::radians(45.0f), (GLfloat)m_shadowMapWidth / (GLfloat)m_shadowMapHeight, 1.0f, 100.0f); break;
	}


	// @TODO: finish shadow map drawing
	//glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void ShadowMapData::shutdown()
{
}

void ShadowMapData::setShadowProjectionType(shadowProjectionType shadowProjectionType)
{
	m_shadowProjectionType = shadowProjectionType;
}

void ShadowMapData::getProjectionMatrix(glm::mat4 & projectionMatrix)
{
	projectionMatrix = m_projectionMatrix;
}