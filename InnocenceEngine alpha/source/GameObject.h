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
	void rotate(Vec3f axis, float angle);

	const Vec3f& getPos();
	const Quaternion& getRot();
	const Vec3f& getScale();

	void setPos(const Vec3f& pos);
	void setRot(const Quaternion& rot);
	void setScale(const Vec3f& scale);

	const Vec3f& getOldPos();
	const Quaternion& getOldRot();
	const Vec3f& getOldScale();

private:

	Vec3f _pos;
	Quaternion _rot;
	Vec3f _scale;

	Vec3f _oldPos;
	Quaternion _oldRot;
	Vec3f _oldScale;
};

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


