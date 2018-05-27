#include "CameraComponent.h"

void CameraComponent::setup()
{
	m_objectStatus = objectStatus::ALIVE;
}

void CameraComponent::initialize()
{
}

void CameraComponent::shutdown()
{
	m_objectStatus = objectStatus::SHUTDOWN;
}

