#include "LightRenderPassSingletonComponent.h"

void LightRenderPassSingletonComponent::setup()
{
	m_objectStatus = objectStatus::ALIVE;
}

void LightRenderPassSingletonComponent::initialize()
{
}

void LightRenderPassSingletonComponent::shutdown()
{
	m_objectStatus = objectStatus::SHUTDOWN;
}

