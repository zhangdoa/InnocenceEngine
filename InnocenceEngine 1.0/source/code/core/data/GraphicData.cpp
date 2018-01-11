#include "../../main/stdafx.h"
#include "GraphicData.h"

MeshData::MeshData()
{
}


MeshData::~MeshData()
{
}

void MeshData::initialize()
{
	m_GLMeshData.init();
	if (m_calculateNormals)
	{
		for (auto& l_vertices : m_vertices)
		{
			l_vertices.m_normal = l_vertices.m_pos;
		}
	}
	m_GLMeshData.sendDataToGPU(m_vertices, m_indices);
}

void MeshData::setup()
{
}

void MeshData::setup(meshDrawMethod meshDrawMethod, bool calculateNormals, bool calculateTangents)
{
	m_meshDrawMethod = meshDrawMethod;
	m_calculateNormals = calculateNormals;
	m_calculateTangents = calculateTangents;
	setStatus(objectStatus::ALIVE);
}

void MeshData::update()
{
	if (getStatus() == objectStatus::ALIVE)
	{
		m_GLMeshData.draw(m_indices, m_meshDrawMethod);
	}
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
	return m_indices;
}

void MeshData::addVertices(GLVertexData & GLVertexData)
{
	m_vertices.emplace_back(GLVertexData);
}

void MeshData::addVertices(glm::vec3 & pos, glm::vec2 & texCoord, glm::vec3 & m_normal)
{
	m_vertices.emplace_back(GLVertexData(pos, texCoord, m_normal));
}

void MeshData::addVertices(float pos_x, float pos_y, float pos_z, float texCoord_x, float texCoord_y, float normal_x, float normal_y, float normal_z)
{
	m_vertices.emplace_back(GLVertexData(pos_x, pos_y, pos_z, texCoord_x, texCoord_y, normal_x, normal_y, normal_z));
}

void MeshData::addUnitCube()
{
	GLVertexData l_VertexData_1;
	l_VertexData_1.m_pos = glm::vec3(1.0f, 1.0f, 1.0f);
	l_VertexData_1.m_texCoord = glm::vec2(1.0f, 1.0f);

	GLVertexData l_VertexData_2;
	l_VertexData_2.m_pos = glm::vec3(1.0f, -1.0f, 1.0f);
	l_VertexData_2.m_texCoord = glm::vec2(1.0f, 0.0f);

	GLVertexData l_VertexData_3;
	l_VertexData_3.m_pos = glm::vec3(-1.0f, -1.0f, 1.0f);
	l_VertexData_3.m_texCoord = glm::vec2(0.0f, 0.0f);

	GLVertexData l_VertexData_4;
	l_VertexData_4.m_pos = glm::vec3(-1.0f, 1.0f, 1.0f);
	l_VertexData_4.m_texCoord = glm::vec2(0.0f, 1.0f);

	GLVertexData l_VertexData_5;
	l_VertexData_5.m_pos = glm::vec3(1.0f, 1.0f, -1.0f);
	l_VertexData_5.m_texCoord = glm::vec2(1.0f, 1.0f);

	GLVertexData l_VertexData_6;
	l_VertexData_6.m_pos = glm::vec3(1.0f, -1.0f, -1.0f);
	l_VertexData_6.m_texCoord = glm::vec2(1.0f, 0.0f);

	GLVertexData l_VertexData_7;
	l_VertexData_7.m_pos = glm::vec3(-1.0f, -1.0f, -1.0f);
	l_VertexData_7.m_texCoord = glm::vec2(0.0f, 0.0f);

	GLVertexData l_VertexData_8;
	l_VertexData_8.m_pos = glm::vec3(-1.0f, 1.0f, -1.0f);
	l_VertexData_8.m_texCoord = glm::vec2(0.0f, 1.0f);


	m_vertices = { l_VertexData_1, l_VertexData_2, l_VertexData_3, l_VertexData_4, l_VertexData_5, l_VertexData_6, l_VertexData_7, l_VertexData_8 };

	for (auto& l_vertexData : m_vertices)
	{
		l_vertexData.m_normal = glm::normalize(l_vertexData.m_pos);
		//l_vertexData.m_tangent = glm::normalize(glm::cross(glm::vec3(0.0, 0.0, 1.0), l_vertexData.m_normal));
		//l_vertexData.m_bitangent = glm::normalize(glm::cross(l_vertexData.m_tangent, l_vertexData.m_normal));
	}
	m_indices = { 0, 1, 3, 1, 2, 3,
		4, 5, 0, 5, 1, 0,
		7, 6, 4, 6, 5, 4,
		3, 2, 7, 2, 6 ,7,
		4, 0, 7, 0, 3, 7,
		1, 5, 2, 5, 6, 2 };
}

void MeshData::addUnitSphere()
{
	unsigned int X_SEGMENTS = 64;
	unsigned int Y_SEGMENTS = 64;
	double PI = 3.14159265359;

	for (unsigned int y = 0; y <= Y_SEGMENTS; ++y)
	{
		for (unsigned int x = 0; x <= X_SEGMENTS; ++x)
		{
			float xSegment = (float)x / (float)X_SEGMENTS;
			float ySegment = (float)y / (float)Y_SEGMENTS;
			double xPos = glm::cos(xSegment * 2.0f * PI) * glm::sin(ySegment * PI);
			double yPos = glm::cos(ySegment * PI);
			double zPos = glm::sin(xSegment * 2.0f * PI) * glm::sin(ySegment * PI);

			GLVertexData l_VertexData;
			l_VertexData.m_pos = glm::vec3(xPos, yPos, zPos);
			l_VertexData.m_texCoord = glm::vec2(xSegment, ySegment);
			l_VertexData.m_normal = glm::normalize(glm::vec3(xPos, yPos, zPos));
			//l_VertexData.m_tangent = glm::normalize(glm::cross(glm::vec3(0.0, 0.0, 1.0), l_VertexData.m_normal));
			//l_VertexData.m_bitangent = glm::normalize(glm::cross(l_VertexData.m_tangent, l_VertexData.m_normal));
			m_vertices.emplace_back(l_VertexData);
		}
	}

	bool oddRow = false;
	for (unsigned y = 0; y < Y_SEGMENTS; ++y)
	{
		if (!oddRow) // even rows: y == 0, y == 2; and so on
		{
			for (unsigned x = 0; x <= X_SEGMENTS; ++x)
			{
				m_indices.push_back(y       * (X_SEGMENTS + 1) + x);
				m_indices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
			}
		}
		else
		{
			for (int x = X_SEGMENTS; x >= 0; --x)
			{
				m_indices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
				m_indices.push_back(y       * (X_SEGMENTS + 1) + x);
			}
		}
		oddRow = !oddRow;
	}
}


void MeshData::addUnitQuad()
{
	GLVertexData l_VertexData_1;
	l_VertexData_1.m_pos = glm::vec3(1.0f, 1.0f, 0.0f);
	l_VertexData_1.m_texCoord = glm::vec2(1.0f, 1.0f);

	GLVertexData l_VertexData_2;
	l_VertexData_2.m_pos = glm::vec3(1.0f, -1.0f, 0.0f);
	l_VertexData_2.m_texCoord = glm::vec2(1.0f, 0.0f);

	GLVertexData l_VertexData_3;
	l_VertexData_3.m_pos = glm::vec3(-1.0f, -1.0f, 0.0f);
	l_VertexData_3.m_texCoord = glm::vec2(0.0f, 0.0f);

	GLVertexData l_VertexData_4;
	l_VertexData_4.m_pos = glm::vec3(-1.0f, 1.0f, 0.0f);
	l_VertexData_4.m_texCoord = glm::vec2(0.0f, 1.0f);

	m_vertices = { l_VertexData_1, l_VertexData_2, l_VertexData_3, l_VertexData_4 };
	m_indices = { 0, 1, 3, 1, 2, 3 };
}

void MeshData::setMeshDrawMethod(meshDrawMethod meshDrawMethod)
{
	m_meshDrawMethod = meshDrawMethod;
}

const meshDrawMethod & MeshData::getMeshDrawMethod() const
{
	return m_meshDrawMethod;
}

TextureData::TextureData()
{
}

TextureData::~TextureData()
{
}

void TextureData::initialize()
{
	m_GLTextureData.init(m_textureType, m_textureWrapMethod);
	m_GLTextureData.sendDataToGPU(m_textureType, m_textureIndex, m_textureFormat, m_textureWidth, m_textureHeight, m_textureRawData);
}

void TextureData::setup()
{
}

void TextureData::setup(textureType textureType, textureWrapMethod textureWrapMethod, int textureIndex, int textureFormat, int textureWidth, int textureHeight, void * textureData)
{
	m_textureType = textureType;
	m_textureWrapMethod = textureWrapMethod;
	m_textureIndex = textureIndex;
	m_textureFormat = textureFormat;
	m_textureWidth = textureWidth;
	m_textureHeight = textureHeight;
	m_textureRawData = textureData;
	setStatus(objectStatus::ALIVE);
}

void TextureData::update()
{
	if (getStatus() == objectStatus::ALIVE)
	{
		m_GLTextureData.draw(m_textureType);
	}
}

void TextureData::shutdown()
{
	m_GLTextureData.shutdown();
}


//GraphicData::GraphicData()
//{
//}
//
//GraphicData::~GraphicData()
//{
//}
//
//void GraphicData::init()
//{
//	if (m_visiblilityType == visiblilityType::SKYBOX)
//	{
//		m_meshData.addTestSkybox();
//	}
//	if (m_visiblilityType == visiblilityType::BILLBOARD)
//	{
//		m_meshData.addTestBillboard();
//	}
//	m_meshData.init();
//	m_meshData.sendDataToGPU();
//}
//
//void GraphicData::draw()
//{
//	for (size_t i = 0; i < m_textureData.size(); i++)
//	{
//		m_textureData[i].draw();
//	}
//	m_meshData.draw();
//}
//
//void GraphicData::shutdown()
//{
//}
//
//const visiblilityType & GraphicData::m_visiblilityType const
//{
//	return m_visiblilityType;
//}
//
//void GraphicData::setVisiblilityType(visiblilityType visiblilityType)
//{
//	m_visiblilityType = visiblilityType;
//}
//
//const textureWrapMethod & GraphicData::getTextureWrapMethod() const
//{
//	return m_textureWrapMethod;
//}
//
//void GraphicData::setTextureWrapMethod(textureWrapMethod textureWrapMethod)
//{
//	m_textureWrapMethod = textureWrapMethod;
//}
//
//MeshData & GraphicData::getMeshData()
//{
//	return m_meshData;
//}
//
//std::vector<TextureData>& GraphicData::getTextureData()
//{
//	return m_textureData;
//}

//ShadowMapData::ShadowMapData()
//{
//}
//
//ShadowMapData::~ShadowMapData()
//{
//}
//
//void ShadowMapData::init()
//{
//	//generate depth map frame buffer
//	glGenFramebuffers(1, &depthMapFBO);
//
//	//generate depth map texture
//	glGenTextures(1, &m_textureID);
//	glBindTexture(GL_TEXTURE_2D, m_textureID);
//	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, m_shadowMapWidth, m_shadowMapHeight, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
//
//	//bind frame buffer
//	glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
//	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_textureID, 0);
//
//	//no need for color
//	glDrawBuffer(GL_NONE);
//	glReadBuffer(GL_NONE);
//	glBindFramebuffer(GL_FRAMEBUFFER, 0);
//}
//
//void ShadowMapData::draw()
//{
//	glViewport(0, 0, m_shadowMapWidth, m_shadowMapHeight);
//	glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
//	glClear(GL_DEPTH_BUFFER_BIT);
//
//	switch (m_shadowProjectionType)
//	{
//	case shadowProjectionType::ORTHOGRAPHIC: m_projectionMatrix = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, 1.0f, 10.0f); break;
//	case shadowProjectionType::PERSPECTIVE: m_projectionMatrix = glm::perspective(glm::radians(45.0f), (GLfloat)m_shadowMapWidth / (GLfloat)m_shadowMapHeight, 1.0f, 100.0f); break;
//	}
//
//
//	// @TODO: finish shadow map drawing
//	//glBindFramebuffer(GL_FRAMEBUFFER, 0);
//}
//
//void ShadowMapData::shutdown()
//{
//}
//
//void ShadowMapData::setShadowProjectionType(shadowProjectionType shadowProjectionType)
//{
//	m_shadowProjectionType = shadowProjectionType;
//}
//
//void ShadowMapData::getProjectionMatrix(glm::mat4 & projectionMatrix)
//{
//	projectionMatrix = m_projectionMatrix;
//}

Material::Material()
{
}

Material::~Material()
{
}

void Material::initialize()
{
}

void Material::update()
{
}

void Material::shutdown()
{
}

void Material::addTextureData(textureType textureType, textureDataID textureDataID)
{
	auto l_exsistedTextureData = m_textureDataMap.find(textureType);
	if (l_exsistedTextureData != m_textureDataMap.end())
	{
		l_exsistedTextureData->second = textureDataID;
	}
	else
	{
		m_textureDataMap.emplace(textureType, textureDataID);
	}
}
