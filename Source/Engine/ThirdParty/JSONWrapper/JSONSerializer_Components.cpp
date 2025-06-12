#include "JSONWrapper.h"
#include "../../Services/EntityManager.h"
#include "../../Services/ComponentManager.h"
#include "../../Services/TemplateAssetService.h"
#include "../../Services/PhysicsSimulationService.h"
#include "../STBWrapper/STBWrapper.h"

#include "../../Engine.h"
using namespace Inno;

void JSONWrapper::to_json(json& j, const ModelComponent& component)
{
    json transform;
    to_json(transform, component.m_Transform);

    j = json
    {
        {"ComponentType", component.GetTypeID()},
        {"Transform", transform},
        // @TODO: Finish all the writings of the child components
    };
}

void JSONWrapper::to_json(json& j, const LightComponent& component)
{
    json color;
    to_json(color, component.m_RGBColor);

    json shape;
    to_json(shape, component.m_Shape);

    json transform;
    to_json(transform, component.m_Transform);

    j = json
    {
        {"ComponentType", component.GetTypeID()},
        {"RGBColor", color},
        {"Shape", shape},
        {"LightType", component.m_LightType},
        {"ColorTemperature", component.m_ColorTemperature},
        {"LuminousFlux", component.m_LuminousFlux},
        {"UseColorTemperature", component.m_UseColorTemperature},
        {"Transform", transform},
    };
}

void JSONWrapper::to_json(json& j, const CameraComponent& component)
{
    json transform;
    to_json(transform, component.m_Transform);

    j = json
    {
        {"ComponentType", component.GetTypeID()},
        {"FOVX", component.m_FOVX},
        {"WidthScale", component.m_widthScale},
        {"HeightScale", component.m_heightScale},
        {"zNear", component.m_zNear},
        {"zFar", component.m_zFar},
        {"Aperture", component.m_aperture},
        {"ShutterTime", component.m_shutterTime},
        {"ISO", component.m_ISO},
        {"Transform", transform},
    };
}

bool JSONWrapper::Load(const char* fileName, ModelComponent& component)
{
    json j;
    if (!Load(fileName, j))
        return false;

    from_json(j["Transform"], component.m_Transform);
    if (j.find("DrawCallComponents") != j.end())
    {
        auto l_j = j["DrawCallComponents"];
        component.m_DrawCallComponents.reserve(l_j.size());
        for (auto& i : l_j)
        {
            auto l_drawCallComponent = g_Engine->Get<ComponentManager>()->Load<DrawCallComponent>(i["Name"].get<std::string>().c_str(), component.m_Owner);
            component.m_DrawCallComponents.emplace_back(l_drawCallComponent);
        }
    }

    if (j.find("AnimationComponents") != j.end())
    {
        auto l_j = j["AnimationComponents"];
        // @TODO: Implement AnimationComponent loading
    }

    return true;
}

bool  JSONWrapper::Load(const char* fileName, DrawCallComponent& component)
{
    json j;
    if (!Load(fileName, j))
        return false;

    if (j.find("MeshComponent") != j.end())
    {
        auto l_j = j["MeshComponent"];
        component.m_MeshComponent = g_Engine->Get<ComponentManager>()->Load<MeshComponent>(l_j["Name"].get<std::string>().c_str(), component.m_Owner);
    }

    if (j.find("MaterialComponent") != j.end())
    {
        auto l_j = j["MaterialComponent"];
        component.m_MaterialComponent = g_Engine->Get<ComponentManager>()->Load<MaterialComponent>(l_j["Name"].get<std::string>().c_str(), component.m_Owner);
    }

    return true;
}

bool JSONWrapper::Load(const char* fileName, MeshComponent& component)
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

    component.m_ObjectStatus = ObjectStatus::Created;
    g_Engine->getRenderingServer()->Initialize(&component, l_vertices, l_indices);

    return true;
}

bool JSONWrapper::Load(const char* fileName, MaterialComponent& component)
{
    json j;
    if (!Load(fileName, j))
        return false;

    if (j.find("TextureComponents") != j.end())
    {
        auto l_j = j["TextureComponents"];
        component.m_TextureComponents.reserve(l_j.size());
        for (auto& i : l_j)
        {
            auto l_textureComponent = g_Engine->Get<ComponentManager>()->Load<TextureComponent>(i["Name"].get<std::string>().c_str(), component.m_Owner);
            component.m_TextureComponents.emplace_back(l_textureComponent);
        }
    }

    component.m_materialAttributes.AlbedoR = j["Albedo"]["R"];
    component.m_materialAttributes.AlbedoG = j["Albedo"]["G"];
    component.m_materialAttributes.AlbedoB = j["Albedo"]["B"];
    component.m_materialAttributes.Alpha = j["Albedo"]["A"];
    component.m_materialAttributes.Metallic = j["Metallic"];
    component.m_materialAttributes.Roughness = j["Roughness"];
    component.m_materialAttributes.AO = j["AO"];
    component.m_materialAttributes.Thickness = j["Thickness"];
    component.m_ShaderModel = ShaderModel(j["ShaderModel"]);

    component.m_ObjectStatus = ObjectStatus::Created;
    g_Engine->getRenderingServer()->Initialize(&component);

    return true;
}

bool JSONWrapper::Load(const char* fileName, TextureComponent& component)
{
    json j;
    if (!Load(fileName, j))
        return false;

    component.m_TextureDesc.Sampler = TextureSampler(j["Sampler"]);
    component.m_TextureDesc.Usage = TextureUsage(j["Usage"]);
    component.m_TextureDesc.IsSRGB = j["IsSRGB"];

    void* textureData = STBWrapper::Load(("../Data/Components/" + j["File"].get<std::string>()).c_str(), component);
    component.m_ObjectStatus = ObjectStatus::Created;

    g_Engine->getRenderingServer()->Initialize(&component, textureData);

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

    from_json(j["Transform"], component.m_Transform);
    from_json(j["RGBColor"], component.m_RGBColor);
    from_json(j["Shape"], component.m_Shape);
    
    int lightTypeValue = j["LightType"].get<int>();
    component.m_LightType = static_cast<LightType>(lightTypeValue);
    Log(Verbose, "Loaded LightComponent from ", fileName, " with LightType: ", lightTypeValue, " (", static_cast<int>(component.m_LightType), ")");
    
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

    from_json(j["Transform"], component.m_Transform);
    component.m_FOVX = j["FOVX"];
    component.m_widthScale = j["WidthScale"];
    component.m_heightScale = j["HeightScale"];
    component.m_zNear = j["zNear"];
    component.m_zFar = j["zFar"];
    component.m_aperture = j["Aperture"];
    component.m_shutterTime = j["ShutterTime"];
    component.m_ISO = j["ISO"];

    return true;
}