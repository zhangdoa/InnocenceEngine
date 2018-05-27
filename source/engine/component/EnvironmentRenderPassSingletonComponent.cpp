#include "EnvironmentRenderPassSingletonComponent.h"

void EnvironmentRenderPassSingletonComponent::setup()
{
	m_objectStatus = objectStatus::ALIVE;
}

void EnvironmentRenderPassSingletonComponent::initialize()
{
}

void EnvironmentRenderPassSingletonComponent::shutdown()
{
	m_objectStatus = objectStatus::SHUTDOWN;
}

