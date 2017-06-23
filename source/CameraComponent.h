#pragma once
#include "GameObject.h"

class CameraComponent: public GameComponent 
{
public:
	CameraComponent();
	CameraComponent(float fov, float aspectRatio, float zNear, float zFar);
	~CameraComponent();
	const Vec3f yAxis = Vec3f(0.0f, 1.0f, 0.0f);

private:
	Camera _camera;
	Mat4f _projection;

};

