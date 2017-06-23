#include "GameObject.h"

Transform::Transform()
{
	if (_parent != nullptr) {
		_parentMat = new Mat4f();
		_parentMat->initIdentity();
		_pos = new Vec3f(0.0f, 0.0f, 0.0f);
		_rot = new Quaternion(0.0f, 0.0f, 0.0f, 0.0f);
		_scale = new Vec3f(1.0f, 1.0f, 1.0f);
	}
	
}


Transform::~Transform()
{
}

void Transform::update()
{
	if (&_oldPos != nullptr)
	{
		_oldPos = _pos;
		_oldRot = _rot;
		_oldScale = _scale;
	}
	else
	{
		_oldPos = &_pos->operator+(1.0f);
		_oldRot = &_rot->operator*(0.5f);
		_oldScale = &_scale->operator+(1.0f);
	}
}

void Transform::setParent(Transform * parentTransform)
{
	_parent = parentTransform;
}

void Transform::rotate(Vec3f axis, float angle)
{
	_rot = &Quaternion(axis, angle).operator*(&_rot->normalized());
}

bool Transform::hasChanged()
{
	if (&_pos != &_oldPos || &_rot != &_oldRot || &_scale != &_oldScale)
	{
		return true;
	}

	if (_parent != nullptr && _parent->hasChanged())
	{
		return true;
	}
	return false;
}

Mat4f Transform::getTransformation()
{
	Mat4f translationMatrix;
	translationMatrix.initTranslation(_pos->getX(), _pos->getY(), _pos->getZ());
	Mat4f rotaionMartix = _rot->toRotationMatrix();
	Mat4f scaleMartix;
	scaleMartix.initScale(_scale->getX(), _scale->getY(), _scale->getZ());

	return getParentMatrix()* translationMatrix * rotaionMartix * scaleMartix;
}

Mat4f Transform::getParentMatrix()
{
	if (_parent != nullptr && _parent->hasChanged())
	{
		_parentMat = &_parent->getTransformation();
		
	}
	return *_parentMat;
	
}

Vec3f Transform::getTransformedPos()
{
	return getParentMatrix().transform(*_pos);
}

Quaternion Transform::getTransformedRot()
{
	Quaternion parentRotation = Quaternion(0, 0, 0, 1);
	if (_parent != nullptr)
		parentRotation = _parent->getTransformedRot();

	return parentRotation * _rot;
}

GameComponent::GameComponent()
{
}

GameComponent::~GameComponent()
{
	_parent = nullptr;
	delete _parent;
}

GameObject * GameComponent::getParent()
{
	return _parent;
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
	//fprintf(stdout, "Game Object is updating.\n");
	for (auto component : _components)
		component->update(delta);

	for (auto child : _children)
		child->update(delta);
}

void GameObject::render(Shader * shader)
{
	for (auto component : _components)
		component->render(shader);

	for (auto child : _children)
		child->render(shader);
}


Transform* GameObject::getTransform()
{
	return _transform;
}

RenderingEngine * GameObject::getRenderingEngine()
{
	return _renderingEngine;
}

