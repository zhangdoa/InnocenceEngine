#pragma once
#include "../common/InnoType.h"

class CameraComponent
{
public:
	CameraComponent() {};
	~CameraComponent() {};

	objectStatus m_objectStatus = objectStatus::SHUTDOWN;
	EntityID m_parentEntity;

	double m_FOVX = 0.0;
	double m_WHRatio = 0.0;
	double m_zNear = 0.0;
	double m_zFar = 0.0;

	Ray m_rayOfEye;
	bool m_drawRay = false;
	bool m_drawFrustum = false;
	bool m_drawAABB = false;
	EntityID m_FrustumMeshID = 0;
	std::vector<Vertex> m_frustumVertices;
	std::vector<unsigned int> m_frustumIndices;
	AABB m_AABB;
	EntityID m_AABBMeshID = 0;
	mat4 m_projectionMatrix;
};
