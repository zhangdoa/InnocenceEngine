#pragma once
#include "BaseComponent.h"
#include "../../component/GLMeshDataComponent.h"
#include "../../component/GLTextureDataComponent.h"

class GLRenderingSystemSingletonComponent : public BaseComponent
{
public:
	~GLRenderingSystemSingletonComponent() {};
	
	static GLRenderingSystemSingletonComponent& getInstance()
	{
		static GLRenderingSystemSingletonComponent instance;
		return instance;
	}

	std::unordered_map<EntityID, GLMeshDataComponent*> m_meshMap;
	std::unordered_map<EntityID, GLTextureDataComponent*> m_textureMap;

private:
	GLRenderingSystemSingletonComponent() {};
};
