#include "TextureDataComponent.h"

void TextureDataComponent::setup()
{
	m_objectStatus = objectStatus::ALIVE;
}

void TextureDataComponent::initialize()
{
}

void TextureDataComponent::shutdown()
{
	m_objectStatus = objectStatus::SHUTDOWN;
}
