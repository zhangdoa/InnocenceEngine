#include "GLMesh.h"

void GLMesh::initialize()
{
	BaseMesh::initialize();
	glGenVertexArrays(1, &m_VAO);
	glGenBuffers(1, &m_VBO);
	glGenBuffers(1, &m_IBO);

	std::vector<float> l_verticesBuffer;
	auto& l_vertices = m_vertices;
	auto& l_indices = m_indices;

	if (m_meshType == meshType::TWO_DIMENSION)
	{
		std::for_each(l_vertices.begin(), l_vertices.end(), [&](Vertex val)
		{
			l_verticesBuffer.emplace_back((float)val.m_pos.x);
			l_verticesBuffer.emplace_back((float)val.m_pos.y);
			l_verticesBuffer.emplace_back((float)val.m_pos.z);
			l_verticesBuffer.emplace_back((float)val.m_texCoord.x);
			l_verticesBuffer.emplace_back((float)val.m_texCoord.y);
		});
	}
	else
	{
		std::for_each(l_vertices.begin(), l_vertices.end(), [&](Vertex val)
		{
			l_verticesBuffer.emplace_back((float)val.m_pos.x);
			l_verticesBuffer.emplace_back((float)val.m_pos.y);
			l_verticesBuffer.emplace_back((float)val.m_pos.z);
			l_verticesBuffer.emplace_back((float)val.m_texCoord.x);
			l_verticesBuffer.emplace_back((float)val.m_texCoord.y);
			l_verticesBuffer.emplace_back((float)val.m_normal.x);
			l_verticesBuffer.emplace_back((float)val.m_normal.y);
			l_verticesBuffer.emplace_back((float)val.m_normal.z);
		});
	}

	glBindVertexArray(m_VAO);

	glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
	glBufferData(GL_ARRAY_BUFFER, l_verticesBuffer.size() * sizeof(float), &l_verticesBuffer[0], GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_IBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, l_indices.size() * sizeof(unsigned int), &l_indices[0], GL_STATIC_DRAW);

	if (m_meshType == meshType::TWO_DIMENSION)
	{
		// position attribute, 1st attribution with 3 * sizeof(float) bits of data
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);

		// texture attribute, 2nd attribution with 2 * sizeof(float) bits of data
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
	}
	else
	{
		// position attribute, 1st attribution with 3 * sizeof(float) bits of data
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);

		// texture attribute, 2nd attribution with 2 * sizeof(float) bits of data
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));

		// normal coord attribute, 3rd attribution with 3 * sizeof(float) bits of data
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(5 * sizeof(float)));
	}

	m_objectStatus = objectStatus::ALIVE;
}

void GLMesh::draw()
{
	if (getStatus() == objectStatus::ALIVE)
	{
		glBindVertexArray(m_VAO);

		glDrawElements(GL_TRIANGLES + (int)m_meshDrawMethod, m_indices.size(), GL_UNSIGNED_INT, 0);
	}
}

void GLMesh::shutdown()
{
	glDeleteVertexArrays(1, &m_VAO);
	glDeleteBuffers(1, &m_VBO);
	glDeleteBuffers(1, &m_IBO);

	m_objectStatus = objectStatus::SHUTDOWN;
}