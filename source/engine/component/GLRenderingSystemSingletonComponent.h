#pragma once
#include "../common/InnoType.h"
#include "../component/GLMeshDataComponent.h"
#include "../component/GLTextureDataComponent.h"

class GLRenderingSystemSingletonComponent
{
public:
	~GLRenderingSystemSingletonComponent() {};
	
	static GLRenderingSystemSingletonComponent& getInstance()
	{
		static GLRenderingSystemSingletonComponent instance;
		return instance;
	}

	ObjectStatus m_objectStatus = ObjectStatus::SHUTDOWN;
	EntityID m_parentEntity;

	std::unordered_map<EntityID, GLMeshDataComponent*> m_meshMap;
	std::unordered_map<EntityID, GLTextureDataComponent*> m_textureMap;

	GLTextureDataComponent* m_iconTemplate_OBJ;
	GLTextureDataComponent* m_iconTemplate_PNG;
	GLTextureDataComponent* m_iconTemplate_SHADER;
	GLTextureDataComponent* m_iconTemplate_UNKNOWN;

private:
	GLRenderingSystemSingletonComponent() {};
};
