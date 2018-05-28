#include "GLRenderingSystemSingletonComponent.h"

void GLRenderingSystemSingletonComponent::setup()
{
	m_objectStatus = objectStatus::ALIVE;
}

void GLRenderingSystemSingletonComponent::initialize()
{
}

void GLRenderingSystemSingletonComponent::shutdown()
{
	m_objectStatus = objectStatus::SHUTDOWN;
}

