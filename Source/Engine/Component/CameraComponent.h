#pragma once
#include "../Common/InnoComponent.h"
#include "../Common/InnoMathHelper.h"

class CameraComponent : public InnoComponent
{
public:
	Mat4 m_projectionMatrix;
	Frustum m_frustum;
	Ray m_rayOfEye;
	float m_FOVX = 0.0f;
	float m_widthScale = 0.0f;
	float m_heightScale = 0.0f;
	float m_zNear = 0.0f;
	float m_zFar = 0.0f;
	float m_WHRatio = 0.0f;
	float m_aperture = 2.2f;
	float m_shutterTime = 1.0f / 2000.0f;
	float m_ISO = 100.0f;
};
