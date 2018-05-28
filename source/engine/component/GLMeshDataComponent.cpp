#include "GLMeshDataComponent.h"

void GLMeshDataComponent::setup()
{
	m_objectStatus = objectStatus::ALIVE;
}

void GLMeshDataComponent::initialize()
{
}

void GLMeshDataComponent::shutdown()
{
	m_objectStatus = objectStatus::SHUTDOWN;
}
