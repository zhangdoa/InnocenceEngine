#include "JSONWrapper.h"
#include "../../Common/IOService.h"
#include "../../Services/EntityRegistry.h"
#include "../../Services/TemplateAssetService.h"
#include "../../Services/PhysicsSimulationService.h"
#include "../../Services/AssetService.h"
#include "../STBWrapper/STBWrapper.h"

#include "../../Engine.h"
using namespace Inno;

void JSONWrapper::to_json(json& j, const TransformComponent& component)
{
    Transform t;
    t.m_pos   = component.m_LocalPos;
    t.m_rot   = component.m_LocalRot;
    t.m_scale = component.m_LocalScale;
    json xfJson;
    to_json(xfJson, t);
    j["ComponentType"] = TransformComponent::GetTypeID();
    j["Position"] = xfJson["Position"];
    j["Rotation"] = xfJson["Rotation"];
    j["Scale"]    = xfJson["Scale"];
}

void JSONWrapper::to_json(json& j, const LightComponent& component)
{
    json color;
    to_json(color, component.m_RGBColor);

    json shape;
    to_json(shape, component.m_Shape);

    j = json
    {
        {"ComponentType", component.GetTypeID()},
        {"RGBColor", color},
        {"Shape", shape},
        {"LightType", component.m_LightType},
        {"ColorTemperature", component.m_ColorTemperature},
        {"LuminousFlux", component.m_LuminousFlux},
        {"UseColorTemperature", component.m_UseColorTemperature},
    };
}

void JSONWrapper::to_json(json& j, const CameraComponent& component)
{
    j = json
    {
        {"ComponentType", component.GetTypeID()},
        {"FOVX", component.m_FOVX},
        {"WidthScale", component.m_WidthScale},
        {"HeightScale", component.m_HeightScale},
        {"zNear", component.m_ZNear},
        {"zFar", component.m_ZFar},
        {"Aperture", component.m_Aperture},
        {"ShutterTime", component.m_ShutterTime},
        {"ISO", component.m_ISO},
    };
}

void JSONWrapper::to_json(json& j, const MeshComponent& component)
{
    j = json
    {
        {"MeshShape", MeshShape::Customized}
    };

    // Note: For binary mesh data, additional fields are added by AssetService::Save
}

void JSONWrapper::to_json(json& j, const MaterialComponent& component)
{
    auto* l_asset = AssetService::GetMaterialAsset(component.m_Asset);
    if (!l_asset)
    {
        j = json{};
        return;
    }

    j = json
    {
        {"ShaderModel", l_asset->m_ShaderModel},
        {"Albedo", {
            {"R", l_asset->m_Attributes.AlbedoR},
            {"G", l_asset->m_Attributes.AlbedoG},
            {"B", l_asset->m_Attributes.AlbedoB},
            {"A", l_asset->m_Attributes.Alpha}
        }},
        {"Metallic", l_asset->m_Attributes.Metallic},
        {"Roughness", l_asset->m_Attributes.Roughness},
        {"AO", l_asset->m_Attributes.AO},
        {"Thickness", l_asset->m_Attributes.Thickness}
    };

    json textureComponents = json::array();
    for (const auto& textureName : l_asset->m_TextureNames)
    {
        json textureJson;
        textureJson["Name"] = textureName;
        textureComponents.push_back(textureJson);
    }
    j["TextureComponents"] = textureComponents;
}

void JSONWrapper::to_json(json& j, const TextureComponent& component)
{
    j = json
    {
        {"ComponentType", component.GetTypeID()},
        {"Sampler", component.m_TextureDesc.Sampler},
        {"Usage", component.m_TextureDesc.Usage},
        {"IsSRGB", component.m_TextureDesc.IsSRGB}
    };

    // Note: For binary texture data, additional fields are added by AssetService::Save
}

bool JSONWrapper::Load(const char* fileName, TransformComponent& component)
{
    json j;
    if (!Load(fileName, j))
        return false;

    Transform t;
    from_json(j, t);
    component.m_LocalPos   = t.m_pos;
    component.m_LocalRot   = t.m_rot;
    component.m_LocalScale = t.m_scale;
    return true;
}

bool JSONWrapper::Load(const char* fileName, MeshComponent& component, EntityID owner)
{
    json j;
    if (!Load(fileName, j))
        return false;

    MeshShape l_meshShape = MeshShape(j["MeshShape"]);
    if (l_meshShape != MeshShape::Customized)
    {
        component = *g_Engine->Get<TemplateAssetService>()->GetMeshComponent(l_meshShape);
        return true;
    }

    auto l_meshFileName = j["File"].get<std::string>();
    std::ifstream l_meshFile("../Data/Components/" + l_meshFileName, std::ios::binary);
    if (!l_meshFile.is_open())
    {
        Log(Error, "Can't open file ", l_meshFileName.c_str(), "!");
        return false;
    }

    size_t l_verticesNumber = j["VerticesNumber"];
    size_t l_indicesNumber = j["IndicesNumber"];

    std::vector<Vertex> l_vertices;
    l_vertices.resize(l_verticesNumber);

    std::vector<Index> l_indices;
    l_indices.resize(l_indicesNumber);

    g_Engine->Get<IOService>()->deserializeVector(l_meshFile, 0, l_verticesNumber * sizeof(Vertex), l_vertices);
    g_Engine->Get<IOService>()->deserializeVector(l_meshFile, l_verticesNumber * sizeof(Vertex), l_indicesNumber * sizeof(Index), l_indices);

    l_meshFile.close();

    if (j.find("Bones") != j.end())
    {
        auto l_bones = j["Bones"];
        std::string l_skeletonName = l_bones["Name"];
        // @TODO: Implement SkeletonComponent loading
    }

    g_Engine->getGraphicsService()->Initialize(&component, l_vertices, l_indices, owner);

    return true;
}

bool JSONWrapper::Load(const char* fileName, MaterialComponent& component, EntityID owner)
{
    json j;
    if (!Load(fileName, j))
        return false;

    auto l_registry = g_Engine->Get<EntityRegistry>();
    auto l_lifespan = (owner != INVALID_ENTITY) ? l_registry->GetLifespan(owner) : ObjectLifespan::Persistence;

    auto l_handle = AssetService::AllocateMaterialAsset(component.m_InstanceName.c_str(), l_lifespan);
    component.m_Asset = l_handle;

    auto* l_asset = AssetService::GetMaterialAsset(l_handle);
    if (!l_asset)
        return false;

    if (j.find("TextureComponents") != j.end())
    {
        auto l_j = j["TextureComponents"];
        l_asset->m_TextureNames.reserve(l_j.size());
        for (const auto& l_entry : l_j)
        {
            l_asset->m_TextureNames.push_back(l_entry["Name"].get<std::string>());
        }
    }

    l_asset->m_Attributes.AlbedoR = j["Albedo"]["R"];
    l_asset->m_Attributes.AlbedoG = j["Albedo"]["G"];
    l_asset->m_Attributes.AlbedoB = j["Albedo"]["B"];
    l_asset->m_Attributes.Alpha = j["Albedo"]["A"];
    l_asset->m_Attributes.Metallic = j["Metallic"];
    l_asset->m_Attributes.Roughness = j["Roughness"];
    l_asset->m_Attributes.AO = j["AO"];
    l_asset->m_Attributes.Thickness = j["Thickness"];
    l_asset->m_ShaderModel = ShaderModel(j["ShaderModel"]);
    l_asset->m_Residency = AssetResidency::Resident;

    g_Engine->getGraphicsService()->Initialize(&component, owner);

    return true;
}

bool JSONWrapper::Load(const char* fileName, TextureComponent& component, EntityID owner)
{
    json j;
    if (!Load(fileName, j))
        return false;

    component.m_TextureDesc.Sampler = TextureSampler(j["Sampler"]);
    component.m_TextureDesc.Usage = TextureUsage(j["Usage"]);
    component.m_TextureDesc.IsSRGB = j["IsSRGB"];

    void* textureData = STBWrapper::Load(("../Data/Components/" + j["File"].get<std::string>()).c_str(), component);

    g_Engine->getGraphicsService()->Initialize(&component, textureData, owner);
    return true;
}

// SkeletonComponent* JSONWrapper::ProcessSkeleton(const json& j, const char* name)
// {
// 	SkeletonComponent* l_SkeletonComp;

// 	// check if this file has already been loaded once
// 	if (g_Engine->Get<AssetService>()->FindLoadedSkeleton(name, l_SkeletonComp))
// 	{
// 		return l_SkeletonComp;
// 	}
// 	else
// 	{
// 		l_SkeletonComp = g_Engine->Get<AnimationService>()->AddSkeletonComponent();
// 		l_SkeletonComp->m_InstanceName = (std::string(name) + ("//")).c_str();

// 		auto l_size = j["Bones"].size();
// 		l_SkeletonComp->m_BoneList.reserve(l_size);
// 		l_SkeletonComp->m_BoneList.fulfill();

// 		for (auto i : j["Bones"])
// 		{
// 			Bone l_boneData;
// 			from_json(i["Transformation"], l_boneData.m_LocalToBoneSpace);
// 			l_SkeletonComp->m_BoneList[i["ID"]] = l_boneData;
// 		}

// 		g_Engine->Get<AssetService>()->RecordLoadedSkeleton(name, l_SkeletonComp);
// 		g_Engine->Get<AnimationService>()->InitializeSkeletonComponent(l_SkeletonComp);

// 		return l_SkeletonComp;
// 	}
// }

// bool JSONWrapper::ProcessAnimations(const json& j)
// {
// 	for (auto i : j)
// 	{
// 		std::string l_animationFileName = i["File"];

// 		std::ifstream l_animationFile(g_Engine->Get<IOService>()->getWorkingDirectory() + l_animationFileName, std::ios::binary);

// 		if (!l_animationFile.is_open())
// 		{
// 			Log(Error, "std::ifstream: can't open file ", l_animationFileName.c_str(), "!");
// 			return false;
// 		}

// 		auto l_ADC = g_Engine->Get<AnimationService>()->AddAnimationComponent();
// 		l_ADC->m_InstanceName = (l_animationFileName + "//").c_str();

// 		std::streamoff l_offset = 0;

// 		g_Engine->Get<IOService>()->deserialize(l_animationFile, l_offset, &l_ADC->m_Duration);
// 		l_offset += sizeof(l_ADC->m_Duration);
// 		g_Engine->Get<IOService>()->deserialize(l_animationFile, l_offset, &l_ADC->m_NumChannels);
// 		l_offset += sizeof(l_ADC->m_NumChannels);
// 		g_Engine->Get<IOService>()->deserialize(l_animationFile, l_offset, &l_ADC->m_NumTicks);
// 		l_offset += sizeof(l_ADC->m_NumTicks);

// 		auto l_keyDataSize = g_Engine->Get<IOService>()->getFileSize(l_animationFile) - l_offset;
// 		l_ADC->m_KeyData.resize(l_keyDataSize / sizeof(KeyData));
// 		g_Engine->Get<IOService>()->deserializeVector(l_animationFile, l_offset, l_keyDataSize, l_ADC->m_KeyData);

// 		g_Engine->Get<AssetService>()->RecordLoadedAnimation(l_animationFileName.c_str(), l_ADC);
// 		g_Engine->Get<AnimationService>()->InitializeAnimationComponent(l_ADC);
// 	}

// 	return true;
// }

bool JSONWrapper::Load(const char* fileName, LightComponent& component)
{
    json j;
    if (!Load(fileName, j))
        return false;

    from_json(j["RGBColor"], component.m_RGBColor);
    from_json(j["Shape"], component.m_Shape);

    component.m_LightType = LightType(j["LightType"]);
    component.m_ColorTemperature = j["ColorTemperature"];
    component.m_LuminousFlux = j["LuminousFlux"];
    component.m_UseColorTemperature = j["UseColorTemperature"];

    return true;
}

bool JSONWrapper::Load(const char* fileName, CameraComponent& component)
{
    json j;
    if (!Load(fileName, j))
        return false;

    component.m_FOVX = j["FOVX"];
    component.m_WidthScale = j["WidthScale"];
    component.m_HeightScale = j["HeightScale"];
    component.m_ZNear = j["zNear"];
    component.m_ZFar = j["zFar"];
    component.m_Aperture = j["Aperture"];
    component.m_ShutterTime = j["ShutterTime"];
    component.m_ISO = j["ISO"];

    return true;
}