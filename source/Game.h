#pragma once
#include "GameObject.h"
#include "RenderingEngine.h"
class Game
{
public:
	Game();
	~Game();
	void init();
	void input(float delta);
	void update(float delta);
	void render(RenderingEngine* renderingEngine);
	void shutdown();
	void addObject(GameObject* object);
private:
	GameObject* _root;
};

