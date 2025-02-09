#include "JSONWrapper.h"
#include "../../Common/ThreadSafeQueue.h"
#include "../../Common/LogService.h"
#include "../../Common/IOService.h"

#include "../../Services/ComponentManager.h"
#include "../../Services/RenderingContextService.h"
#include "../../Services/TemplateAssetService.h"
#include "../../Services/AnimationService.h"
#include "../../Services/AssetService.h"
#include "../../Services/EntityManager.h"

#include "../../Engine.h"
using namespace Inno;

namespace Inno
{
	namespace JSONWrapper
	{
		template<typename T>
		inline void Load(const json& j, Entity* entity)
		{
			auto l_result = g_Engine->Get<ComponentManager>()->Spawn<T>(entity, true, ObjectLifespan::Scene);
			from_json(j, *l_result);
		}

		template<typename T>
		inline bool Save(json& topLevel, T* rhs)
		{
			json j;
			to_json(j, *rhs);

			auto l_result = std::find_if(
				topLevel["SceneEntities"].begin(),
				topLevel["SceneEntities"].end(),
				[&](auto val) -> bool {
					return val["UUID"] == rhs->m_Owner->m_UUID;
				});

			if (l_result != topLevel["SceneEntities"].end())
			{
				l_result.value()["ChildrenComponents"].emplace_back(j);
				return true;
			}
			else
			{
				Log(Warning, "saveComponentData<T>: UUID ", rhs->m_Owner->m_UUID, " is invalid.");
				return false;
			}
		}

		Model* ProcessModel(const json& j);
		bool ProcessAnimations(const json& j);
		ArrayRangeInfo ProcessMeshes(const json& j);
		SkeletonComponent* ProcessSkeleton(const json& j, const char* name);
		MaterialComponent* ProcessMaterial(const json& j, const char* name);

		bool PostLoad();

		ThreadSafeQueue<std::pair<TransformComponent*, ObjectName>> m_orphanTransformComponents;
	}
}

bool JSONWrapper::Load(const char* fileName, json& data)
{
	std::ifstream i;

	i.open(g_Engine->Get<IOService>()->getWorkingDirectory() + fileName);

	if (!i.is_open())
	{
		Log(Error, "Can't open JSON file: ", fileName, "!");
		return false;
	}

	i >> data;
	i.close();

	return true;
}

bool JSONWrapper::Save(const char* fileName, const json& data)
{
	std::ofstream o;
	o.open(g_Engine->Get<IOService>()->getWorkingDirectory() + fileName, std::ios::out | std::ios::trunc);
	o << std::setw(4) << data << std::endl;
	o.close();

	Log(Verbose, "JSON file: ", fileName, " has been saved.");

	return true;
}

void JSONWrapper::to_json(json& j, const Entity& p)
{
	j = json
	{
		{"UUID", p.m_UUID},
		{"ObjectName", p.m_InstanceName.c_str()},
	};
}

void JSONWrapper::to_json(json& j, const Vec4& p)
{
	j = json
	{
		{
			"X", p.x
		},
		{
			"Y", p.y
		},
		{
			"Z", p.z
		},
		{
			"W", p.w
		}
	};
}

void JSONWrapper::to_json(json& j, const Mat4& p)
{
	j["00"] = p.m00;
	j["01"] = p.m01;
	j["02"] = p.m02;
	j["03"] = p.m03;
	j["10"] = p.m10;
	j["11"] = p.m11;
	j["12"] = p.m12;
	j["13"] = p.m13;
	j["20"] = p.m20;
	j["21"] = p.m21;
	j["22"] = p.m22;
	j["23"] = p.m23;
	j["30"] = p.m30;
	j["31"] = p.m31;
	j["32"] = p.m32;
	j["33"] = p.m33;
}

void JSONWrapper::from_json(const json& j, Mat4& p)
{
	p.m00 = j["00"];
	p.m01 = j["01"];
	p.m02 = j["02"];
	p.m03 = j["03"];
	p.m10 = j["10"];
	p.m11 = j["11"];
	p.m12 = j["12"];
	p.m13 = j["13"];
	p.m20 = j["20"];
	p.m21 = j["21"];
	p.m22 = j["22"];
	p.m23 = j["23"];
	p.m30 = j["30"];
	p.m31 = j["31"];
	p.m32 = j["32"];
	p.m33 = j["33"];
}

void JSONWrapper::to_json(json& j, const TransformVector& p)
{
	j = json
	{
		{
			"Position",
			{
				{
					"X", p.m_pos.x
				},
				{
					"Y", p.m_pos.y
				},
				{
					"Z", p.m_pos.z
				}
			}
		},
		{
			"Rotation",
			{
				{
					"X", p.m_rot.x
				},
				{
					"Y", p.m_rot.y
				},
				{
					"Z", p.m_rot.z
				},
				{
					"W", p.m_rot.w
				}
			}
		},
		{
			"Scale",
			{
				{
					"X", p.m_scale.x
				},
				{
					"Y", p.m_scale.y
				},
				{
					"Z", p.m_scale.z
				},
			}
		}
	};
}

void JSONWrapper::to_json(json& j, const TransformComponent& p)
{
	json localTransformVector;

	to_json(localTransformVector, p.m_localTransformVector);

	auto parentTransformComponentEntityName = p.m_parentTransformComponent->m_Owner->m_InstanceName;

	j = json
	{
		{"ComponentType", p.GetTypeID()},
		{"ParentTransformComponentEntityName", parentTransformComponentEntityName.c_str()},
		{"LocalTransformVector", localTransformVector },
	};
}

void JSONWrapper::to_json(json& j, const ModelComponent& p)
{
	j = json
	{
		{"ComponentType", p.GetTypeID()},
		{"MeshUsage", p.m_meshUsage},
		{"MeshShape", p.m_MeshShape},
		{"ModelFileName", p.m_modelFileName},
		{"SimulatePhysics", p.m_simulatePhysics},
	};
}

void JSONWrapper::to_json(json& j, const LightComponent& p)
{
	json color;
	to_json(color, p.m_RGBColor);

	json shape;
	to_json(shape, p.m_Shape);

	j = json
	{
		{"ComponentType", p.GetTypeID()},
		{"RGBColor", color},
		{"Shape", shape},
		{"LightType", p.m_LightType},
		{"ColorTemperature", p.m_ColorTemperature},
		{"LuminousFlux", p.m_LuminousFlux},
		{"UseColorTemperature", p.m_UseColorTemperature},
	};
}

void JSONWrapper::to_json(json& j, const CameraComponent& p)
{
	j = json
	{
		{"ComponentType", p.GetTypeID()},
		{"FOVX", p.m_FOVX},
		{"WidthScale", p.m_widthScale},
		{"HeightScale", p.m_heightScale},
		{"zNear", p.m_zNear},
		{"zFar", p.m_zFar},
		{"Aperture", p.m_aperture},
		{"ShutterTime", p.m_shutterTime},
		{"ISO", p.m_ISO},
	};
}

void JSONWrapper::from_json(const json& j, Vec4& p)
{
	p.x = j["X"];
	p.y = j["Y"];
	p.z = j["Z"];
	p.w = j["W"];
}

void JSONWrapper::from_json(const json& j, TransformComponent& p)
{
	from_json(j["LocalTransformVector"], p.m_localTransformVector);
	auto l_parentTransformComponentEntityName = j["ParentTransformComponentEntityName"];
	if (l_parentTransformComponentEntityName == "RootTransform")
	{
		auto l_rootTransformComponent = g_Engine->Get<ComponentManager>()->Get<TransformComponent>(0);
		p.m_parentTransformComponent = l_rootTransformComponent;
	}
	else
	{
		// JSON is an order-irrelevant format, so the parent transform component would always be instantiated at some random points, and it's necessary to assign it later.
		std::string l_parentName = l_parentTransformComponentEntityName;
		l_parentName += "/";
		m_orphanTransformComponents.push({ &p, ObjectName(l_parentName.c_str()) });
	}
}

void JSONWrapper::from_json(const json& j, TransformVector& p)
{
	p.m_pos.x = j["Position"]["X"];
	p.m_pos.y = j["Position"]["Y"];
	p.m_pos.z = j["Position"]["Z"];
	p.m_pos.w = 1.0f;

	p.m_rot.x = j["Rotation"]["X"];
	p.m_rot.y = j["Rotation"]["Y"];
	p.m_rot.z = j["Rotation"]["Z"];
	p.m_rot.w = j["Rotation"]["W"];

	p.m_scale.x = j["Scale"]["X"];
	p.m_scale.y = j["Scale"]["Y"];
	p.m_scale.z = j["Scale"]["Z"];
	p.m_scale.w = 1.0f;
}

void JSONWrapper::from_json(const json& j, ModelComponent& p)
{
	p.m_meshUsage = j["MeshUsage"];
	p.m_MeshShape = j["MeshShape"];
	p.m_modelFileName = j["ModelFileName"];
	p.m_simulatePhysics = j["SimulatePhysics"];
}

void JSONWrapper::from_json(const json& j, LightComponent& p)
{
	from_json(j["RGBColor"], p.m_RGBColor);
	from_json(j["Shape"], p.m_Shape);
	p.m_LightType = j["LightType"];
	p.m_ColorTemperature = j["ColorTemperature"];
	p.m_LuminousFlux = j["LuminousFlux"];
	p.m_UseColorTemperature = j["UseColorTemperature"];
}

void JSONWrapper::from_json(const json& j, CameraComponent& p)
{
	p.m_FOVX = j["FOVX"];
	p.m_widthScale = j["WidthScale"];
	p.m_heightScale = j["HeightScale"];
	p.m_zNear = j["zNear"];
	p.m_zFar = j["zFar"];
	p.m_aperture = j["Aperture"];
	p.m_shutterTime = j["ShutterTime"];
	p.m_ISO = j["ISO"];
}

Model* JSONWrapper::LoadModel(const char* fileName)
{
	json j;

	Load(fileName, j);

	return ProcessModel(j);
}

Model* JSONWrapper::ProcessModel(const json& j)
{
	Model* l_result;

	if (j.find("Meshes") != j.end())
	{
		l_result = g_Engine->Get<AssetService>()->AddModel();
		l_result->renderableSets = ProcessMeshes(j["Meshes"]);
	}

	if (j.find("Animations") != j.end())
	{
		ProcessAnimations(j["Animations"]);
	}

	return l_result;
}

bool JSONWrapper::ProcessAnimations(const json& j)
{
	for (auto i : j)
	{
		std::string l_animationFileName = i["File"];

		std::ifstream l_animationFile(g_Engine->Get<IOService>()->getWorkingDirectory() + l_animationFileName, std::ios::binary);

		if (!l_animationFile.is_open())
		{
			Log(Error, "std::ifstream: can't open file ", l_animationFileName.c_str(), "!");
			return false;
		}

		auto l_ADC = g_Engine->Get<AnimationService>()->AddAnimationComponent();
		l_ADC->m_InstanceName = (l_animationFileName + "//").c_str();

		std::streamoff l_offset = 0;

		g_Engine->Get<IOService>()->deserialize(l_animationFile, l_offset, &l_ADC->m_Duration);
		l_offset += sizeof(l_ADC->m_Duration);
		g_Engine->Get<IOService>()->deserialize(l_animationFile, l_offset, &l_ADC->m_NumChannels);
		l_offset += sizeof(l_ADC->m_NumChannels);
		g_Engine->Get<IOService>()->deserialize(l_animationFile, l_offset, &l_ADC->m_NumTicks);
		l_offset += sizeof(l_ADC->m_NumTicks);

		auto l_keyDataSize = g_Engine->Get<IOService>()->getFileSize(l_animationFile) - l_offset;
		l_ADC->m_KeyData.resize(l_keyDataSize / sizeof(KeyData));
		g_Engine->Get<IOService>()->deserializeVector(l_animationFile, l_offset, l_keyDataSize, l_ADC->m_KeyData);

		g_Engine->Get<AssetService>()->RecordLoadedAnimation(l_animationFileName.c_str(), l_ADC);
		g_Engine->Get<AnimationService>()->InitializeAnimationComponent(l_ADC);
	}

	return true;
}

ArrayRangeInfo JSONWrapper::ProcessMeshes(const json& j)
{
	auto l_result = g_Engine->Get<AssetService>()->AddRenderableSets(j.size());

	uint64_t l_currentIndex = 0;

	for (auto& i : j)
	{
		auto l_currentRenderableSet = g_Engine->Get<AssetService>()->GetRenderableSet(l_result.m_startOffset + l_currentIndex);

		// Load material data
		if (i.find("Material") != i.end())
		{
			std::string l_materialName = i["Name"];
			l_materialName += "_Material";
			l_currentRenderableSet->material = ProcessMaterial(i["Material"], l_materialName.c_str());
		}
		else
		{
			l_currentRenderableSet->material = g_Engine->getRenderingServer()->AddMaterialComponent();
			l_currentRenderableSet->material->m_ObjectStatus = ObjectStatus::Created;
			g_Engine->getRenderingServer()->Initialize(l_currentRenderableSet->material);
		}

		MeshShape l_meshShape = MeshShape(i["MeshShape"].get<int32_t>());

		// Load custom mesh data
		if (l_meshShape == MeshShape::Customized)
		{
			auto l_meshFileName = i["File"].get<std::string>();

			RenderableSet* l_loadedRenderableSet;

			// check if this file has already been loaded once
			if (g_Engine->Get<AssetService>()->FindLoadedRenderableSet(l_meshFileName.c_str(), l_loadedRenderableSet))
			{
				l_currentRenderableSet = l_loadedRenderableSet;
			}
			else
			{
				std::ifstream l_meshFile(g_Engine->Get<IOService>()->getWorkingDirectory() + l_meshFileName, std::ios::binary);

				if (!l_meshFile.is_open())
				{
					Log(Error, "Can't open file ", l_meshFileName.c_str(), "!");
				}

				auto l_mesh = g_Engine->getRenderingServer()->AddMeshComponent();
				l_mesh->m_InstanceName = (l_meshFileName + "//").c_str();

				size_t l_verticesNumber = i["VerticesNumber"];
				size_t l_indicesNumber = i["IndicesNumber"];

				l_mesh->m_Vertices.reserve(l_verticesNumber);
				l_mesh->m_Vertices.fulfill();
				l_mesh->m_Indices.reserve(l_indicesNumber);
				l_mesh->m_Indices.fulfill();

				g_Engine->Get<IOService>()->deserializeVector(l_meshFile, 0, l_verticesNumber * sizeof(Vertex), l_mesh->m_Vertices);
				g_Engine->Get<IOService>()->deserializeVector(l_meshFile, l_verticesNumber * sizeof(Vertex), l_indicesNumber * sizeof(Index), l_mesh->m_Indices);

				l_meshFile.close();

				l_mesh->m_IndexCount = l_mesh->m_Indices.size();

				l_currentRenderableSet->mesh = l_mesh;

				// Load bones data
				if (i.find("Bones") != i.end())
				{
					std::string l_skeletonName = i["Name"];
					l_skeletonName += "_Skeleton";
					l_currentRenderableSet->skeleton = ProcessSkeleton(i, l_skeletonName.c_str());
				}

				l_currentRenderableSet->mesh->m_MeshShape = MeshShape::Customized;
				l_currentRenderableSet->mesh->m_ObjectStatus = ObjectStatus::Created;

				g_Engine->getRenderingServer()->Initialize(l_mesh);

				g_Engine->Get<AssetService>()->RecordLoadedRenderableSet(l_meshFileName.c_str(), l_currentRenderableSet);
			}
		}
		else
		{
			MeshShape l_MeshShape = MeshShape(i["MeshShape"].get<int32_t>());

			l_currentRenderableSet->mesh = g_Engine->Get<TemplateAssetService>()->GetMeshComponent(l_MeshShape);
		}

		l_currentIndex++;
	}

	return l_result;
}

SkeletonComponent* JSONWrapper::ProcessSkeleton(const json& j, const char* name)
{
	SkeletonComponent* l_SkeletonComp;

	// check if this file has already been loaded once
	if (g_Engine->Get<AssetService>()->FindLoadedSkeleton(name, l_SkeletonComp))
	{
		return l_SkeletonComp;
	}
	else
	{
		l_SkeletonComp = g_Engine->Get<AnimationService>()->AddSkeletonComponent();
		l_SkeletonComp->m_InstanceName = (std::string(name) + ("//")).c_str();

		auto l_size = j["Bones"].size();
		l_SkeletonComp->m_BoneList.reserve(l_size);
		l_SkeletonComp->m_BoneList.fulfill();

		for (auto i : j["Bones"])
		{
			Bone l_boneData;
			from_json(i["Transformation"], l_boneData.m_LocalToBoneSpace);
			l_SkeletonComp->m_BoneList[i["ID"]] = l_boneData;
		}

		g_Engine->Get<AssetService>()->RecordLoadedSkeleton(name, l_SkeletonComp);
		g_Engine->Get<AnimationService>()->InitializeSkeletonComponent(l_SkeletonComp);

		return l_SkeletonComp;
	}
}

MaterialComponent* JSONWrapper::ProcessMaterial(const json& j, const char* name)
{
	auto l_MeshComp = g_Engine->getRenderingServer()->AddMaterialComponent();
	l_MeshComp->m_InstanceName = (std::string(name) + ("//")).c_str();
	auto l_defaultMaterial = g_Engine->Get<TemplateAssetService>()->GetDefaultMaterialComponent();

	if (j.find("Textures") != j.end())
	{
		for (auto i : j["Textures"])
		{
			std::string l_textureFile = i["File"];
			size_t l_textureSlotIndex = i["TextureSlotIndex"];

			auto l_TextureComp = g_Engine->Get<AssetService>()->LoadTexture(l_textureFile.c_str());
			if (l_TextureComp)
			{
				l_TextureComp->m_TextureDesc.Sampler = TextureSampler(i["Sampler"]);
				l_TextureComp->m_TextureDesc.Usage = TextureUsage(i["Usage"]);
				l_TextureComp->m_TextureDesc.IsSRGB = i["IsSRGB"];

				l_MeshComp->m_TextureSlots[l_textureSlotIndex].m_Texture = l_TextureComp;
			}
		}
	}

	l_MeshComp->m_materialAttributes.AlbedoR = j["Albedo"]["R"];
	l_MeshComp->m_materialAttributes.AlbedoG = j["Albedo"]["G"];
	l_MeshComp->m_materialAttributes.AlbedoB = j["Albedo"]["B"];
	l_MeshComp->m_materialAttributes.Alpha = j["Albedo"]["A"];
	l_MeshComp->m_materialAttributes.Metallic = j["Metallic"];
	l_MeshComp->m_materialAttributes.Roughness = j["Roughness"];
	l_MeshComp->m_materialAttributes.AO = j["AO"];
	l_MeshComp->m_materialAttributes.Thickness = j["Thickness"];
	l_MeshComp->m_ShaderModel = ShaderModel(j["ShaderModel"]);

	l_MeshComp->m_ObjectStatus = ObjectStatus::Created;

	g_Engine->getRenderingServer()->Initialize(l_MeshComp);

	return l_MeshComp;
}

bool JSONWrapper::Save(const char* fileName)
{
	json topLevel;
	topLevel["SceneName"] = fileName;

	// save entities name and ID
	for (auto i : g_Engine->Get<EntityManager>()->GetEntities())
	{
		if (i->m_Serializable)
		{
			json j;
			to_json(j, *i);
			topLevel["SceneEntities"].emplace_back(j);
		}
	}

	// @TODO: Use ComponentManager to iterate over types
	// save children components
	for (auto i : g_Engine->Get<ComponentManager>()->GetAll<TransformComponent>())
	{
		if (i->m_Serializable)
		{
			Save(topLevel, i);
		}
	}
	for (auto i : g_Engine->Get<ComponentManager>()->GetAll<ModelComponent>())
	{
		if (i->m_Serializable)
		{
			Save(topLevel, i);
		}
	}
	for (auto i : g_Engine->Get<ComponentManager>()->GetAll<LightComponent>())
	{
		if (i->m_Serializable)
		{
			Save(topLevel, i);
		}
	}
	for (auto i : g_Engine->Get<ComponentManager>()->GetAll<CameraComponent>())
	{
		if (i->m_Serializable)
		{
			Save(topLevel, i);
		}
	}

	Save(fileName, topLevel);

	Log(Success, "Scene ", fileName, " has been saved.");

	return true;
}

bool JSONWrapper::Load(const char* fileName)
{
	json j;
	if (!Load(fileName, j))
	{
		return false;
	}

	auto l_sceneName = j["SceneName"];

	for (auto i : j["SceneEntities"])
	{
		std::string l_entityName = i["ObjectName"];
		l_entityName += "/";
		auto l_entity = g_Engine->Get<EntityManager>()->Spawn(true, ObjectLifespan::Scene, l_entityName.c_str());

		for (auto k : i["ChildrenComponents"])
		{
			uint32_t componentTypeID = k["ComponentType"];

			if (componentTypeID == TransformComponent::GetTypeID())
			{
				Load<TransformComponent>(k, l_entity);
			}
			else if (componentTypeID == ModelComponent::GetTypeID())
			{
				Load<ModelComponent>(k, l_entity);
			}
			else if (componentTypeID == LightComponent::GetTypeID())
			{
				Load<LightComponent>(k, l_entity);
			}
			else if (componentTypeID == CameraComponent::GetTypeID())
			{
				Load<CameraComponent>(k, l_entity);
			}
			else
			{
				Log(Error, "Unknown ComponentTypeID: ", componentTypeID);
			}
		}
	}

	Log(Success, "Scene loading finished.");

	PostLoad();

	return true;
}

bool JSONWrapper::PostLoad()
{
	while (m_orphanTransformComponents.size() > 0)
	{
		std::pair<TransformComponent*, ObjectName> l_orphan;
		if (m_orphanTransformComponents.tryPop(l_orphan))
		{
			auto l_entity = g_Engine->Get<EntityManager>()->Find(l_orphan.second.c_str());

			if (l_entity.has_value())
			{
				l_orphan.first->m_parentTransformComponent = g_Engine->Get<ComponentManager>()->Find<TransformComponent>(*l_entity);
			}
			else
			{
				Log(Error, "Can't find TransformComponent with entity name: ", l_orphan.second.c_str(), "!");
			}
		}
	}

	return true;
}