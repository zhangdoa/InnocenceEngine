#include "GameComponent.h"

GameComponent::GameComponent()
{
}

GameComponent::~GameComponent()
{
	_parent = nullptr;
	delete _parent;
}

void GameComponent::setParent(GameObject* parent)
{
	_parent = parent;
}

Transform* GameComponent::getTransform()
{
	return _parent->getTransform();
}

void GameComponent::input(float delta)
{
}

void GameComponent::update(float delta)
{
}
