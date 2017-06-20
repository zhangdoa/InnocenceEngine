#pragma once
#include "stdafx.h"
#include "Vec2f.h"
#include "Vec3f.h"

class GameObject;
class CameraComponent;
class BaseLightComponent;
class RenderingEngine
{
public:
	RenderingEngine();
	~RenderingEngine();
	void init();
	void update();
	void render(GameObject* object);
	void shutdown();
	void addCamera(CameraComponent* camera);
	void addLight(BaseLightComponent* activeLight);
private:
	CameraComponent* _mainCamera;
	Vec3f ambientLight;
	std::vector<BaseLightComponent*> _lights;
	BaseLightComponent* _activeLight;
};

