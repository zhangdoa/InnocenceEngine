#include "stdafx.h"
#include "SkyboxComponent.h"


SkyboxComponent::SkyboxComponent()
{
}


SkyboxComponent::~SkyboxComponent()
{
}

void SkyboxComponent::init()
{
	setVisibleGameEntityType(SKYBOX);
	m_meshData.init();
	m_cubemapData.init();
	m_cubemapData.loadCubemap(std::vector<std::string> {"skybox/right.tga",
		"skybox/left.tga", "skybox/top.tga", "skybox/bottom.tga", "skybox/back.tga", "skybox/front.tga" });
}

void SkyboxComponent::update()
{
	m_cubemapData.update();
	m_meshData.update();
}

void SkyboxComponent::shutdown()
{
	m_meshData.shutdown();
	m_cubemapData.shutdown();
}
