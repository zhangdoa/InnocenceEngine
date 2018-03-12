#pragma once

#include "interface/IRenderingManager.h"
#include "interface/IAssetManager.h"
#include "interface/IGameManager.h"
#include "interface/ILogManager.h"

#include "GLWindowManager.h"
#include "GLInputManager.h"
#include "GLRenderingManager.h"

#include "entity/ComponentHeaders.h"

extern ILogManager* g_pLogManager;
extern IAssetManager* g_pAssetManager;
extern IGameManager* g_pGameManager;

class RenderingManager : public IRenderingManager
{
public:
	RenderingManager() {};
	~RenderingManager() {};

	void setup() override;
	void initialize() override;
	void update() override;
	void shutdown() override;

	void render() override;
	meshID addMesh() override;
	textureID addTexture(textureType textureType) override;
	BaseMesh* getMesh(meshID meshID) override;
	BaseTexture* getTexture(textureType textureType, textureID textureID) override;

	const objectStatus& getStatus() const override;

protected:
	void setStatus(objectStatus objectStatus) override;

private:
	objectStatus m_objectStatus = objectStatus::SHUTDOWN;

	enum class textureAssignType { ADD_DEFAULT, OVERWRITE };

	void changeDrawPolygonMode();
	void changeDrawTextureMode();
	void assignUnitMesh(VisibleComponent& visibleComponent, meshType meshType);
	void assignLoadedTexture(textureAssignType textureAssignType, texturePair& loadedTextureDataPair, VisibleComponent& visibleComponent);
	void assignDefaultTextures(textureAssignType textureAssignType, VisibleComponent & visibleComponent);
	void loadTexture(const std::vector<std::string>& fileName, textureType textureType, VisibleComponent& visibleComponent);
	void loadModel(const std::string& fileName, VisibleComponent& visibleComponent);
	void assignloadedModel(modelMap& loadedGraphicDataMap, VisibleComponent& visibleComponent);

	std::vector<std::unique_ptr<IManager>> m_childManager;
	std::function<void()> f_changeDrawPolygonMode;
	std::function<void()> f_changeDrawTextureMode;

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

