#pragma once
#include "IGameEntity.h"
#include "InputManager.h"
#include "WindowManager.h"

class CameraData
{
public:
	CameraData();
	~CameraData();

	void addCameraData(float fov, float aspectRatio, float zNear, float zFar);

	Mat4f getViewProjectionMatrix(BaseComponent* parent);

private:
	Mat4f m_projectionMatrix;
};

class CameraComponent : public BaseComponent
{
public:
	CameraComponent();
	~CameraComponent();

	enum moveDirection {FORWARD, BACKWARD, LEFT, RIGHT};

	Mat4f getViewProjectionMatrix();
	void move(moveDirection moveDirection);

private:
	CameraData m_cameraData;
	void init() override;
	void update() override;
	void shutdown() override;
};
