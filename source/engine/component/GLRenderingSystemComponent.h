#pragma once
#include "../common/InnoType.h"
#include "../component/GLMeshDataComponent.h"
#include "../component/GLTextureDataComponent.h"
#include "../component/GLRenderPassComponent.h"

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

struct OpaquePassDataPack
{
	size_t indiceSize;
	GPassMeshUBOData meshUBOData;
	GLMeshDataComponent* GLMDC;
	MeshPrimitiveTopology meshPrimitiveTopology;
	MeshShapeType meshShapeType;
	GPassTextureUBOData textureUBOData;
	GLTextureDataComponent* normalGLTDC;
	GLTextureDataComponent* albedoGLTDC;
	GLTextureDataComponent* metallicGLTDC;
	GLTextureDataComponent* roughnessGLTDC;
	GLTextureDataComponent* AOGLTDC;
	VisiblilityType visiblilityType;
};

struct TransparentPassDataPack
{
	size_t indiceSize;
	GPassMeshUBOData meshUBOData;
	GLMeshDataComponent* GLMDC;
	MeshPrimitiveTopology meshPrimitiveTopology;
	MeshShapeType meshShapeType;
	MeshCustomMaterial meshCustomMaterial;
	VisiblilityType visiblilityType;
};

struct BillboardPassDataPack
{
	vec4 globalPos;
	float distanceToCamera;
	WorldEditorIconType iconType;
};

struct DebuggerPassDataPack
{
	mat4 m;
	size_t indiceSize;
	GLMeshDataComponent* GLMDC;
	MeshPrimitiveTopology meshPrimitiveTopology;
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

	GLFrameBufferDesc depthOnlyPassFBDesc = GLFrameBufferDesc();
	TextureDataDesc depthOnlyPassTextureDesc = TextureDataDesc();

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

	GPassCameraUBOData m_GPassCameraUBOData;

	std::queue<OpaquePassDataPack> m_opaquePassDataQueue;

	std::queue<TransparentPassDataPack> m_transparentPassDataQueue;

	std::queue<BillboardPassDataPack> m_billboardPassDataQueue;

	std::queue<DebuggerPassDataPack> m_debuggerPassDataQueue;

	const unsigned int m_maxPointLights = 64;
	std::vector<PointLightData> m_PointLightDatas;

	const unsigned int m_maxSphereLights = 64;
	std::vector<SphereLightData> m_SphereLightDatas;

private:
	GLRenderingSystemComponent() {};
};
