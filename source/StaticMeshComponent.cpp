#include "StaticMeshComponent.h"



StaticMeshComponent::StaticMeshComponent()
{
	init();
}


StaticMeshComponent::~StaticMeshComponent()
{
}

void StaticMeshComponent::init()
{
	_mesh = new Mesh();
}

void StaticMeshComponent::render(Shader * shader, RenderingEngine * renderingEngine)
{
	shader->bind();
	_mesh->draw();
}
