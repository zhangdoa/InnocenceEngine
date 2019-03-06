#pragma once
#include "../common/InnoType.h"
#include "vulkan/vulkan.h"
#include "../component/VKMeshDataComponent.h"
#include "../component/TextureDataComponent.h"
#include "../component/VKTextureDataComponent.h"

class VKRenderingSystemComponent
{
public:
	~VKRenderingSystemComponent() {};
	
	static VKRenderingSystemComponent& get()
	{
		static VKRenderingSystemComponent instance;
		return instance;
	}

	ObjectStatus m_objectStatus = ObjectStatus::SHUTDOWN;
	EntityID m_parentEntity;

	bool m_vsync_enabled = true;

	VkDevice m_device;

	TextureDataDesc deferredPassTextureDesc = TextureDataDesc();

	std::unordered_map<EntityID, VKMeshDataComponent*> m_meshMap;
	std::unordered_map<EntityID, VKTextureDataComponent*> m_textureMap;
	
	VKMeshDataComponent* m_UnitLineVKMDC;
	VKMeshDataComponent* m_UnitQuadVKMDC;
	VKMeshDataComponent* m_UnitCubeVKMDC;
	VKMeshDataComponent* m_UnitSphereVKMDC;

	VKTextureDataComponent* m_iconTemplate_OBJ;
	VKTextureDataComponent* m_iconTemplate_PNG;
	VKTextureDataComponent* m_iconTemplate_SHADER;
	VKTextureDataComponent* m_iconTemplate_UNKNOWN;

	VKTextureDataComponent* m_iconTemplate_DirectionalLight;
	VKTextureDataComponent* m_iconTemplate_PointLight;
	VKTextureDataComponent* m_iconTemplate_SphereLight;

	VKTextureDataComponent* m_basicNormalVKTDC;
	VKTextureDataComponent* m_basicAlbedoVKTDC;
	VKTextureDataComponent* m_basicMetallicVKTDC;
	VKTextureDataComponent* m_basicRoughnessVKTDC;
	VKTextureDataComponent* m_basicAOVKTDC;

private:
	VKRenderingSystemComponent() {};
};
