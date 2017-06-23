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

void StaticMeshComponent::render(Shader * shader)
{
	shader->bind();
	shader->updateUniforms(getParent(), _material);
	_mesh->draw();
}