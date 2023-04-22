#include "../../Engine/Interface/IEngine.h"

using namespace Inno;
extern INNO_ENGINE_API IEngine* g_Engine;

#include "Player.inl"

namespace Inno
{
	class WorldSystem : public ISystem
	{
	public:
		INNO_CLASS_CONCRETE_DEFAULT(WorldSystem);

		bool Setup(ISystemConfig* systemConfig) override;
		bool Initialize() override;
		bool Update();
		bool Terminate() override;
		ObjectStatus GetStatus() {return m_ObjectStatus; }
		
	private:
		bool setupReferenceSpheres();
		bool setupOcclusionCubes();
		bool setupOpaqueSpheres();
		bool setupTransparentCubes();
		bool setupVolumetricCubes();
		bool setupPointLights();

		bool updateMaterial(Model* model, Vec4 albedo, Vec4 MRAT, ShaderModel shaderModel = ShaderModel::Opaque);
		void updateSpheres();

		void runTest(uint32_t testTime, std::function<bool()> testCase);

		Vec4 getMousePositionInWorldSpace();

		ObjectStatus m_ObjectStatus = ObjectStatus::Terminated;

		Player* m_player = nullptr;

		std::vector<Entity*> m_referenceSphereEntites;
		std::vector<TransformComponent*> m_referenceSphereTransformComponents;
		std::vector<VisibleComponent*> m_referenceSphereVisibleComponents;

		std::vector<Entity*> m_opaqueSphereEntites;
		std::vector<TransformComponent*> m_opaqueSphereTransformComponents;
		std::vector<VisibleComponent*> m_opaqueSphereVisibleComponents;

		std::vector<Entity*> m_transparentCubeEntites;
		std::vector<TransformComponent*> m_transparentCubeTransformComponents;
		std::vector<VisibleComponent*> m_transparentCubeVisibleComponents;

		std::vector<Entity*> m_volumetricCubeEntites;
		std::vector<TransformComponent*> m_volumetricCubeTransformComponents;
		std::vector<VisibleComponent*> m_volumetricCubeVisibleComponents;

		std::vector<Entity*> m_occlusionCubeEntites;
		std::vector<TransformComponent*> m_occlusionCubeTransformComponents;
		std::vector<VisibleComponent*> m_occlusionCubeVisibleComponents;

		std::vector<Entity*> m_pointLightEntites;
		std::vector<TransformComponent*> m_pointLightTransformComponents;
		std::vector<LightComponent*> m_pointLightComponents;

		Vec4 m_posOffset;
		std::default_random_engine m_generator;

		std::function<void()> f_sceneLoadingFinishCallback;
		std::function<void()> f_loadTestScene;
		std::function<void()> f_convertModel;

		std::function<void()> f_runRayTracing;
		std::function<void()> f_pauseGame;

		float seed = 0.0f;
		bool allowUpdate = true;
	};

	bool WorldSystem::setupReferenceSpheres()
	{
		uint32_t l_matrixDim = 8;
		float l_breadthInterval = 4.0f;
		auto l_containerSize = l_matrixDim * l_matrixDim;

		m_referenceSphereTransformComponents.clear();
		m_referenceSphereVisibleComponents.clear();
		m_referenceSphereEntites.clear();

		m_referenceSphereTransformComponents.reserve(l_containerSize);
		m_referenceSphereVisibleComponents.reserve(l_containerSize);
		m_referenceSphereEntites.reserve(l_containerSize);

		for (uint32_t i = 0; i < l_containerSize; i++)
		{
			m_referenceSphereTransformComponents.emplace_back();
			m_referenceSphereVisibleComponents.emplace_back();
			auto l_entityName = std::string("MaterialReferenceSphere_" + std::to_string(i) + "/");
			m_referenceSphereEntites.emplace_back(g_Engine->getEntityManager()->Spawn(false, ObjectLifespan::Scene, l_entityName.c_str()));
		}

		auto l_rootTranformComponent = g_Engine->getComponentManager()->Get<TransformComponent>(0);

		for (uint32_t i = 0; i < l_containerSize; i++)
		{
			m_referenceSphereTransformComponents[i] = g_Engine->getComponentManager()->Spawn<TransformComponent>(m_referenceSphereEntites[i], false, ObjectLifespan::Scene);
			m_referenceSphereTransformComponents[i]->m_parentTransformComponent = l_rootTranformComponent;
			m_referenceSphereTransformComponents[i]->m_localTransformVector.m_scale = Vec4(1.0f, 1.0f, 1.0f, 1.0f);
			m_referenceSphereVisibleComponents[i] = g_Engine->getComponentManager()->Spawn<VisibleComponent>(m_referenceSphereEntites[i], false, ObjectLifespan::Scene);
			m_referenceSphereVisibleComponents[i]->m_proceduralMeshShape = ProceduralMeshShape::Sphere;
			m_referenceSphereVisibleComponents[i]->m_meshUsage = MeshUsage::Dynamic;
			m_referenceSphereVisibleComponents[i]->m_meshPrimitiveTopology = MeshPrimitiveTopology::TriangleStrip;
			m_referenceSphereVisibleComponents[i]->m_simulatePhysics = true;
		}

		for (uint32_t i = 0; i < l_matrixDim; i++)
		{
			for (uint32_t j = 0; j < l_matrixDim; j++)
			{
				m_referenceSphereTransformComponents[i * l_matrixDim + j]->m_localTransformVector.m_pos =
					m_posOffset +
					Vec4(
						(-(l_matrixDim - 1.0f) * l_breadthInterval / 2.0f) + (i * l_breadthInterval) + 100.0f,
						2.0f,
						(j * l_breadthInterval) - 2.0f * (l_matrixDim - 1),
						0.0f);
			}
		}

		return true;
	}

	bool WorldSystem::setupOcclusionCubes()
	{
		uint32_t l_matrixDim = 8;
		float l_breadthInterval = 42.0f;
		auto l_containerSize = l_matrixDim * l_matrixDim;

		m_occlusionCubeTransformComponents.clear();
		m_occlusionCubeVisibleComponents.clear();
		m_occlusionCubeEntites.clear();

		m_occlusionCubeTransformComponents.reserve(l_containerSize);
		m_occlusionCubeVisibleComponents.reserve(l_containerSize);
		m_occlusionCubeEntites.reserve(l_containerSize);

		for (uint32_t i = 0; i < l_containerSize; i++)
		{
			m_occlusionCubeTransformComponents.emplace_back();
			m_occlusionCubeVisibleComponents.emplace_back();
			auto l_entityName = std::string("OcclusionCube_" + std::to_string(i) + "/");
			m_occlusionCubeEntites.emplace_back(g_Engine->getEntityManager()->Spawn(false, ObjectLifespan::Scene, l_entityName.c_str()));
		}

		auto l_rootTranformComponent = g_Engine->getComponentManager()->Get<TransformComponent>(0);

		for (uint32_t i = 0; i < l_containerSize; i++)
		{
			m_occlusionCubeTransformComponents[i] = g_Engine->getComponentManager()->Spawn<TransformComponent>(m_occlusionCubeEntites[i], false, ObjectLifespan::Scene);
			m_occlusionCubeTransformComponents[i]->m_parentTransformComponent = l_rootTranformComponent;
			m_occlusionCubeVisibleComponents[i] = g_Engine->getComponentManager()->Spawn<VisibleComponent>(m_occlusionCubeEntites[i], false, ObjectLifespan::Scene);
			m_occlusionCubeVisibleComponents[i]->m_proceduralMeshShape = ProceduralMeshShape::Cube;
			m_occlusionCubeVisibleComponents[i]->m_meshUsage = MeshUsage::Static;
			m_occlusionCubeVisibleComponents[i]->m_meshPrimitiveTopology = MeshPrimitiveTopology::TriangleStrip;
			m_occlusionCubeVisibleComponents[i]->m_simulatePhysics = true;
		}

		std::uniform_real_distribution<float> l_randomRotDelta(0.0f, 180.0f);
		std::uniform_real_distribution<float> l_randomHeightDelta(16.0f, 48.0f);
		std::uniform_real_distribution<float> l_randomWidthDelta(4.0f, 6.0f);
		std::uniform_real_distribution<float> l_randomDepthDelta(6.0f, 8.0f);

		auto l_halfMatrixDim = float(l_matrixDim - 1) / 2.0f;
		auto l_offset = l_halfMatrixDim * l_breadthInterval;

		for (uint32_t i = 0; i < l_matrixDim; i++)
		{
			for (uint32_t j = 0; j < l_matrixDim; j++)
			{
				auto l_currentComponent = m_occlusionCubeTransformComponents[i * l_matrixDim + j];

				auto l_heightOffset = l_halfMatrixDim * 3.0f - std::abs((float)i - l_halfMatrixDim) - std::abs((float)j - l_halfMatrixDim);
				l_heightOffset *= 4.0f;
				l_currentComponent->m_localTransformVector.m_scale =
					Vec4(l_randomWidthDelta(m_generator), l_heightOffset, l_randomDepthDelta(m_generator), 1.0f);

				l_currentComponent->m_localTransformVector.m_pos =
					m_posOffset +
					Vec4(
						(i * l_breadthInterval) - l_offset,
						l_currentComponent->m_localTransformVector.m_scale.y / 2.0f,
						(j * l_breadthInterval) - l_offset,
						0.0f);

				l_currentComponent->m_localTransformVector.m_rot =
					Math::calcRotatedLocalRotator(l_currentComponent->m_localTransformVector.m_rot,
						Vec4(0.0f, 1.0f, 0.0f, 0.0f),
						l_randomRotDelta(m_generator));
			}
		}

		return true;
	}

	bool WorldSystem::setupOpaqueSpheres()
	{
		uint32_t l_matrixDim = 8;
		float l_breadthInterval = 4.0f;
		auto l_containerSize = l_matrixDim * l_matrixDim;

		m_opaqueSphereTransformComponents.clear();
		m_opaqueSphereVisibleComponents.clear();
		m_opaqueSphereEntites.clear();

		m_opaqueSphereTransformComponents.reserve(l_containerSize);
		m_opaqueSphereVisibleComponents.reserve(l_containerSize);
		m_opaqueSphereEntites.reserve(l_containerSize);

		for (uint32_t i = 0; i < l_containerSize; i++)
		{
			m_opaqueSphereTransformComponents.emplace_back();
			m_opaqueSphereVisibleComponents.emplace_back();
			auto l_entityName = std::string("PhysicsTestOpaqueObject_" + std::to_string(i) + "/");
			m_opaqueSphereEntites.emplace_back(g_Engine->getEntityManager()->Spawn(false, ObjectLifespan::Scene, l_entityName.c_str()));
		}

		auto l_rootTranformComponent = g_Engine->getComponentManager()->Get<TransformComponent>(0);

		for (uint32_t i = 0; i < l_containerSize; i++)
		{
			m_opaqueSphereTransformComponents[i] = g_Engine->getComponentManager()->Spawn<TransformComponent>(m_opaqueSphereEntites[i], false, ObjectLifespan::Scene);
			m_opaqueSphereTransformComponents[i]->m_parentTransformComponent = l_rootTranformComponent;
			m_opaqueSphereTransformComponents[i]->m_localTransformVector.m_scale = Vec4(1.0f, 1.0f, 1.0f, 1.0f);
			m_opaqueSphereVisibleComponents[i] = g_Engine->getComponentManager()->Spawn<VisibleComponent>(m_opaqueSphereEntites[i], false, ObjectLifespan::Scene);
			m_opaqueSphereVisibleComponents[i]->m_proceduralMeshShape = ProceduralMeshShape(i % 6 + 5);
			m_opaqueSphereVisibleComponents[i]->m_meshUsage = MeshUsage::Dynamic;
			m_opaqueSphereVisibleComponents[i]->m_meshPrimitiveTopology = MeshPrimitiveTopology::TriangleStrip;
			m_opaqueSphereVisibleComponents[i]->m_simulatePhysics = true;
		}

		std::uniform_real_distribution<float> l_randomPosDelta(0.0f, 1.0f);
		std::uniform_real_distribution<float> l_randomRotDelta(0.0f, 180.0f);

		for (uint32_t i = 0; i < l_matrixDim; i++)
		{
			for (uint32_t j = 0; j < l_matrixDim; j++)
			{
				auto l_currentComponent = m_opaqueSphereTransformComponents[i * l_matrixDim + j];
				l_currentComponent->m_localTransformVector.m_pos =
					m_posOffset +
					Vec4(
						(-(l_matrixDim - 1.0f) * l_breadthInterval / 2.0f) + (i * l_breadthInterval),
						l_randomPosDelta(m_generator) * 50.0f,
						(j * l_breadthInterval) - 2.0f * (l_matrixDim - 1),
						0.0f);

				l_currentComponent->m_localTransformVector.m_rot =
					Math::calcRotatedLocalRotator(l_currentComponent->m_localTransformVector.m_rot,
						Vec4(l_randomPosDelta(m_generator), l_randomPosDelta(m_generator), l_randomPosDelta(m_generator), 0.0f).normalize(),
						l_randomRotDelta(m_generator));
			}
		}

		return true;
	}

	bool WorldSystem::setupTransparentCubes()
	{
		float l_breadthInterval = 4.0f;
		uint32_t l_containerSize = 8;

		m_transparentCubeTransformComponents.clear();
		m_transparentCubeVisibleComponents.clear();
		m_transparentCubeEntites.clear();

		m_transparentCubeTransformComponents.reserve(l_containerSize);
		m_transparentCubeVisibleComponents.reserve(l_containerSize);
		m_transparentCubeEntites.reserve(l_containerSize);

		for (uint32_t i = 0; i < l_containerSize; i++)
		{
			m_transparentCubeTransformComponents.emplace_back();
			m_transparentCubeVisibleComponents.emplace_back();
			auto l_entityName = std::string("PhysicsTestTransparentCube_" + std::to_string(i) + "/");
			m_transparentCubeEntites.emplace_back(g_Engine->getEntityManager()->Spawn(false, ObjectLifespan::Scene, l_entityName.c_str()));
		}

		auto l_rootTranformComponent = g_Engine->getComponentManager()->Get<TransformComponent>(0);

		for (uint32_t i = 0; i < l_containerSize; i++)
		{
			m_transparentCubeTransformComponents[i] = g_Engine->getComponentManager()->Spawn<TransformComponent>(m_transparentCubeEntites[i], false, ObjectLifespan::Scene);
			m_transparentCubeTransformComponents[i]->m_parentTransformComponent = l_rootTranformComponent;
			m_transparentCubeTransformComponents[i]->m_localTransformVector.m_scale = Vec4(1.0f * i, 1.0f * i, 0.5f, 1.0f);
			m_transparentCubeVisibleComponents[i] = g_Engine->getComponentManager()->Spawn<VisibleComponent>(m_transparentCubeEntites[i], false, ObjectLifespan::Scene);
			m_transparentCubeVisibleComponents[i]->m_proceduralMeshShape = ProceduralMeshShape::Cube;
			m_transparentCubeVisibleComponents[i]->m_meshUsage = MeshUsage::Dynamic;
			m_transparentCubeVisibleComponents[i]->m_meshPrimitiveTopology = MeshPrimitiveTopology::TriangleStrip;
			m_transparentCubeVisibleComponents[i]->m_simulatePhysics = true;
		}

		for (uint32_t i = 0; i < l_containerSize; i++)
		{
			m_transparentCubeTransformComponents[i]->m_localTransformVector.m_pos = Vec4(0.0f, 2.0f * i, -(i * l_breadthInterval) - 4.0f, 1.0f);
		}

		return true;
	}

	bool WorldSystem::setupVolumetricCubes()
	{
		float l_breadthInterval = 4.0f;
		uint32_t l_containerSize = 8;

		m_volumetricCubeTransformComponents.clear();
		m_volumetricCubeVisibleComponents.clear();
		m_volumetricCubeEntites.clear();

		m_volumetricCubeTransformComponents.reserve(l_containerSize);
		m_volumetricCubeVisibleComponents.reserve(l_containerSize);
		m_volumetricCubeEntites.reserve(l_containerSize);

		for (uint32_t i = 0; i < l_containerSize; i++)
		{
			m_volumetricCubeTransformComponents.emplace_back();
			m_volumetricCubeVisibleComponents.emplace_back();
			auto l_entityName = std::string("PhysicsTestVolumetricCube_" + std::to_string(i) + "/");
			m_volumetricCubeEntites.emplace_back(g_Engine->getEntityManager()->Spawn(false, ObjectLifespan::Scene, l_entityName.c_str()));
		}

		auto l_rootTranformComponent = g_Engine->getComponentManager()->Get<TransformComponent>(0);

		for (uint32_t i = 0; i < l_containerSize; i++)
		{
			m_volumetricCubeTransformComponents[i] = g_Engine->getComponentManager()->Spawn<TransformComponent>(m_volumetricCubeEntites[i], false, ObjectLifespan::Scene);
			m_volumetricCubeTransformComponents[i]->m_parentTransformComponent = l_rootTranformComponent;
			m_volumetricCubeTransformComponents[i]->m_localTransformVector.m_scale = Vec4(4.0f, 4.0f, 4.0f, 1.0f);
			m_volumetricCubeVisibleComponents[i] = g_Engine->getComponentManager()->Spawn<VisibleComponent>(m_volumetricCubeEntites[i], false, ObjectLifespan::Scene);
			m_volumetricCubeVisibleComponents[i]->m_proceduralMeshShape = ProceduralMeshShape::Cube;
			m_volumetricCubeVisibleComponents[i]->m_meshUsage = MeshUsage::Dynamic;
			m_volumetricCubeVisibleComponents[i]->m_meshPrimitiveTopology = MeshPrimitiveTopology::TriangleStrip;
			m_volumetricCubeVisibleComponents[i]->m_simulatePhysics = false;
		}

		std::uniform_real_distribution<float> l_randomPosDelta(-40.0f, 40.0f);

		for (uint32_t i = 0; i < l_containerSize; i++)
		{
			m_volumetricCubeTransformComponents[i]->m_localTransformVector.m_pos = Vec4(l_randomPosDelta(m_generator), 2.0f, l_randomPosDelta(m_generator), 1.0f);
		}

		return true;
	}

	bool WorldSystem::setupPointLights()
	{
		uint32_t l_matrixDim = 16;
		float l_breadthInterval = 4.0f;

		auto l_containerSize = l_matrixDim * l_matrixDim;

		m_pointLightTransformComponents.clear();
		m_pointLightComponents.clear();
		m_pointLightEntites.clear();

		m_pointLightTransformComponents.reserve(l_containerSize);
		m_pointLightComponents.reserve(l_containerSize);
		m_pointLightEntites.reserve(l_containerSize);

		std::uniform_real_distribution<float> l_randomPosDelta(0.0f, 1.0f);
		std::uniform_real_distribution<float> l_randomLuminousFlux(10.0f, 100.0f);
		std::uniform_real_distribution<float> l_randomColorTemperature(2000.0f, 14000.0f);

		for (uint32_t i = 0; i < l_containerSize; i++)
		{
			m_pointLightTransformComponents.emplace_back();
			m_pointLightComponents.emplace_back();
			auto l_entityName = std::string("TestPointLight_" + std::to_string(i) + "/");
			m_pointLightEntites.emplace_back(g_Engine->getEntityManager()->Spawn(false, ObjectLifespan::Scene, l_entityName.c_str()));
		}

		auto l_rootTranformComponent = g_Engine->getComponentManager()->Get<TransformComponent>(0);

		for (uint32_t i = 0; i < l_containerSize; i++)
		{
			m_pointLightTransformComponents[i] = g_Engine->getComponentManager()->Spawn<TransformComponent>(m_pointLightEntites[i], false, ObjectLifespan::Scene);
			m_pointLightTransformComponents[i]->m_parentTransformComponent = l_rootTranformComponent;
			m_pointLightTransformComponents[i]->m_localTransformVector.m_scale = Vec4(1.0f, 1.0f, 1.0f, 1.0f);
			m_pointLightComponents[i] = g_Engine->getComponentManager()->Spawn<LightComponent>(m_pointLightEntites[i], false, ObjectLifespan::Scene);
			m_pointLightComponents[i]->m_LightType = LightType::Point;
			m_pointLightComponents[i]->m_LuminousFlux = l_randomLuminousFlux(m_generator);
			m_pointLightComponents[i]->m_ColorTemperature = l_randomColorTemperature(m_generator);
		}

		for (uint32_t i = 0; i < l_matrixDim; i++)
		{
			for (uint32_t j = 0; j < l_matrixDim; j++)
			{
				m_pointLightTransformComponents[i * l_matrixDim + j]->m_localTransformVector.m_pos =
					m_player->m_playerCameraTransformComponent->m_localTransformVector.m_pos +
					Vec4(
						(-(l_matrixDim - 1.0f) * l_breadthInterval * l_randomPosDelta(m_generator) / 2.0f) + (i * l_breadthInterval), l_randomPosDelta(m_generator) * 32.0f,
						(j * l_breadthInterval) - 2.0f * (l_matrixDim - 1),
						0.0f);
			}
		}

		return true;
	}

	bool WorldSystem::Setup(ISystemConfig* systemConfig)
	{
		auto l_testQuatToMat = [&]() -> bool {
			std::uniform_real_distribution<float> randomAxis(0.0f, 1.0f);
			auto axisSample = Vec4(randomAxis(m_generator) * 2.0f - 1.0f, randomAxis(m_generator) * 2.0f - 1.0f, randomAxis(m_generator) * 2.0f - 1.0f, 0.0f);
			axisSample = axisSample.normalize();

			std::uniform_real_distribution<float> randomAngle(0.0f, 360.0f);
			auto angleSample = randomAngle(m_generator);

			Vec4 originalRot = Math::getQuatRotator(axisSample, angleSample);
			Mat4 rotMat = Math::toRotationMatrix(originalRot);
			auto resultRot = Math::toQuatRotator(rotMat);

			auto testResult = true;
			testResult &= (std::abs(std::abs(originalRot.w) - std::abs(resultRot.w)) < epsilon<float, 4>);
			testResult &= (std::abs(std::abs(originalRot.x) - std::abs(resultRot.x)) < epsilon<float, 4>);
			testResult &= (std::abs(std::abs(originalRot.y) - std::abs(resultRot.y)) < epsilon<float, 4>);
			testResult &= (std::abs(std::abs(originalRot.z) - std::abs(resultRot.z)) < epsilon<float, 4>);

			return testResult;
		};

		runTest(512, l_testQuatToMat);

		f_runRayTracing = [&]() { g_Engine->getRenderingFrontend()->runRayTrace(); };
		f_pauseGame = [&]() { allowUpdate = !allowUpdate; };

		g_Engine->getEventSystem()->addButtonStateCallback(ButtonState{ INNO_KEY_N, true }, ButtonEvent{ EventLifeTime::OneShot, &f_runRayTracing });
		g_Engine->getEventSystem()->addButtonStateCallback(ButtonState{ INNO_KEY_F, true }, ButtonEvent{ EventLifeTime::OneShot, &f_pauseGame });

		f_sceneLoadingFinishCallback = [&]() {
			if (!m_player)
				m_player = new Player();
			m_player->Setup();

			m_posOffset = m_player->m_playerCameraTransformComponent->m_localTransformVector.m_pos;
			m_posOffset.z -= 75.0f;

			m_posOffset = Vec4(0.0f, 0.0f, 0.0f, 1.0f);

			setupReferenceSpheres();
			setupOcclusionCubes();
			setupOpaqueSpheres();
			setupTransparentCubes();
			setupVolumetricCubes();
			setupPointLights();

			m_ObjectStatus = ObjectStatus::Activated;
		};

		g_Engine->getSceneSystem()->addSceneLoadingFinishCallback(&f_sceneLoadingFinishCallback, 0);

		return true;
	}

	bool WorldSystem::Initialize()
	{       
		bool l_result = true;
        
        g_Engine->getSceneSystem()->loadScene("..//Res//Scenes//default.InnoScene");

        //g_Engine->getSceneSystem()->loadScene("..//Res//Scenes//GITestBox.InnoScene");
        //g_Engine->getSceneSystem()->loadScene("..//Res//Scenes//GITestSibenik.InnoScene");
        //g_Engine->getSceneSystem()->loadScene("..//Res//Scenes//GITestSponza.InnoScene");
        //g_Engine->getSceneSystem()->loadScene("..//Res//Scenes//GITestFireplaceRoom.InnoScene");

		f_loadTestScene = []() {
			g_Engine->getSceneSystem()->loadScene("..//Res//Scenes//GITestBox.InnoScene");
			//g_Engine->getSceneSystem()->loadScene("..//Res//Scenes//GITestSibenik.InnoScene");
			//g_Engine->getSceneSystem()->loadScene("..//Res//Scenes//GITestSponza.InnoScene");
			//g_Engine->getSceneSystem()->loadScene("..//Res//Scenes//GITestFireplaceRoom.InnoScene");
		};

		f_convertModel = []() {
			g_Engine->getAssetSystem()->convertModel("..//Res//Models//Wolf//Wolf.fbx", "..//Res//ConvertedAssets//");
		};

		g_Engine->getEventSystem()->addButtonStateCallback(ButtonState{ INNO_KEY_R, true }, ButtonEvent{ EventLifeTime::OneShot, &f_loadTestScene });
		g_Engine->getEventSystem()->addButtonStateCallback(ButtonState{ INNO_KEY_Y, true }, ButtonEvent{ EventLifeTime::OneShot, &f_convertModel });

		return true;
	}

	bool WorldSystem::updateMaterial(Model* model, Vec4 albedo, Vec4 MRAT, ShaderModel shaderModel)
	{
		if (model)
		{
			for (uint64_t j = 0; j < model->meshMaterialPairs.m_count; j++)
			{
				auto l_pair = g_Engine->getAssetSystem()->getMeshMaterialPair(model->meshMaterialPairs.m_startOffset + j);
				l_pair->material->m_materialAttributes.AlbedoR = albedo.x;
				l_pair->material->m_materialAttributes.AlbedoG = albedo.y;
				l_pair->material->m_materialAttributes.AlbedoB = albedo.z;
				l_pair->material->m_materialAttributes.Metallic = MRAT.x;
				l_pair->material->m_materialAttributes.Roughness = MRAT.y;
				l_pair->material->m_materialAttributes.AO = MRAT.z;
				l_pair->material->m_materialAttributes.Alpha = albedo.w;
				l_pair->material->m_materialAttributes.Thickness = MRAT.w;
				l_pair->material->m_ShaderModel = shaderModel;
			}
		}
		return true;
	}


	bool WorldSystem::Update()
	{
		if (m_ObjectStatus == ObjectStatus::Activated)
		{
			if (allowUpdate)
			{
				auto l_tickTime = g_Engine->getTickTime();
				seed += (l_tickTime / 1000.0f);

				auto l_seed = (1.0f - l_tickTime / 100.0f);
				l_seed = l_seed > 0.0f ? l_seed : 0.01f;
				l_seed = l_seed > 0.85f ? 0.85f : l_seed;

				if (m_player)
					m_player->Update(l_seed);

				updateSpheres();
			}
		}

		return true;
	}

    bool WorldSystem::Terminate()
    {
		if (m_player)
		{
			m_player->Terminate();
			delete m_player;
			return true;
		}
        return false;
    }

	void WorldSystem::runTest(uint32_t testTime, std::function<bool()> testCase)
	{
		g_Engine->getLogSystem()->Log(LogLevel::Verbose, "Start test...");
		for (uint32_t i = 0; i < testTime; i++)
		{
			auto l_result = testCase();
			if (!l_result)
			{
				g_Engine->getLogSystem()->Log(LogLevel::Warning, "Test failure.");
			}
		}
		g_Engine->getLogSystem()->Log(LogLevel::Verbose, "Finished test for ", testTime, " times.");
	}

	Vec4 WorldSystem::getMousePositionInWorldSpace()
	{
		auto l_screenResolution = g_Engine->getRenderingFrontend()->getScreenResolution();
		auto l_mousePositionSS = g_Engine->getEventSystem()->getMousePosition();

		auto l_x = 2.0f * l_mousePositionSS.x / l_screenResolution.x - 1.0f;
		auto l_y = 1.0f - 2.0f * l_mousePositionSS.y / l_screenResolution.y;
		auto l_z = -1.0f;
		auto l_w = 1.0f;
		Vec4 l_ndcSpace = Vec4(l_x, l_y, l_z, l_w);

		auto l_activeCamera = static_cast<ICameraSystem*>(g_Engine->getComponentManager()->GetComponentSystem<CameraComponent>())->GetActiveCamera();
		if (l_activeCamera == nullptr)
		{
			return Vec4();
		}
		auto l_cameraTransformComponent = g_Engine->getComponentManager()->Find<TransformComponent>(l_activeCamera->m_Owner);
		if (l_cameraTransformComponent == nullptr)
		{
			return Vec4();
		}
		auto pCamera = l_activeCamera->m_projectionMatrix;
		auto rCamera =
			Math::getInvertRotationMatrix(
				l_cameraTransformComponent->m_globalTransformVector.m_rot);
		auto tCamera =
			Math::getInvertTranslationMatrix(
				l_cameraTransformComponent->m_globalTransformVector.m_pos);
		//Column-Major memory layout
#ifdef USE_COLUMN_MAJOR_MEMORY_LAYOUT
		l_ndcSpace = Math::mul(l_ndcSpace, pCamera.inverse());
		l_ndcSpace.z = -1.0f;
		l_ndcSpace.w = 0.0f;
		l_ndcSpace = Math::mul(l_ndcSpace, rCamera.inverse());
		l_ndcSpace = Math::mul(l_ndcSpace, tCamera.inverse());
#endif
		//Row-Major memory layout
#ifdef USE_ROW_MAJOR_MEMORY_LAYOUT

		l_ndcSpace = Math::mul(pCamera.inverse(), l_ndcSpace);
		l_ndcSpace.z = -1.0f;
		l_ndcSpace.w = 0.0f;
		l_ndcSpace = Math::mul(tCamera.inverse(), l_ndcSpace);
		l_ndcSpace = Math::mul(rCamera.inverse(), l_ndcSpace);
#endif
		l_ndcSpace = l_ndcSpace.normalize();
		return l_ndcSpace;
	}

	void WorldSystem::updateSpheres()
	{
		for (uint32_t i = 0; i < m_opaqueSphereVisibleComponents.size(); i += 4)
		{
			auto l_albedoFactor1 = (sin(seed / 2.0f + i) + 1.0f) / 2.0f;
			auto l_albedoFactor2 = (sin(seed / 3.0f + i) + 1.0f) / 2.0f;
			auto l_albedoFactor3 = (sin(seed / 5.0f + i) + 1.0f) / 2.0f;

			auto l_albedo1 = Vec4(l_albedoFactor1, l_albedoFactor2, l_albedoFactor3, 1.0f);
			auto l_albedo2 = Vec4(l_albedoFactor3, l_albedoFactor2, l_albedoFactor1, 1.0f);
			auto l_albedo3 = Vec4(l_albedoFactor2, l_albedoFactor3, l_albedoFactor1, 1.0f);
			auto l_albedo4 = Vec4(l_albedoFactor2, l_albedoFactor1, l_albedoFactor3, 1.0f);

			auto l_MRATFactor1 = ((sin(seed / 4.0f + i) + 1.0f) / 2.001f);
			auto l_MRATFactor2 = ((sin(seed / 5.0f + i) + 1.0f) / 2.001f);
			auto l_MRATFactor3 = ((sin(seed / 6.0f + i) + 1.0f) / 2.001f);

			updateMaterial(m_opaqueSphereVisibleComponents[i]->m_model, l_albedo1, Vec4(l_MRATFactor1, l_MRATFactor2, 0.0f, 0.0f));
			updateMaterial(m_opaqueSphereVisibleComponents[i + 1]->m_model, l_albedo2, Vec4(l_MRATFactor2, l_MRATFactor1, 0.0f, 0.0f));
			updateMaterial(m_opaqueSphereVisibleComponents[i + 2]->m_model, l_albedo3, Vec4(l_MRATFactor3, l_MRATFactor2, 0.0f, 0.0f));
			updateMaterial(m_opaqueSphereVisibleComponents[i + 3]->m_model, l_albedo4, Vec4(l_MRATFactor3, l_MRATFactor1, 0.0f, 0.0f));
		}

		for (uint32_t i = 0; i < m_transparentCubeVisibleComponents.size(); i++)
		{
			auto l_albedo = Math::HSVtoRGB(Vec4((sin(seed / 6.0f + i) * 0.5f + 0.5f) * 360.0f, 1.0f, 1.0f, 0.5f));
			l_albedo.w = sin(seed / 6.0f + i) * 0.5f + 0.5f;
			auto l_MRAT = Vec4(0.0f, sin(seed / 4.0f + i) * 0.5f + 0.5f, 1.0f, clamp((float)sin(seed / 5.0f + i) * 0.5f + 0.5f, epsilon<float, 4>, 1.0f));
			updateMaterial(m_transparentCubeVisibleComponents[i]->m_model, l_albedo, l_MRAT, ShaderModel::Transparent);
		}

		for (uint32_t i = 0; i < m_volumetricCubeVisibleComponents.size(); i++)
		{
			auto l_albedo = Math::HSVtoRGB(Vec4((sin(seed / 6.0f + i) * 0.5f + 0.5f) * 360.0f, 1.0f, 1.0f, 0.5f));
			l_albedo.w = clamp((float)sin(seed / 7.0f + i) * 0.5f + 0.5f, epsilon<float, 4>, 1.0f);
			auto l_MRAT = Vec4(clamp((float)sin(seed / 5.0f + i) * 0.5f + 0.5f, epsilon<float, 4>, 1.0f), 1.0f, 1.0f, 1.0f);
			updateMaterial(m_volumetricCubeVisibleComponents[i]->m_model, l_albedo, l_MRAT, ShaderModel::Volumetric);
		}

		uint32_t l_matrixDim = 8;
		for (uint32_t i = 0; i < l_matrixDim; i++)
		{
			for (uint32_t j = 0; j < l_matrixDim; j++)
			{
				auto l_MRAT = Vec4((float)i / (float)(l_matrixDim - 1), (float)j / (float)(l_matrixDim - 1), 0.0f, 1.0f);
				updateMaterial(m_referenceSphereVisibleComponents[i * l_matrixDim + j]->m_model, Vec4(1.0f, 1.0f, 1.0f, 1.0f), l_MRAT);
			}
		}
	}
}