#include "../../main/stdafx.h"
#include "GraphicData.h"

StaticMeshData::StaticMeshData()
{
}


StaticMeshData::~StaticMeshData()
{
}

void StaticMeshData::init()
{
	m_GLMeshData.init();
}

void StaticMeshData::update()
{
	m_GLMeshData.update(m_intices);
}

void StaticMeshData::shutdown()
{
	m_GLMeshData.shutdown();
}

void StaticMeshData::loadData(const std::string & meshFileName)
{
	bool isProcessingVerticesData;
	std::ifstream meshFile;
	meshFile.open(("../res/models/" + meshFileName).c_str());

	std::string line;

	if (meshFile.is_open())
	{
		while (meshFile.good() && meshFile.is_open())
		{
			getline(meshFile, line);
			if (line.find("#INNO_MESH_EOF#") == std::string::npos)
			{
				if (line.find("#INNO_MESH_VERTICES#") != std::string::npos)
				{
					isProcessingVerticesData = true;
					getline(meshFile, line);
				}
				if (line.find("#INNO_MESH_INDICES#") != std::string::npos)
				{
					isProcessingVerticesData = false;
					getline(meshFile, line);
				}
				// process vertices data
				if (isProcessingVerticesData)
				{
					// check delimer per-parse and add them to a string array
					std::vector<std::string> meshDataLineArray;
					std::string delimer = " ";
					std::size_t start = 0;
					std::size_t end = 0;

					while ((end = line.find(delimer, start)) != std::string::npos)
					{
						meshDataLineArray.push_back(line.substr(start, end - start));
						start = end + 1;
					}

					//add vertex data from previous processed string array
					VertexData vertexData;

					for (auto i = 0; i < meshDataLineArray.size(); i += meshDataLineArray.size())
					{
						vertexData.m_pos.x = std::stof(meshDataLineArray[i]);
						vertexData.m_pos.y = std::stof(meshDataLineArray[i + 1]);
						vertexData.m_pos.z = std::stof(meshDataLineArray[i + 2]);
						vertexData.m_texCoord.x = std::stof(meshDataLineArray[i + 3]);
						vertexData.m_texCoord.y = std::stof(meshDataLineArray[i + 4]);
						vertexData.m_normal.x = std::stof(meshDataLineArray[i + 5]);
						vertexData.m_normal.y = std::stof(meshDataLineArray[i + 6]);
						vertexData.m_normal.z = std::stof(meshDataLineArray[i + 7]);
					}

					m_vertices.emplace_back(vertexData);
				}
				else
				{
					unsigned int index;
					index = std::stoi(line);
					m_intices.emplace_back(index);
				}
			}
			else
			{
				meshFile.close();
			}
		}
		LogManager::getInstance().printLog("Mesh loaded.");
	}
	else
	{
		LogManager::getInstance().printLog("Error: StaticMeshData: Cannot open mesh file!");
	}
	m_GLMeshData.addGLMeshData(m_vertices, m_intices, false);
}

void StaticMeshData::addTestCube()
{
	VertexData l_VertexData_1;
	l_VertexData_1.m_pos = glm::vec3(0.5f, 0.5f, 0.5f);
	l_VertexData_1.m_texCoord = glm::vec2(1.0f, 1.0f);
	l_VertexData_1.m_normal = glm::vec3(0.0f, 0.0f, 1.0f);

	VertexData l_VertexData_2;
	l_VertexData_2.m_pos = glm::vec3(0.5f, -0.5f, 0.5f);
	l_VertexData_2.m_texCoord = glm::vec2(1.0f, 0.0f);
	l_VertexData_2.m_normal = glm::vec3(0.0f, 0.0f, 1.0f);

	VertexData l_VertexData_3;
	l_VertexData_3.m_pos = glm::vec3(-0.5f, -0.5f, 0.5f);
	l_VertexData_3.m_texCoord = glm::vec2(0.0f, 0.0f);
	l_VertexData_3.m_normal = glm::vec3(0.0f, 0.0f, 1.0f);

	VertexData l_VertexData_4;
	l_VertexData_4.m_pos = glm::vec3(-0.5f, 0.5f, 0.5f);
	l_VertexData_4.m_texCoord = glm::vec2(0.0f, 1.0f);
	l_VertexData_4.m_normal = glm::vec3(0.0f, 0.0f, 1.0f);

	VertexData l_VertexData_5;
	l_VertexData_5.m_pos = glm::vec3(0.5f, 0.5f, -0.5f);
	l_VertexData_5.m_texCoord = glm::vec2(1.0f, 1.0f);
	l_VertexData_5.m_normal = glm::vec3(0.0f, 0.0f, -1.0f);

	VertexData l_VertexData_6;
	l_VertexData_6.m_pos = glm::vec3(0.5f, -0.5f, -0.5f);
	l_VertexData_6.m_texCoord = glm::vec2(1.0f, 0.0f);
	l_VertexData_6.m_normal = glm::vec3(0.0f, 0.0f, -1.0f);

	VertexData l_VertexData_7;
	l_VertexData_7.m_pos = glm::vec3(-0.5f, -0.5f, -0.5f);
	l_VertexData_7.m_texCoord = glm::vec2(0.0f, 0.0f);
	l_VertexData_7.m_normal = glm::vec3(0.0f, 0.0f, -1.0f);

	VertexData l_VertexData_8;
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

	m_GLMeshData.addGLMeshData(m_vertices, m_intices, false);
}

void StaticMeshData::addTestSkybox()
{
	VertexData l_VertexData_1;
	l_VertexData_1.m_pos = glm::vec3(1.0f, 1.0f, 1.0f);

	VertexData l_VertexData_2;
	l_VertexData_2.m_pos = glm::vec3(1.0f, -1.0f, 1.0f);

	VertexData l_VertexData_3;
	l_VertexData_3.m_pos = glm::vec3(-1.0f, -1.0f, 1.0f);

	VertexData l_VertexData_4;
	l_VertexData_4.m_pos = glm::vec3(-1.0f, 1.0f, 1.0f);

	VertexData l_VertexData_5;
	l_VertexData_5.m_pos = glm::vec3(1.0f, 1.0f, -1.0f);

	VertexData l_VertexData_6;
	l_VertexData_6.m_pos = glm::vec3(1.0f, -1.0f, -1.0f);

	VertexData l_VertexData_7;
	l_VertexData_7.m_pos = glm::vec3(-1.0f, -1.0f, -1.0f);

	VertexData l_VertexData_8;
	l_VertexData_8.m_pos = glm::vec3(-1.0f, 1.0f, -1.0f);

	m_vertices = { l_VertexData_1, l_VertexData_2, l_VertexData_3, l_VertexData_4, l_VertexData_5, l_VertexData_6, l_VertexData_7, l_VertexData_8 };

	m_intices = { 0, 1, 3, 1, 2, 3,
		4, 5, 0, 5, 1, 0,
		7, 6, 4, 6, 5, 4,
		3, 2, 7, 2, 6 ,7,
		4, 0, 7, 0, 3, 7,
		1, 5, 2, 5, 6, 2 };

	m_GLMeshData.addGLMeshData(m_vertices, m_intices, false);
}

TextureData::TextureData()
{
}

TextureData::~TextureData()
{
}

void TextureData::init()
{
	m_GLTextureData.init();
}

void TextureData::update()
{
	m_GLTextureData.update();
}

void TextureData::shutdown()
{
	m_GLTextureData.shutdown();
}

void TextureData::loadTexture(const std::string & filePath) const
{
	m_GLTextureData.loadTexture(filePath);
}

CubemapData::CubemapData()
{
}

CubemapData::~CubemapData()
{
}

void CubemapData::init()
{
	m_GLCubemapData.init();
}

void CubemapData::update()
{
	m_GLCubemapData.update();
}

void CubemapData::shutdown()
{
	m_GLCubemapData.shutdown();
}


void CubemapData::loadCubemap(const std::vector<std::string> & filePath) const
{
	m_GLCubemapData.loadCubemap(filePath);
}

