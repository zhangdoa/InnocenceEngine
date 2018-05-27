#include "MeshDataComponent.h"

void MeshDataComponent::setup()
{
	m_objectStatus = objectStatus::ALIVE;
}

void MeshDataComponent::initialize()
{
}

void MeshDataComponent::shutdown()
{
	m_objectStatus = objectStatus::SHUTDOWN;
}
