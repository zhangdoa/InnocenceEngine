#include "TransformComponent.h"

void TransformComponent::setup()
{
	m_objectStatus = objectStatus::ALIVE;
}

void TransformComponent::initialize()
{
}

void TransformComponent::shutdown()
{
	m_objectStatus = objectStatus::SHUTDOWN;
}
