#pragma once
#include "stdafx.h"
#include "Math.h"
#include "Shader.h"
#include "RenderingEngine.h"

class Transform
{
public:
	Transform();
	~Transform();
	void update();
	void setParent(Transform* parent);
	void rotate(Vec3f axis, float angle);
	bool hasChanged();
	Mat4f getTransformation();
	Mat4f getParentMatrix();
	Vec3f getTransformedPos();
	Quaternion getTransformedRot();

private:
	Transform* _parent;
	Mat4f* _parentMat;

	Vec3f* _pos;
	Quaternion* _rot;
	Vec3f* _scale;

	Vec3f* _oldPos;
	Quaternion* _oldRot;
	Vec3f* _oldScale;
};

class GameComponent
{
public:
	GameComponent();
	~GameComponent();
	GameObject* getParent();
	void setParent(GameObject* parent);
	Transform* getTransform();
	void input(float delta);
	void update(float delta);
	virtual void render(Shader* shader) {};
private:
	GameObject* _parent;
};

class GameObject
{
public:
	GameObject();
	~GameObject();

	void addChind(GameObject* child);
	void addComponent(GameComponent* component);
	void input(float delta);
	void update(float delta);
	void render(Shader* shader);
	Transform* getTransform();
	RenderingEngine* getRenderingEngine();
private:
	std::vector<GameObject*> _children;
	std::vector<GameComponent*> _components;
	Transform* _transform;
	RenderingEngine* _renderingEngine;

};


