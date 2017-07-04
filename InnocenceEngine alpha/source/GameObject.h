#pragma once
#include "stdafx.h"
#include "Math.h"
#include "Shader.h"
#include "RenderingEngine.h"

class GameComponent
{
public:
	GameComponent();
	~GameComponent();
	GameObject* getParent();
	void setParent(GameObject* parent);
	const std::string& getName();
	void setName(const std::string& name);
	Transform* getTransform();
	void input(float delta);
	void update(float delta);
	virtual void render(Shader* shader) {};
private:
	GameObject* _parent;
	std::string _name;
};

class GameObject
{
public:
	GameObject();
	~GameObject();

	
	void input(float delta);
	void update(float delta);
	void render(Shader* shader);

	GameObject* getParent();
	void setParent(GameObject* parent);
	const std::string& getName();
	void setName(const std::string& name);
	void addChind(GameObject* child);
	void addComponent(GameComponent* component);
	Transform* getTransform();
	RenderingEngine* getRenderingEngine();
	void setRenderingEngine(RenderingEngine* renderingEngine);

	bool hasTransformChanged();
	Mat4f caclTransformation();
	Vec3f caclTransformedPos();
	Quaternion caclTransformedRot();

private:
	GameObject* _parent;
	std::string _name;
	std::vector<GameObject*> _children;
	std::vector<GameComponent*> _components;
	Transform _transform;
	RenderingEngine* _renderingEngine;
};


