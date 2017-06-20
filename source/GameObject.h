#pragma once
#include "stdafx.h"
#include "Transform.h"
#include "Shader.h"
#include "RenderingEngine.h"

class GameComponent;
class GameObject
{
public:
	GameObject();
	~GameObject();

	void addChind(GameObject* child);
	void addComponent(GameComponent* component);
	void input(float delta);
	void update(float delta);
	void render(Shader* shader, RenderingEngine* renderingEngine);
	Transform* getTransform();
	void addToRenderingEngine(RenderingEngine* renderingEngine);
private:
	std::vector<GameObject*> _children;
	std::vector<GameComponent*> _components;
	Transform* _transform;

};


