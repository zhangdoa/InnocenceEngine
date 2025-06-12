#include "../../Engine/Services/EntityManager.h"
#include "../../Engine/Services/ComponentManager.h"
#include "../../Engine/Services/RenderingConfigurationService.h"
#include "../../Engine/Services/RenderingContextService.h"
#include "../../Engine/Services/SceneService.h"
#include "../../Engine/Services/AssetService.h"
#include "../../Engine/RayTracer/RayTracer.h"

#include "../../Engine/Engine.h"

using namespace Inno;

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
		ObjectStatus GetStatus() { return m_ObjectStatus; }

	private:
		bool setupReferenceSpheres();
		bool setupOcclusionCubes();
		bool setupOpaqueSpheres();
		bool setupTransparentCubes();
		bool setupVolumetricCubes();
		bool setupPointLights();

		//bool updateMaterial(Model* model, Vec4 albedo, Vec4 MRAT, ShaderModel shaderModel = ShaderModel::Opaque);
		void updateSpheres();

		void runTest(uint32_t testTime, std::function<bool()> testCase);

		Vec4 getMousePositionInWorldSpace();

		ObjectStatus m_ObjectStatus = ObjectStatus::Terminated;

		Player* m_player = nullptr;

		std::vector<Entity*> m_referenceSphereEntities;
		std::vector<ModelComponent*> m_referenceSphereModelComponents;

		std::vector<Entity*> m_opaqueSphereEntities;
		std::vector<ModelComponent*> m_opaqueSphereModelComponents;

		std::vector<Entity*> m_transparentCubeEntities;
		std::vector<ModelComponent*> m_transparentCubeModelComponents;

		std::vector<Entity*> m_volumetricCubeEntities;
		std::vector<ModelComponent*> m_volumetricCubeModelComponents;

		std::vector<Entity*> m_occlusionCubeEntities;
		std::vector<ModelComponent*> m_occlusionCubeModelComponents;

		std::vector<Entity*> m_pointLightEntities;
		std::vector<LightComponent*> m_pointLightComponents;

		Vec4 m_posOffset;
		std::default_random_engine m_generator;

		std::function<void()> f_sceneLoadingFinishedCallback;
		std::function<void()> f_loadTestScene;
		std::function<void()> f_convertModel;

		std::function<void()> f_runRayTracing;
		std::function<void()> f_pauseGame;

		float m_seed = 0.0f;
		bool allowUpdate = true;
		uint32_t m_matrixDim = 8;
	};

	bool WorldSystem::setupReferenceSpheres()
	{
		float l_breadthInterval = 4.0f;
		auto l_containerSize = m_matrixDim * m_matrixDim;

		m_referenceSphereModelComponents.clear();
		m_referenceSphereEntities.clear();

		m_referenceSphereModelComponents.reserve(l_containerSize);
		m_referenceSphereEntities.reserve(l_containerSize);

		for (uint32_t i = 0; i < l_containerSize; i++)
		{
			m_referenceSphereModelComponents.emplace_back();
			auto l_entityName = std::string("MaterialReferenceSphere_" + std::to_string(i) + "/");
			m_referenceSphereEntities.emplace_back(g_Engine->Get<EntityManager>()->Spawn(false, ObjectLifespan::Scene, l_entityName.c_str()));
		}

		for (uint32_t i = 0; i < l_containerSize; i++)
		{
			m_referenceSphereModelComponents[i] = g_Engine->Get<ComponentManager>()->Spawn<ModelComponent>(m_referenceSphereEntities[i], false, ObjectLifespan::Scene);
			m_referenceSphereModelComponents[i]->m_Transform.m_scale = Vec4(1.0f, 1.0f, 1.0f, 1.0f);
		}

		for (uint32_t i = 0; i < m_matrixDim; i++)
		{
			for (uint32_t j = 0; j < m_matrixDim; j++)
			{
				m_referenceSphereModelComponents[i * m_matrixDim + j]->m_Transform.m_pos =
					m_posOffset +
					Vec4(
						(-(m_matrixDim - 1.0f) * l_breadthInterval / 2.0f) + (i * l_breadthInterval) + 100.0f,
						2.0f,
						(j * l_breadthInterval) - 2.0f * (m_matrixDim - 1),
						0.0f);
			}
		}

		return true;
	}

	bool WorldSystem::setupOcclusionCubes()
	{
		uint32_t matrixDim = 4;
		float l_breadthInterval = 42.0f;
		auto l_containerSize = matrixDim * matrixDim;

		m_occlusionCubeModelComponents.clear();
		m_occlusionCubeEntities.clear();

		m_occlusionCubeModelComponents.reserve(l_containerSize);
		m_occlusionCubeEntities.reserve(l_containerSize);

		for (uint32_t i = 0; i < l_containerSize; i++)
		{
			m_occlusionCubeModelComponents.emplace_back();
			auto l_entityName = std::string("OcclusionCube_" + std::to_string(i) + "/");
			m_occlusionCubeEntities.emplace_back(g_Engine->Get<EntityManager>()->Spawn(false, ObjectLifespan::Scene, l_entityName.c_str()));
		}

		for (uint32_t i = 0; i < l_containerSize; i++)
		{
			m_occlusionCubeModelComponents[i] = g_Engine->Get<ComponentManager>()->Spawn<ModelComponent>(m_occlusionCubeEntities[i], false, ObjectLifespan::Scene);
		}

		std::uniform_real_distribution<float> l_randomRotDelta(0.0f, 180.0f);
		std::uniform_real_distribution<float> l_randomHeightDelta(16.0f, 48.0f);
		std::uniform_real_distribution<float> l_randomWidthDelta(4.0f, 6.0f);
		std::uniform_real_distribution<float> l_randomDepthDelta(6.0f, 8.0f);

		auto l_halfMatrixDim = float(matrixDim - 1) / 2.0f;
		auto l_offset = l_halfMatrixDim * l_breadthInterval;

		for (uint32_t i = 0; i < matrixDim; i++)
		{
			for (uint32_t j = 0; j < matrixDim; j++)
			{
				auto l_currentComponent = m_occlusionCubeModelComponents[i * matrixDim + j];

				auto l_heightOffset = l_halfMatrixDim * 3.0f - std::abs((float)i - l_halfMatrixDim) - std::abs((float)j - l_halfMatrixDim);
				l_heightOffset *= 4.0f;
				l_currentComponent->m_Transform.m_scale =
					Vec4(l_randomWidthDelta(m_generator), l_heightOffset, l_randomDepthDelta(m_generator), 1.0f);

				l_currentComponent->m_Transform.m_pos =
					m_posOffset +
					Vec4(
						(i * l_breadthInterval) - l_offset,
						l_currentComponent->m_Transform.m_scale.y / 2.0f,
						(j * l_breadthInterval) - l_offset,
						0.0f);

				l_currentComponent->m_Transform.m_rot =
					Math::calcRotatedLocalRotator(l_currentComponent->m_Transform.m_rot,
						Vec4(0.0f, 1.0f, 0.0f, 0.0f),
						l_randomRotDelta(m_generator));
			}
		}

		return true;
	}

	bool WorldSystem::setupOpaqueSpheres()
	{
		float l_breadthInterval = 4.0f;
		auto l_containerSize = m_matrixDim * m_matrixDim;

		m_opaqueSphereModelComponents.clear();
		m_opaqueSphereEntities.clear();

		m_opaqueSphereModelComponents.reserve(l_containerSize);
		m_opaqueSphereEntities.reserve(l_containerSize);

		for (uint32_t i = 0; i < l_containerSize; i++)
		{
			m_opaqueSphereModelComponents.emplace_back();
			auto l_entityName = std::string("PhysicsTestOpaqueObject_" + std::to_string(i) + "/");
			m_opaqueSphereEntities.emplace_back(g_Engine->Get<EntityManager>()->Spawn(false, ObjectLifespan::Scene, l_entityName.c_str()));
		}

		for (uint32_t i = 0; i < l_containerSize; i++)
		{
			m_opaqueSphereModelComponents[i] = g_Engine->Get<ComponentManager>()->Spawn<ModelComponent>(m_opaqueSphereEntities[i], false, ObjectLifespan::Scene);
			m_opaqueSphereModelComponents[i]->m_Transform.m_scale = Vec4(1.0f, 1.0f, 1.0f, 1.0f);
		}

		std::uniform_real_distribution<float> l_randomPosDelta(0.0f, 1.0f);
		std::uniform_real_distribution<float> l_randomRotDelta(0.0f, 180.0f);

		for (uint32_t i = 0; i < m_matrixDim; i++)
		{
			for (uint32_t j = 0; j < m_matrixDim; j++)
			{
				auto l_currentComponent = m_opaqueSphereModelComponents[i * m_matrixDim + j];
				l_currentComponent->m_Transform.m_pos =
					m_posOffset +
					Vec4(
						(-(m_matrixDim - 1.0f) * l_breadthInterval / 2.0f) + (i * l_breadthInterval),
						l_randomPosDelta(m_generator) * 50.0f,
						(j * l_breadthInterval) - 2.0f * (m_matrixDim - 1),
						0.0f);

				l_currentComponent->m_Transform.m_rot =
					Math::calcRotatedLocalRotator(l_currentComponent->m_Transform.m_rot,
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

		m_transparentCubeModelComponents.clear();
		m_transparentCubeEntities.clear();

		m_transparentCubeModelComponents.reserve(l_containerSize);
		m_transparentCubeEntities.reserve(l_containerSize);

		for (uint32_t i = 0; i < l_containerSize; i++)
		{
			m_transparentCubeModelComponents.emplace_back();
			auto l_entityName = std::string("PhysicsTestTransparentCube_" + std::to_string(i) + "/");
			m_transparentCubeEntities.emplace_back(g_Engine->Get<EntityManager>()->Spawn(false, ObjectLifespan::Scene, l_entityName.c_str()));
		}

		for (uint32_t i = 0; i < l_containerSize; i++)
		{
			m_transparentCubeModelComponents[i] = g_Engine->Get<ComponentManager>()->Spawn<ModelComponent>(m_transparentCubeEntities[i], false, ObjectLifespan::Scene);
			m_transparentCubeModelComponents[i]->m_Transform.m_scale = Vec4(1.0f * i, 1.0f * i, 0.5f, 1.0f);
		}

		for (uint32_t i = 0; i < l_containerSize; i++)
		{
			m_transparentCubeModelComponents[i]->m_Transform.m_pos = Vec4(0.0f, 2.0f * i, -(i * l_breadthInterval) - 4.0f, 1.0f);
		}

		return true;
	}

	bool WorldSystem::setupVolumetricCubes()
	{
		uint32_t l_containerSize = 8;

		m_volumetricCubeModelComponents.clear();
		m_volumetricCubeEntities.clear();

		m_volumetricCubeModelComponents.reserve(l_containerSize);
		m_volumetricCubeEntities.reserve(l_containerSize);

		for (uint32_t i = 0; i < l_containerSize; i++)
		{
			m_volumetricCubeModelComponents.emplace_back();
			auto l_entityName = std::string("PhysicsTestVolumetricCube_" + std::to_string(i) + "/");
			m_volumetricCubeEntities.emplace_back(g_Engine->Get<EntityManager>()->Spawn(false, ObjectLifespan::Scene, l_entityName.c_str()));
		}

		for (uint32_t i = 0; i < l_containerSize; i++)
		{
			m_volumetricCubeModelComponents[i] = g_Engine->Get<ComponentManager>()->Spawn<ModelComponent>(m_volumetricCubeEntities[i], false, ObjectLifespan::Scene);
			m_volumetricCubeModelComponents[i]->m_Transform.m_scale = Vec4(4.0f, 4.0f, 4.0f, 1.0f);
		}

		std::uniform_real_distribution<float> l_randomPosDelta(-40.0f, 40.0f);

		for (uint32_t i = 0; i < l_containerSize; i++)
		{
			m_volumetricCubeModelComponents[i]->m_Transform.m_pos = Vec4(l_randomPosDelta(m_generator), 2.0f, l_randomPosDelta(m_generator), 1.0f);
		}

		return true;
	}

	bool WorldSystem::setupPointLights()
	{
		uint32_t l_matrixDim = 16;
		float l_breadthInterval = 4.0f;

		auto l_containerSize = l_matrixDim * l_matrixDim;

		m_pointLightComponents.clear();
		m_pointLightEntities.clear();

		m_pointLightComponents.reserve(l_containerSize);
		m_pointLightEntities.reserve(l_containerSize);

		std::uniform_real_distribution<float> l_randomPosDelta(0.0f, 1.0f);
		std::uniform_real_distribution<float> l_randomLuminousFlux(10.0f, 100.0f);
		std::uniform_real_distribution<float> l_randomColorTemperature(2000.0f, 14000.0f);

		for (uint32_t i = 0; i < l_containerSize; i++)
		{
			m_pointLightComponents.emplace_back();
			auto l_entityName = std::string("TestPointLight_" + std::to_string(i) + "/");
			m_pointLightEntities.emplace_back(g_Engine->Get<EntityManager>()->Spawn(false, ObjectLifespan::Scene, l_entityName.c_str()));
		}

		for (uint32_t i = 0; i < l_containerSize; i++)
		{
			m_pointLightComponents[i] = g_Engine->Get<ComponentManager>()->Spawn<LightComponent>(m_pointLightEntities[i], false, ObjectLifespan::Scene);
			m_pointLightComponents[i]->m_LightType = LightType::Point;
			m_pointLightComponents[i]->m_LuminousFlux = l_randomLuminousFlux(m_generator);
			m_pointLightComponents[i]->m_ColorTemperature = l_randomColorTemperature(m_generator);
		}

		for (uint32_t i = 0; i < l_matrixDim; i++)
		{
			for (uint32_t j = 0; j < l_matrixDim; j++)
			{
				m_pointLightComponents[i * l_matrixDim + j]->m_Transform.m_pos =
					m_player->m_playerCameraComponent->m_Transform.m_pos +
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

		f_runRayTracing = [&]() { g_Engine->Get<RayTracer>()->Execute(); };
		f_pauseGame = [&]() { allowUpdate = !allowUpdate; };

		g_Engine->Get<HIDService>()->AddButtonStateCallback(ButtonState{ INNO_KEY_N, true }, ButtonEvent{ EventLifeTime::OneShot, &f_runRayTracing });
		g_Engine->Get<HIDService>()->AddButtonStateCallback(ButtonState{ INNO_KEY_F, true }, ButtonEvent{ EventLifeTime::OneShot, &f_pauseGame });

		f_sceneLoadingFinishedCallback = [&]() {
			if (!m_player)
				m_player = new Player();
			m_player->Setup();

			m_posOffset = m_player->m_playerCameraComponent->m_Transform.m_pos;
			m_posOffset.z -= 75.0f;

			m_posOffset = Vec4(0.0f, 0.0f, 0.0f, 1.0f);

			setupReferenceSpheres();
			//setupOcclusionCubes();
			setupOpaqueSpheres();
			setupTransparentCubes();
			setupVolumetricCubes();
			setupPointLights();

			m_ObjectStatus = ObjectStatus::Activated;
			};

		g_Engine->Get<SceneService>()->AddSceneLoadingFinishedCallback(&f_sceneLoadingFinishedCallback, 0);

		return true;
	}

	bool WorldSystem::Initialize()
	{
		bool l_result = true;

		//g_Engine->Get<SceneService>()->Load("..//Res//Scenes//UnitTest.InnoScene");

		//g_Engine->Get<SceneService>()->Load("..//Res//Scenes//GITestBox.InnoScene");
		//g_Engine->Get<SceneService>()->Load("..//Res//Scenes//GITestSibenik.InnoScene");
		//g_Engine->Get<SceneService>()->Load("..//Res//Scenes//GITestSponza_PBR.InnoScene");
		//g_Engine->Get<SceneService>()->Load("..//Res//Scenes//GITestFireplaceRoom.InnoScene");

		f_loadTestScene = []() {
			//g_Engine->Get<SceneService>()->Load("..//Res//Scenes//GITestSponza_PBR.InnoScene");
			//g_Engine->Get<SceneService>()->Load("..//Res//Scenes//GITestSibenik.InnoScene");
			g_Engine->Get<SceneService>()->Load("..//Res//Scenes//GITestBox.InnoScene");
			//g_Engine->Get<SceneService>()->Load("..//Res//Scenes//GITestFireplaceRoom.InnoScene");
			};

		f_convertModel = []() {
			g_Engine->Get<AssetService>()->Import("..//Res//Models//Sponza_PBR//NewSponza_Merged.fbx");
			};

		g_Engine->Get<HIDService>()->AddButtonStateCallback(ButtonState{ INNO_KEY_R, true }, ButtonEvent{ EventLifeTime::OneShot, &f_loadTestScene });
		g_Engine->Get<HIDService>()->AddButtonStateCallback(ButtonState{ INNO_KEY_Y, true }, ButtonEvent{ EventLifeTime::OneShot, &f_convertModel });

		return true;
	}

	// bool WorldSystem::updateMaterial(Model* model, Vec4 albedo, Vec4 MRAT, ShaderModel shaderModel)
	// {
	// 	if (!model)
	// 		return false;

	// 	for (uint64_t j = 0; j < model->renderableSets.m_count; j++)
	// 	{
	// 		// auto l_pair = g_Engine->Get<AssetService>()->GetRenderableSet(model->renderableSets.m_startOffset + j);
	// 		// l_pair->material->m_materialAttributes.AlbedoR = albedo.x;
	// 		// l_pair->material->m_materialAttributes.AlbedoG = albedo.y;
	// 		// l_pair->material->m_materialAttributes.AlbedoB = albedo.z;
	// 		// l_pair->material->m_materialAttributes.Metallic = MRAT.x;
	// 		// l_pair->material->m_materialAttributes.Roughness = MRAT.y;
	// 		// l_pair->material->m_materialAttributes.AO = MRAT.z;
	// 		// l_pair->material->m_materialAttributes.Alpha = albedo.w;
	// 		// l_pair->material->m_materialAttributes.Thickness = MRAT.w;
	// 		// l_pair->material->m_ShaderModel = shaderModel;
	// 	}
	// 	return true;
	// }

	bool WorldSystem::Update()
	{
		if (m_ObjectStatus != ObjectStatus::Activated)
			return false;

		if (!allowUpdate)
			return false;

		auto l_tickTime = g_Engine->getTickTime();
		m_seed += (l_tickTime / 1000.0f);

		auto l_seed = (1.0f - l_tickTime / 100.0f);
		l_seed = l_seed > 0.0f ? l_seed : 0.01f;
		l_seed = l_seed > 0.85f ? 0.85f : l_seed;

		if (m_player)
			m_player->Update(l_seed);

		updateSpheres();
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
		Log(Verbose, "Start test...");
		for (uint32_t i = 0; i < testTime; i++)
		{
			auto l_result = testCase();
			if (!l_result)
			{
				Log(Warning, "Test failure.");
			}
		}

		Log(Verbose, "Finished test for ", testTime, " times.");
	}

	Vec4 WorldSystem::getMousePositionInWorldSpace()
	{
		auto l_screenResolution = g_Engine->Get<RenderingConfigurationService>()->GetScreenResolution();
		auto l_mousePositionSS = g_Engine->Get<HIDService>()->GetMousePosition();

		auto l_x = 2.0f * l_mousePositionSS.x / l_screenResolution.x - 1.0f;
		auto l_y = 1.0f - 2.0f * l_mousePositionSS.y / l_screenResolution.y;
		auto l_z = -1.0f;
		auto l_w = 1.0f;
		Vec4 l_ndcSpace = Vec4(l_x, l_y, l_z, l_w);

		auto l_activeCamera = static_cast<ICameraSystem*>(g_Engine->Get<ComponentManager>()->GetComponentSystem<CameraComponent>())->GetActiveCamera();
		if (l_activeCamera == nullptr)
		{
			return Vec4();
		}
		
		auto pCamera = l_activeCamera->m_projectionMatrix;
		auto rCamera = Math::getInvertRotationMatrix(l_activeCamera->m_Transform.m_rot);
		auto tCamera = Math::getInvertTranslationMatrix(l_activeCamera->m_Transform.m_pos);
		
		l_ndcSpace = pCamera.inverse() * l_ndcSpace;
		l_ndcSpace.z = -1.0f;
		l_ndcSpace.w = 0.0f;
		l_ndcSpace = tCamera.inverse() * l_ndcSpace;
		l_ndcSpace = rCamera.inverse() * l_ndcSpace;
		l_ndcSpace = l_ndcSpace.normalize();
		return l_ndcSpace;
	}

	void WorldSystem::updateSpheres()
	{
		for (uint32_t i = 0; i < m_opaqueSphereModelComponents.size(); i += 4)
		{
			auto l_albedoFactor1 = (sin(m_seed / 2.0f + i) + 1.0f) / 2.0f;
			auto l_albedoFactor2 = (sin(m_seed / 3.0f + i) + 1.0f) / 2.0f;
			auto l_albedoFactor3 = (sin(m_seed / 5.0f + i) + 1.0f) / 2.0f;

			auto l_albedo1 = Vec4(l_albedoFactor1, l_albedoFactor2, l_albedoFactor3, 1.0f);
			auto l_albedo2 = Vec4(l_albedoFactor3, l_albedoFactor2, l_albedoFactor1, 1.0f);
			auto l_albedo3 = Vec4(l_albedoFactor2, l_albedoFactor3, l_albedoFactor1, 1.0f);
			auto l_albedo4 = Vec4(l_albedoFactor2, l_albedoFactor1, l_albedoFactor3, 1.0f);

			auto l_MRATFactor1 = ((sin(m_seed / 4.0f + i) + 1.0f) / 2.001f);
			auto l_MRATFactor2 = ((sin(m_seed / 5.0f + i) + 1.0f) / 2.001f);
			auto l_MRATFactor3 = ((sin(m_seed / 6.0f + i) + 1.0f) / 2.001f);

			// updateMaterial(m_opaqueSphereModelComponents[i]->m_Model, l_albedo1, Vec4(l_MRATFactor1, l_MRATFactor2, 0.0f, 0.0f));
			// updateMaterial(m_opaqueSphereModelComponents[i + 1]->m_Model, l_albedo2, Vec4(l_MRATFactor2, l_MRATFactor1, 0.0f, 0.0f));
			// updateMaterial(m_opaqueSphereModelComponents[i + 2]->m_Model, l_albedo3, Vec4(l_MRATFactor3, l_MRATFactor2, 0.0f, 0.0f));
			// updateMaterial(m_opaqueSphereModelComponents[i + 3]->m_Model, l_albedo4, Vec4(l_MRATFactor3, l_MRATFactor1, 0.0f, 0.0f));
		}

		for (uint32_t i = 0; i < m_transparentCubeModelComponents.size(); i++)
		{
			auto l_albedo = Math::HSVtoRGB(Vec4((sin(m_seed / 6.0f + i) * 0.5f + 0.5f) * 360.0f, 1.0f, 1.0f, 0.5f));
			l_albedo.w = sin(m_seed / 6.0f + i) * 0.5f + 0.5f;
			auto l_MRAT = Vec4(0.0f, sin(m_seed / 4.0f + i) * 0.5f + 0.5f, 1.0f, clamp((float)sin(m_seed / 5.0f + i) * 0.5f + 0.5f, epsilon<float, 4>, 1.0f));
			//updateMaterial(m_transparentCubeModelComponents[i]->m_Model, l_albedo, l_MRAT, ShaderModel::Transparent);
		}

		for (uint32_t i = 0; i < m_volumetricCubeModelComponents.size(); i++)
		{
			auto l_albedo = Math::HSVtoRGB(Vec4((sin(m_seed / 6.0f + i) * 0.5f + 0.5f) * 360.0f, 1.0f, 1.0f, 0.5f));
			l_albedo.w = clamp((float)sin(m_seed / 7.0f + i) * 0.5f + 0.5f, epsilon<float, 4>, 1.0f);
			auto l_MRAT = Vec4(clamp((float)sin(m_seed / 5.0f + i) * 0.5f + 0.5f, epsilon<float, 4>, 1.0f), 1.0f, 1.0f, 1.0f);
			//updateMaterial(m_volumetricCubeModelComponents[i]->m_Model, l_albedo, l_MRAT, ShaderModel::Volumetric);
		}

		if (m_referenceSphereModelComponents.size() == 0)
			return;

		for (uint32_t i = 0; i < m_matrixDim; i++)
		{
			for (uint32_t j = 0; j < m_matrixDim; j++)
			{
				auto l_MRAT = Vec4((float)i / (float)(m_matrixDim - 1), (float)j / (float)(m_matrixDim - 1), 0.0f, 1.0f);
				//updateMaterial(m_referenceSphereModelComponents[i * m_matrixDim + j]->m_Model, Vec4(1.0f, 1.0f, 1.0f, 1.0f), l_MRAT);
			}
		}
	}
}