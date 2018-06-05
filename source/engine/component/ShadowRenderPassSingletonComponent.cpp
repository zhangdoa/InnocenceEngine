#include "ShadowRenderPassSingletonComponent.h"

void ShadowRenderPassSingletonComponent::setup()
{
	m_objectStatus = objectStatus::ALIVE;
}

void ShadowRenderPassSingletonComponent::initialize()
{
}

void ShadowRenderPassSingletonComponent::shutdown()
{
	m_objectStatus = objectStatus::SHUTDOWN;
}

