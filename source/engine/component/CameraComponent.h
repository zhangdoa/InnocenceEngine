#pragma once
#include "../common/InnoType.h"

class CameraComponent
{
public:
	CameraComponent() {};
	~CameraComponent() {};

	ObjectStatus m_objectStatus = ObjectStatus::SHUTDOWN;
	EntityID m_parentEntity;
	unsigned int m_UUID = 0;

	float m_FOVX = 0.0f;
	float m_widthScale = 0.0f;
	float m_heightScale = 0.0f;
	float m_zNear = 0.0f;
	float m_zFar = 0.0f;

	float m_WHRatio = 0.0;
	Ray m_rayOfEye;
	Frustum m_frustum;
	mat4 m_projectionMatrix;
};
