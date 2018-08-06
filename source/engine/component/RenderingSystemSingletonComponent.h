#pragma once
#include "BaseComponent.h"
#include "AssetSystemSingletonComponent.h"
#include  "VisibleComponent.h"
//#define BlinnPhong
#define CookTorrance

class RenderingSystemSingletonComponent : public BaseComponent
{
public:
	~RenderingSystemSingletonComponent() {};
	
	static RenderingSystemSingletonComponent& getInstance()
	{
		static RenderingSystemSingletonComponent instance;
		return instance;
	}

	bool m_shouldUpdateEnvironmentMap = true;
	std::unordered_map<meshID, MeshDataComponent*> m_initializedMeshMap;
	std::unordered_map<textureID, TextureDataComponent*> m_initializedTextureMap;
	vec2 m_renderTargetSize = vec2(1280, 720);
	std::vector<VisibleComponent*> m_staticMeshVisibleComponents;
	std::vector<VisibleComponent*> m_emissiveVisibleComponents;
	std::vector<VisibleComponent*> m_selectedVisibleComponents;
	std::vector<VisibleComponent*> m_inFrustumVisibleComponents;

private:
	RenderingSystemSingletonComponent() {};
};
