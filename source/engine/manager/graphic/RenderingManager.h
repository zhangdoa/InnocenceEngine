#pragma once

#include "manager/BaseManager.h"
#include "manager/LogManager.h"

#include "GLWindowManager.h"
#include "GLInputManager.h"
#include "GLRenderingManager.h"

#include "entity/ComponentHeaders.h"

class RenderingManager : public BaseManager
{
public:
	RenderingManager() {};
	~RenderingManager() {};

	void setup() override;
	void initialize() override;
	void update() override;
	void render();
	void shutdown() override;

	meshID addMesh();
	textureID add2DTexture();
	textureID add2DHDRTexture();
	textureID add3DTexture();
	textureID add3DHDRTexture();
	BaseMesh* getMesh(meshID meshID);
	//@TODO: use BaseTexture instead
	Base2DTexture* get2DTexture(textureID textureID);
	Base2DTexture* get2DHDRTexture(textureID textureID);
	Base3DTexture* get3DTexture(textureID textureID);
	Base3DTexture* get3DHDRTexture(textureID textureID);

private:
	enum class textureAssignType { ADD_DEFAULT, OVERWRITE };

	void changeDrawPolygonMode();
	void changeDrawTextureMode();
	void assignUnitMesh(VisibleComponent& visibleComponent, meshType unitMeshType);
	void assignLoadedTexture(textureAssignType textureAssignType, texturePair& loadedTextureDataPair, VisibleComponent& visibleComponent);
	void assignDefaultTextures(textureAssignType textureAssignType, VisibleComponent & visibleComponent);
	void loadTexture(const std::vector<std::string>& fileName, textureType textureType, VisibleComponent& visibleComponent);
	void loadModel(const std::string& fileName, VisibleComponent& visibleComponent);
	void assignloadedModel(modelMap& loadedGraphicDataMap, VisibleComponent& visibleComponent);

	std::vector<std::unique_ptr<IManager>> m_childManager;
	std::function<void()> f_changeDrawPolygonMode;
	std::function<void()> f_changeDrawTextureMode;

	std::vector<VisibleComponent*> m_VisibleComponents;
	std::vector<LightComponent*> m_LightComponents;
	std::vector<CameraComponent*> m_CameraComponents;
	std::vector<InputComponent*> m_InputComponents;

	meshID m_UnitCubeTemplate;
	meshID m_UnitSphereTemplate;
	meshID m_UnitQuadTemplate;
	textureID m_basicNormalTemplate;
	textureID m_basicAlbedoTemplate;
	textureID m_basicMetallicTemplate;
	textureID m_basicRoughnessTemplate;
	textureID m_basicAOTemplate;

	std::unordered_map<std::string, modelMap> m_loadedModelMap;
	std::unordered_map<std::string, texturePair> m_loadedTextureMap;
};

