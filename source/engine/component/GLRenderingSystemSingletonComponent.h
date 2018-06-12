#pragma once
#include "BaseComponent.h"
#include "AssetSystemSingletonComponent.h"
#include  "VisibleComponent.h"
//#define BlinnPhong
#define CookTorrance

class GLRenderingSystemSingletonComponent : public BaseComponent
{
public:
	~GLRenderingSystemSingletonComponent() {};

	void setup() override;
	void initialize() override;
	void shutdown() override;
	
	static GLRenderingSystemSingletonComponent& getInstance()
	{
		static GLRenderingSystemSingletonComponent instance;
		return instance;
	}

	bool m_shouldUpdateEnvironmentMap = true;
	std::unordered_map<meshID, MeshDataComponent*> m_initializedMeshMap;
	std::unordered_map<textureID, TextureDataComponent*> m_initializedTextureMap;
	vec2 m_renderTargetSize = vec2(640, 360);
	std::vector<VisibleComponent*> m_staticMeshVisibleComponents;
	std::vector<VisibleComponent*> m_emissiveVisibleComponents;
	std::vector<VisibleComponent*> m_selectedVisibleComponents;
	std::vector<VisibleComponent*> m_inFrustumVisibleComponents;

private:
	GLRenderingSystemSingletonComponent() {};
};
