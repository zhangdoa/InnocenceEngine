#pragma once
#include "../common/InnoType.h"
#include "../component/GLMeshDataComponent.h"
#include "../component/GLTextureDataComponent.h"
#include "../component/GLFrameBufferComponent.h"

struct GPassCameraUBOData
{
	mat4 m_CamProjOriginal;
	mat4 m_CamProjJittered;
	mat4 m_CamRot;
	mat4 m_CamTrans;
	mat4 m_CamRot_prev;
	mat4 m_CamTrans_prev;
};

struct GPassMeshUBOData
{
	mat4 m;
	mat4 m_prev;
};

// glsl's bool is 4 bytes, so use int here
struct GPassTextureUBOData
{
	vec4 albedo;
	vec4 MRA;
	int useNormalTexture = true;
	int useAlbedoTexture = true;
	int useMetallicTexture = true;
	int useRoughnessTexture = true;
	int useAOTexture = true;
};

struct GPassOpaqueRenderDataPack
{
	size_t indiceSize;
	GPassMeshUBOData m_GPassMeshUBOData;
	GLMeshDataComponent* GLMDC;
	MeshPrimitiveTopology m_meshDrawMethod;
	GPassTextureUBOData m_GPassTextureUBOData;
	GLTextureDataComponent* m_basicNormalGLTDC;
	GLTextureDataComponent* m_basicAlbedoGLTDC;
	GLTextureDataComponent* m_basicMetallicGLTDC;
	GLTextureDataComponent* m_basicRoughnessGLTDC;
	GLTextureDataComponent* m_basicAOGLTDC;
	VisiblilityType visiblilityType;
};

struct GPassTransparentRenderDataPack
{
	size_t indiceSize;
	GPassMeshUBOData m_GPassMeshUBOData;
	GLMeshDataComponent* GLMDC;
	MeshPrimitiveTopology m_meshDrawMethod;
	MeshCustomMaterial meshCustomMaterial;
	VisiblilityType visiblilityType;
};

struct BillboardPassRenderDataPack
{
	vec4 globalPos;
	float distanceToCamera;
	WorldEditorIconType iconType;
};

struct PointLightData
{
	vec4 pos;
	vec4 luminance;
	float attenuationRadius;
};

struct SphereLightData
{
	vec4 pos;
	vec4 luminance;
	float sphereRadius;
};

class GLRenderingSystemComponent
{
public:
	~GLRenderingSystemComponent() {};
	
	static GLRenderingSystemComponent& get()
	{
		static GLRenderingSystemComponent instance;
		return instance;
	}

	ObjectStatus m_objectStatus = ObjectStatus::SHUTDOWN;
	EntityID m_parentEntity;

	GLTextureDataComponent* m_iconTemplate_OBJ;
	GLTextureDataComponent* m_iconTemplate_PNG;
	GLTextureDataComponent* m_iconTemplate_SHADER;
	GLTextureDataComponent* m_iconTemplate_UNKNOWN;

	GLTextureDataComponent* m_iconTemplate_DirectionalLight;
	GLTextureDataComponent* m_iconTemplate_PointLight;
	GLTextureDataComponent* m_iconTemplate_SphereLight;

	GLFrameBufferDesc deferredPassFBDesc = GLFrameBufferDesc();
	TextureDataDesc deferredPassTextureDesc = TextureDataDesc();

	GLMeshDataComponent* m_UnitLineGLMDC;
	GLMeshDataComponent* m_UnitQuadGLMDC;
	GLMeshDataComponent* m_UnitCubeGLMDC;
	GLMeshDataComponent* m_UnitSphereGLMDC;
	GLMeshDataComponent* m_terrainGLMDC;

	GLTextureDataComponent* m_basicNormalGLTDC;
	GLTextureDataComponent* m_basicAlbedoGLTDC;
	GLTextureDataComponent* m_basicMetallicGLTDC;
	GLTextureDataComponent* m_basicRoughnessGLTDC;
	GLTextureDataComponent* m_basicAOGLTDC;

	mat4 m_CamProjOriginal;
	mat4 m_CamProjJittered;
	mat4 m_CamRot;
	mat4 m_CamTrans;
	mat4 m_CamRot_prev;
	mat4 m_CamTrans_prev;
	vec4 m_CamGlobalPos;

	vec4 m_sunDir;
	vec4 m_sunColor;
	mat4 m_sunRot;

	std::vector<mat4> m_CSMProjs;
	std::vector<mat4> m_CSMViews;
	std::vector<vec4> m_CSMSplitCorners;

	GPassCameraUBOData m_GPassCameraUBOData;

	std::queue<GPassOpaqueRenderDataPack> m_GPassOpaqueRenderDataQueue;
	std::queue<GPassOpaqueRenderDataPack> m_GPassOpaqueRenderDataQueue_copy;

	std::queue<GPassTransparentRenderDataPack> m_GPassTransparentRenderDataQueue;

	std::queue<BillboardPassRenderDataPack> m_BillboardPassRenderDataQueue;

	std::vector<PointLightData> m_PointLightDatas;

	std::vector<SphereLightData> m_SphereLightDatas;

private:
	GLRenderingSystemComponent() {};
};
