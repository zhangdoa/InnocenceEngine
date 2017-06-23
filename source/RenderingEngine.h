#pragma once
#include "stdafx.h"
#include "Math.h"

class Transform;
class GameObject;
class GameComponent;
class BaseLightComponent;

class Camera
{
public:
	Camera();
	~Camera();
	const Vec3f yAxis = Vec3f(0.0f, 1.0f, 0.0f);
	void setProjection(Mat4f* projection);
	void setTransform(Transform* transform);
	Mat4f getViewProjection();
private:
	Transform* _transform;
	Mat4f* _projection;
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

	Camera* getMainCamera();
	void setMainCamera(Camera* mainCamera);
	Vec3f* getAmbientLight();

private:
	std::vector<BaseLightComponent*> _lights;
	Camera* _mainCamera;
	Vec3f _ambientLight;
};

