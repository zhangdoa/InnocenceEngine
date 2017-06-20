#include "GameObject.h"
#include "GameComponent.h"


GameObject::GameObject()
{
	_transform = new Transform();
}


GameObject::~GameObject()
{
	_children.clear();
	delete &_children;
	_components.clear();
	delete &_components;
	_transform = nullptr;
	delete _transform;
}

void GameObject::addChind(GameObject* child)
{
	_children.push_back(child);
	child->getTransform()->setParent(_transform);
}

void GameObject::addComponent(GameComponent* component)
{
	_components.push_back(component);
	component->setParent(this);
}

void GameObject::input(float delta)
{
	_transform->update();

	for (auto component : _components)
		component->input(delta);

	for (auto child : _children)
		child->input(delta);
}

void GameObject::update(float delta)
{
	for (auto component : _components)
		component->update(delta);

	for (auto child : _children)
		child->update(delta);
}

void GameObject::render(Shader* shader, RenderingEngine* renderingEngine)
{
	for (auto component : _components)
		component->render(shader, renderingEngine);

	for (auto child : _children)
		child->render(shader, renderingEngine);
}

Transform* GameObject::getTransform()
{
	return _transform;
}

void GameObject::addToRenderingEngine(RenderingEngine * renderingEngine)
{
	for (auto component : _components)
		component->addToRenderingEngine(renderingEngine);

	for (auto child : _children)
		child->addToRenderingEngine(renderingEngine);
}

