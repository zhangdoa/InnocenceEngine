#pragma once
#include "../common/InnoType.h"

class CameraComponent
{
public:
	CameraComponent() {};
	~CameraComponent() {};

	ObjectStatus m_objectStatus = ObjectStatus::SHUTDOWN;
	EntityID m_parentEntity;

	float m_FOVX = 0.0;
	float m_WHRatio = 0.0;
	float m_zNear = 0.0;
	float m_zFar = 0.0;

	Ray m_rayOfEye;
	Frustum m_frustum;
	bool m_drawRay = false;
	bool m_drawFrustum = false;
	bool m_drawAABB = false;
	mat4 m_projectionMatrix;
	MeshDataComponent* m_frustumMDC = nullptr;
};
