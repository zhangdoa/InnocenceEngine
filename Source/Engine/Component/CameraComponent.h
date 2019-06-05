#pragma once
#include "../common/InnoComponent.h"

class CameraComponent : public InnoComponent
{
public:
	CameraComponent() {};
	~CameraComponent() {};

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
