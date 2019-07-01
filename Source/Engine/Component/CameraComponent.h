#pragma once
#include "../Common/InnoComponent.h"
#include "../Common/InnoMath.h"

class CameraComponent : public InnoComponent
{
public:
	CameraComponent() {};
	~CameraComponent() {};

	mat4 m_projectionMatrix;
	Frustum m_frustum;
	Ray m_rayOfEye;
	float m_FOVX = 0.0f;
	float m_widthScale = 0.0f;
	float m_heightScale = 0.0f;
	float m_zNear = 0.0f;
	float m_zFar = 0.0f;
	float m_WHRatio = 0.0;
};
