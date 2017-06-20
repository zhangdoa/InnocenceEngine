#pragma once
#include "GameComponent.h"
#include "Mesh.h"
class StaticMeshComponent: public GameComponent
{
public:
	StaticMeshComponent();
	~StaticMeshComponent();
	void init();
	void render(Shader* shader, RenderingEngine* renderingEngine) override;
private:
	Mesh* _mesh;
};

