#pragma once
#include "BaseComponent.h"
#include "common/GLHeaders.h"
#include "GLMeshDataComponent.h"
#include "GLTextureDataComponent.h"

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

	std::unordered_map<meshID, GLMeshDataComponent*> m_meshMap;
	std::unordered_map<textureID, GLTextureDataComponent*> m_textureMap;

private:
	GLRenderingSystemSingletonComponent() {};
};
