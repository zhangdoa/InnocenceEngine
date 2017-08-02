#include "../../main/stdafx.h"
#include "SkyboxComponent.h"


SkyboxComponent::SkyboxComponent()
{
}


SkyboxComponent::~SkyboxComponent()
{
}

void SkyboxComponent::render()
{
	m_meshData.update();
	m_cubemapData.update();
}

void SkyboxComponent::init()
{
	setVisibleGameEntityType(SKYBOX);
	m_meshData.init();
	m_cubemapData.init();
	m_cubemapData.loadCubemap(std::vector<std::string> {"skybox/right.jpg",
		"skybox/left.jpg", "skybox/top.jpg", "skybox/bottom.jpg", "skybox/back.jpg", "skybox/front.jpg" });
}

void SkyboxComponent::update()
{
	getTransform()->update();
}

void SkyboxComponent::shutdown()
{
	m_meshData.shutdown();
	m_cubemapData.shutdown();
}
