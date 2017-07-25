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

	void getViewProjectionMatrix(const BaseComponent& parent, glm::mat4& outViewProjectionMatrix) const;
	void getTranslatonMatrix(const BaseComponent& parent, glm::mat4& outTranslationMatrix) const;
	void getRotationMatrix(const BaseComponent& parent, glm::mat4& outRotationMatrix) const;
	void getProjectionMatrix(glm::mat4& outProjectionMatrix) const;

private:
	glm::mat4 m_projectionMatrix;
};

class CameraComponent : public BaseComponent
{
public:
	CameraComponent();
	~CameraComponent();

	enum moveDirection { FORWARD, BACKWARD, LEFT, RIGHT };

	void getViewProjectionMatrix(glm::mat4& outViewProjectionMatrix) const;
	void getTranslatonMatrix(glm::mat4& outTranslationMatrix) const;
	void getRotationMatrix(glm::mat4& outRotationMatrix) const;
	void getProjectionMatrix(glm::mat4& outProjectionMatrix) const;

	void move(moveDirection moveDirection);

private:
	CameraData m_cameraData;
	void init() override;
	void update() override;
	void shutdown() override;
};
