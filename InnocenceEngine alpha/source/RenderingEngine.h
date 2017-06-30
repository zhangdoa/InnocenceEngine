#pragma once
#include "stdafx.h"
#include "Math.h"
#include "Shader.h"


class Transform;
class GameObject;
class GameComponent;
class CameraComponent;
class BaseLightComponent;

class Camera
{
public:
	Camera();
	Camera(float fov, float aspectRatio, float zNear, float zFar);
	~Camera();
	const Vec3f yAxis = Vec3f(0.0f, 1.0f, 0.0f);
	Mat4f getViewProjection();

private:
	Mat4f _projection;

};

class RenderingEngine
{
public:
	RenderingEngine();
	~RenderingEngine();

	void init();
	void update();
	void render(GameObject* object);
	void shutdown();

	void addLight(BaseLightComponent* activeLight);

	CameraComponent* getMainCamera();
	void setMainCamera(CameraComponent* camera);
	const Vec3f& getAmbientLight();

private:
	CameraComponent* _mainCamera;
	BaseLightComponent* _activeLight;
	std::vector<BaseLightComponent*> _lights;
	Vec3f _ambientLight;
	BasicShaderWrapper basicShader;
	//ForwardAmbientShaderWrapper forwardAmbientShader;
};

