#pragma once

#include "interface/IRenderingManager.h"
#include "interface/IAssetManager.h"
#include "interface/ILogManager.h"

#include "GLWindowManager.h"
#include "GLInputManager.h"
#include "GLRenderingManager.h"

#include "entity/ComponentHeaders.h"

extern ILogManager* g_pLogManager;
extern IAssetManager* g_pAssetManager;

class RenderingManager : public IRenderingManager
{
public:
	RenderingManager() {};
	~RenderingManager() {};

	void setup() override;
	void initialize() override;
	void update() override;
	void render() override;
	void shutdown() override;

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

