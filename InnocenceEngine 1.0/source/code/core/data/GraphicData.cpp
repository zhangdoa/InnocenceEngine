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
	setStatus(objectStatus::ALIVE);
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

//Material::Material()
//{
//}
//
//Material::~Material()
//{
//}
//
//void Material::setup()
//{
//}
//
//void Material::initialize()
//{
//}
//
//void Material::update()
//{
//}
//
//void Material::shutdown()
//{
//}
//
//void Material::addTextureData(textureType textureType, textureDataID textureDataID)
//{
//	auto l_exsistedTextureData = m_textureDataMap.find(textureType);
//	if (l_exsistedTextureData != m_textureDataMap.end())
//	{
//		l_exsistedTextureData->second = textureDataID;
//	}
//	else
//	{
//		m_textureDataMap.emplace(textureType, textureDataID);
//	}
//}

void IMesh::setup()
{
	this->setup(meshDrawMethod::TRIANGLE, false, false);
}

void IMesh::setup(meshDrawMethod meshDrawMethod, bool calculateNormals, bool calculateTangents)
{
	m_meshDrawMethod = meshDrawMethod;
	m_calculateNormals = calculateNormals;
	m_calculateTangents = calculateTangents;
}

void IMesh::addVertices(IVertex & IVertex)
{
}

void IMesh::addVertices(float pos_x, float pos_y, float pos_z, float texCoord_x, float texCoord_y, float normal_x, float normal_y, float normal_z)
{
}

void IMesh::addUnitCube()
{
}

void IMesh::addUnitSphere()
{
}

void IMesh::addUnitQuad()
{
}

meshID IMesh::getMeshDataID() const
{
	return this->getGameObjectID();
}

void GLMesh::initialize()
{
	glGenVertexArrays(1, &m_VAO);
	glGenBuffers(1, &m_VBO);
	glGenBuffers(1, &m_IBO);

	std::vector<float> l_verticesBuffer;

	std::for_each(m_vertices.begin(), m_vertices.end(), [&](IVertex val)
	{
		l_verticesBuffer.emplace_back(val.m_pos.x);
		l_verticesBuffer.emplace_back(val.m_pos.y);
		l_verticesBuffer.emplace_back(val.m_pos.z);
		l_verticesBuffer.emplace_back(val.m_texCoord.x);
		l_verticesBuffer.emplace_back(val.m_texCoord.y);
		l_verticesBuffer.emplace_back(val.m_normal.x);
		l_verticesBuffer.emplace_back(val.m_normal.y);
		l_verticesBuffer.emplace_back(val.m_normal.z);
	});

	glBindVertexArray(m_VAO);

	glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
	glBufferData(GL_ARRAY_BUFFER, l_verticesBuffer.size() * sizeof(float), &l_verticesBuffer[0], GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_IBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_indices.size() * sizeof(float), &m_indices[0], GL_STATIC_DRAW);

	// position attribute, 1st attribution with 3 * sizeof(float) bits of data
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);

	// texture attribute, 2nd attribution with 2 * sizeof(float) bits of data
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));

	// normal coord attribute, 3rd attribution with 3 * sizeof(float) bits of data
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(5 * sizeof(float)));

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	setStatus(objectStatus::ALIVE);
}

void GLMesh::update()
{
	glBindVertexArray(m_VAO);
	glDrawElements(GL_TRIANGLES + (int)m_meshDrawMethod, m_indices.size(), GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);
	glActiveTexture(GL_TEXTURE0);
}

void GLMesh::shutdown()
{
	glDeleteVertexArrays(1, &m_VAO);
	glDeleteBuffers(1, &m_VBO);
	glDeleteBuffers(1, &m_IBO);

	setStatus(objectStatus::SHUTDOWN);
}

void ITexture::setup()
{
	this->setup(textureType::DIFFUSE, textureWrapMethod::REPEAT, 0, 0, 0, 0, nullptr);
}

void ITexture::setup(textureType textureType, textureWrapMethod textureWrapMethod, int textureIndex, int textureFormat, int textureWidth, int textureHeight, void * textureData)
{
	m_textureType = textureType;
	m_textureWrapMethod = textureWrapMethod;
	m_textureIndex = textureIndex;
	m_textureFormat = textureFormat;
	m_textureWidth = textureWidth;
	m_textureHeight = textureHeight;
	m_textureRawData = textureData;
}

textureID ITexture::getTextureDataID() const
{
	return this->getGameObjectID();
}

void GLTexture::initialize()
{
	GLint l_textureWrapMethod;
	switch (m_textureWrapMethod)
	{
	case textureWrapMethod::REPEAT: l_textureWrapMethod = GL_REPEAT; break;
	case textureWrapMethod::CLAMPTOEDGE: l_textureWrapMethod = GL_CLAMP_TO_EDGE; break;
	}
	if (m_textureType == textureType::INVISIBLE)
	{
		return;
	}
	else if (m_textureType == textureType::CUBEMAP)
	{
		glGenTextures(1, &m_textureID);
		glBindTexture(GL_TEXTURE_CUBE_MAP, m_textureID);
		// set the texture wrapping parameters
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, l_textureWrapMethod);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, l_textureWrapMethod);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, l_textureWrapMethod);
		// set texture filtering parameters
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	}
	else
	{
		// @TODO: more general texture parameter
		glGenTextures(1, &m_textureID);
		glBindTexture(GL_TEXTURE_2D, m_textureID);
		// set the texture wrapping parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, l_textureWrapMethod);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, l_textureWrapMethod);
		// set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	}
	GLenum l_internalFormat;
	if (m_textureFormat == 1)
	{
		l_internalFormat = GL_RED;
	}
	else if (m_textureFormat == 3)
	{
		if (m_textureType == textureType::CUBEMAP)
		{
			l_internalFormat = GL_SRGB;
		}
		else
		{
			l_internalFormat = GL_RGB;
		}
	}
	else if (m_textureFormat == 4)
	{
		if (m_textureType == textureType::CUBEMAP)
		{
			l_internalFormat = GL_SRGB_ALPHA;
		}
		else
		{
			l_internalFormat = GL_RGBA;
		}
	}

	if (m_textureType == textureType::INVISIBLE)
	{
		return;
	}
	else if (m_textureType == textureType::CUBEMAP)
	{
		glBindTexture(GL_TEXTURE_CUBE_MAP, m_textureID);
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + m_textureIndex, 0, l_internalFormat, m_textureWidth, m_textureHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, m_textureRawData);
	}
	else
	{
		glBindTexture(GL_TEXTURE_2D, m_textureID);
		glTexImage2D(GL_TEXTURE_2D, 0, l_internalFormat, m_textureWidth, m_textureHeight, 0, l_internalFormat, GL_UNSIGNED_BYTE, m_textureRawData);
		glGenerateMipmap(GL_TEXTURE_2D);
	}

	setStatus(objectStatus::ALIVE);
}

void GLTexture::update()
{
	//@TODO: switch is too slow for CPU
	switch (m_textureType)
	{
	case textureType::INVISIBLE: break;
	case textureType::DIFFUSE:
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, m_textureID);
		break;
	case textureType::SPECULAR:
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, m_textureID);
		break;
	case textureType::NORMALS:
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, m_textureID);
		break;
	case textureType::AMBIENT:
		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_2D, m_textureID);
		break;
	case textureType::EMISSIVE:
		glActiveTexture(GL_TEXTURE4);
		glBindTexture(GL_TEXTURE_2D, m_textureID);
		break;
	case textureType::CUBEMAP:
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_CUBE_MAP, m_textureID);
		break;
	}
}

void GLTexture::shutdown()
{
	glDeleteTextures(1, &m_textureID);

	setStatus(objectStatus::SHUTDOWN);
}
