#pragma once
#include "../interface/IGameEntity.h"
#include "../manager/InputManager.h"
#include "../manager/WindowManager.h"

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
	//TODO: extract CameraData class to CameraComponent class
	CameraData m_cameraData;
	void init() override;
	void update() override;
	void shutdown() override;

	int l_mouseRightResult = 0;
	glm::vec2 l_mousePosition;
	int l_keyWResult = 0;
	int l_keySResult = 0;
	int l_keyAResult = 0;
	int l_keyDResult = 0;
};
