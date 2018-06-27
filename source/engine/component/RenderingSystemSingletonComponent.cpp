#include "RenderingSystemSingletonComponent.h"

void RenderingSystemSingletonComponent::setup()
{
	m_objectStatus = objectStatus::ALIVE;
}

void RenderingSystemSingletonComponent::initialize()
{
}

void RenderingSystemSingletonComponent::shutdown()
{
	m_objectStatus = objectStatus::SHUTDOWN;
}

