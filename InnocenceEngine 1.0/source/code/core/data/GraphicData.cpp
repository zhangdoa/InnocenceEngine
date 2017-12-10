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
	sendDataToGPU(false, false);
}

void MeshData::initialize(meshDrawMethod meshDrawMethod, bool calculateNormals, bool calculateTangents)
{
	m_meshDrawMethod = meshDrawMethod;
	m_GLMeshData.init();
	sendDataToGPU(calculateNormals, calculateTangents);
}

void MeshData::update()
{
	m_GLMeshData.draw(m_indices, m_meshDrawMethod);
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

void MeshData::sendDataToGPU(bool calculateNormals = false, bool calculateTangents = false)
{
	if (calculateNormals)
	{
		for (auto& l_vertices : m_vertices)
		{
			l_vertices.m_normal = l_vertices.m_pos;
		}
	}
	if (calculateTangents)
	{
		// @TODO: correct tangent calculation
		for (size_t i = 0; i < m_vertices.size(); i += 3) {
			int i0 = m_indices[i];
			int i1 = m_indices[i + 1];
			int i2 = m_indices[i + 2];

			glm::vec3 l_edge1 = m_vertices[i1].m_pos - m_vertices[i0].m_pos;
			glm::vec3 l_edge2 = m_vertices[i2].m_pos - m_vertices[i0].m_pos;

			double DeltaU1 = m_vertices[i1].m_texCoord.x -  m_vertices[i0].m_texCoord.x;
			double DeltaV1 =  m_vertices[i1].m_texCoord.y -  m_vertices[i0].m_texCoord.y;
			double DeltaU2 =  m_vertices[i2].m_texCoord.x -  m_vertices[i0].m_texCoord.x;
			double DeltaV2 =  m_vertices[i2].m_texCoord.y -  m_vertices[i0].m_texCoord.y;

			double f = DeltaU1 * DeltaV2 - DeltaU2 * DeltaV1;

			if (f == 0.0)
			{
				f = 1.0;
			}
			else
			{
				f = 1.0 / f;
			}

			glm::vec3 l_tangent;

			double ftx = DeltaV2 * l_edge1.x - DeltaV1 * l_edge2.x;
			double fty = DeltaV2 * l_edge1.y - DeltaV1 * l_edge2.y;
			double ftz = DeltaV2 * l_edge1.z - DeltaV1 * l_edge2.z;

			l_tangent.x = f * ftx;
			l_tangent.y = f * fty;
			l_tangent.z = f * ftz;

			m_vertices[i0].m_tangent += l_tangent;
			m_vertices[i1].m_tangent += l_tangent;
			m_vertices[i2].m_tangent += l_tangent;
		}
		for (auto& l_vertices : m_vertices)
		{
			l_vertices.m_tangent = glm::normalize(glm::cross(glm::cross(l_vertices.m_normal, l_vertices.m_tangent), l_vertices.m_normal));
			l_vertices.m_bitangent = glm::normalize(glm::cross(l_vertices.m_normal, l_vertices.m_tangent));
		}
	}
	m_GLMeshData.sendDataToGPU(m_vertices, m_indices);
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
		l_vertexData.m_tangent = glm::normalize(glm::cross(glm::vec3(0.0, 0.0, 1.0), l_vertexData.m_normal));
		l_vertexData.m_bitangent = glm::normalize(glm::cross(l_vertexData.m_tangent, l_vertexData.m_normal));
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
			l_VertexData.m_tangent = glm::normalize(glm::cross(glm::vec3(0.0, 0.0, 1.0), l_VertexData.m_normal));
			l_VertexData.m_bitangent = glm::normalize(glm::cross(l_VertexData.m_tangent, l_VertexData.m_normal));
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
}

void TextureData::update()
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