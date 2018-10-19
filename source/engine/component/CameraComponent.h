#pragma once
#include "BaseComponent.h"

class CameraComponent : public BaseComponent
{
public:
	CameraComponent() {};
	~CameraComponent() {};

	double m_FOVX = 0.0;
	double m_WHRatio = 0.0;
	double m_zNear = 0.0;
	double m_zFar = 0.0;

	Ray m_rayOfEye;
	bool m_drawRay = false;
	bool m_drawFrustum = false;
	bool m_drawAABB = false;
	meshID m_FrustumMeshID = 0;
	std::vector<Vertex> m_frustumVertices;
	std::vector<unsigned int> m_frustumIndices;
	AABB m_AABB;
	meshID m_AABBMeshID = 0;
	mat4 m_projectionMatrix;
};
