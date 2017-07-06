#pragma once
#include "IGameEntity.h"

class CameraData
{
public:
	CameraData();
	~CameraData();

	const Vec3f yAxis = Vec3f(0.0f, 1.0f, 0.0f);

	void addCameraData(float fov, float aspectRatio, float zNear, float zFar);
	Mat4f* getProjectionMatrix();
	Mat4f getViewProjectionMatrix(IGameEntity* parent);

private:
	Mat4f m_projectionMatrix;
};

class CameraComponent : public IGameEntity
{
public:
	CameraComponent();
	~CameraComponent();

	Mat4f* getProjectionMatrix();
	Mat4f getViewProjectionMatrix();

private:
	CameraData m_cameraData;
	void init() override;
	void update() override;
	void shutdown() override;
};
