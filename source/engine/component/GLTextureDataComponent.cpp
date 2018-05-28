#include "GLTextureDataComponent.h"

void GLTextureDataComponent::setup()
{
	m_objectStatus = objectStatus::ALIVE;
}

void GLTextureDataComponent::initialize()
{
}

void GLTextureDataComponent::shutdown()
{
	m_objectStatus = objectStatus::SHUTDOWN;
}
