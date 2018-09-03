#pragma once
#include "BaseComponent.h"

class CameraComponent : public BaseComponent
{
public:
	CameraComponent() {};
	~CameraComponent() {};

	double m_FOVX;
	double m_WHRatio;
	double m_zNear;
	double m_zFar;

	Ray m_rayOfEye;
	bool m_drawRay = false;
	bool m_drawFrustum = false;
	bool m_drawAABB = false;
	meshID m_FrustumMeshID;
	std::vector<Vertex> m_frustumVertices;
	std::vector<unsigned int> m_frustumIndices;
	AABB m_AABB;
	meshID m_AABBMeshID;
	mat4 m_projectionMatrix;
};
