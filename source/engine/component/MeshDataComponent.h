#pragma once
#include "BaseComponent.h"

class MeshDataComponent : public BaseComponent
{
public:
	MeshDataComponent() { m_meshID = std::rand(); };
	~MeshDataComponent() {};

	meshID m_meshID = 0;
	meshType m_meshType = meshType::NORMAL;
	std::vector<Vertex> m_vertices;
	std::vector<unsigned int> m_indices;

	meshDrawMethod m_meshDrawMethod = meshDrawMethod::TRIANGLE;
	bool m_calculateNormals = false;
	bool m_calculateTangents = false;
};

