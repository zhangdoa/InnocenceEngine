#include "AssetSystemSingletonComponent.h"

void AssetSystemSingletonComponent::setup()
{
	m_objectStatus = objectStatus::ALIVE;
}

void AssetSystemSingletonComponent::initialize()
{
}

void AssetSystemSingletonComponent::shutdown()
{
	m_objectStatus = objectStatus::SHUTDOWN;
}

