#include "GeometryRenderPassSingletonComponent.h"

void GeometryRenderPassSingletonComponent::setup()
{
	m_objectStatus = objectStatus::ALIVE;
}

void GeometryRenderPassSingletonComponent::initialize()
{
}

void GeometryRenderPassSingletonComponent::shutdown()
{
	m_objectStatus = objectStatus::SHUTDOWN;
}

