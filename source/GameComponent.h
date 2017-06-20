#pragma once
#include "stdafx.h"
#include "GameObject.h"

class GameComponent
{
public:
	GameComponent();
	~GameComponent();
	void setParent(GameObject* parent);
	Transform* getTransform();
	void input(float delta);
	void update(float delta);
	virtual void render(Shader* shader, RenderingEngine* renderingEngine) {};
	virtual void addToRenderingEngine(RenderingEngine* renderingEngine) {};
private:
	GameObject* _parent;
};
