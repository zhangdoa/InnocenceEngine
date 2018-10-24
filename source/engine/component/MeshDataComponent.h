#pragma once
#include "BaseComponent.h"

class MeshDataComponent : public BaseComponent
{
public:
	MeshDataComponent() {};
	~MeshDataComponent() {};

	meshType m_meshType = meshType::NORMAL;
	std::vector<Vertex> m_vertices;
	std::vector<unsigned int> m_indices;

	meshDrawMethod m_meshDrawMethod = meshDrawMethod::TRIANGLE;
	bool m_calculateNormals = false;
	bool m_calculateTangents = false;
};

