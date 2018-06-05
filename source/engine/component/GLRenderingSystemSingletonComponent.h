#pragma once
#include "BaseComponent.h"
#include "AssetSystemSingletonComponent.h"

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

private:
	GLRenderingSystemSingletonComponent() {};
};
