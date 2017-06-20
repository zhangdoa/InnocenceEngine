#pragma once
#include "stdafx.h"
#include "GameComponent.h"
#include "Vec2f.h"
#include "Vec3f.h"
#include "Mat4f.h"

class CameraComponent: public GameComponent 
{
public:
	CameraComponent(float fov, float aspectRatio, float zNear, float zFar);
	~CameraComponent();
	const Vec3f yAxis = Vec3f(0.0f, 1.0f, 0.0f);

	Mat4f getViewProjection();
	void addToRenderingEngine(RenderingEngine* renderingEngine) override;

private:
	Mat4f projection;

};

