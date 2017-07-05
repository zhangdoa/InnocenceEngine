//#include "StaticMeshComponent.h"
//
//
//
//StaticMeshComponent::StaticMeshComponent()
//{
//	init();
//}
//
//
//StaticMeshComponent::~StaticMeshComponent()
//{
//}
//
//void StaticMeshComponent::init()
//{
//	_mesh = new Mesh();
//}
//
//void StaticMeshComponent::render(Shader * shader)
//{
//	//std::cout << getName() + " is rendering with " + shader->getName() << std::endl;
//	shader->bind();
//	shader->updateUniforms(getParent(), _material);
//	_mesh->draw();
//}