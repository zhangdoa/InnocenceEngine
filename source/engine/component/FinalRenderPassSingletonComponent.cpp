#include "FinalRenderPassSingletonComponent.h"

void FinalRenderPassSingletonComponent::setup()
{
	m_objectStatus = objectStatus::ALIVE;
}

void FinalRenderPassSingletonComponent::initialize()
{
}

void FinalRenderPassSingletonComponent::shutdown()
{
	m_objectStatus = objectStatus::SHUTDOWN;
}

