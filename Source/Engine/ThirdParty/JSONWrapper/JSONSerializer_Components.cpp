#include "JSONWrapper.h"
#include "../../Common/IOService.h"
#include "../../Services/EntityRegistry.h"
#include "../../Services/TemplateAssetService.h"
#include "../../Services/AssetService.h"
#include "../../Engine.h"
#include "../../Services/TextureResourceService.h"
#include "../../Services/MeshResourceService.h"
#include "../../Services/MaterialResourceService.h"
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
        {"CastShadow", component.m_CastShadow},
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
        {"ExposureMode", static_cast<uint32_t>(component.m_ExposureMode)},
        {"AutoExposureKey", component.m_AutoExposureKey},
        {"AutoExposureCompensation", component.m_AutoExposureCompensation},
    };
}

void JSONWrapper::to_json(json& j, const MeshComponent& component)
{
    j = json
    {
        {"MeshShape", MeshShape::Customized}
    };
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
        {"ComponentType", MaterialComponent::GetTypeID()},
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

    if (component.m_TextureDesc.PixelDataType == TexturePixelDataType::Compressed)
    {
        j["PixelDataFormat"] = component.m_TextureDesc.PixelDataFormat;
        j["PixelDataType"]   = component.m_TextureDesc.PixelDataType;
        j["Width"]           = component.m_TextureDesc.Width;
        j["Height"]          = component.m_TextureDesc.Height;
    }
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
        auto* l_template = g_Engine->Get<TemplateAssetService>()->GetMeshComponent(l_meshShape);
        if (!l_template)
        {
            // TemplateAssetService not yet initialized (called during its own init).
            return false;
        }
        component = *l_template;
        // Queue for deferred activation: the asset may not be Resident yet (BLAS still building).
        // InitializeComponents will activate this component once the shared handle is Resident.
        std::vector<Vertex> l_emptyVerts;
        std::vector<Index>  l_emptyIndices;
        g_Engine->Get<MeshResourceService>()->Initialize(&component, l_emptyVerts, l_emptyIndices, owner);
        return true;
    }

    auto l_meshFileName = j["File"].get<std::string>();
    std::ifstream l_meshFile(AssetService::GetBinaryFilePath(l_meshFileName.c_str()), std::ios::binary);
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

    g_Engine->Get<IOService>()->DeserializeVector(l_meshFile, 0, l_verticesNumber * sizeof(Vertex), l_vertices);
    g_Engine->Get<IOService>()->DeserializeVector(l_meshFile, l_verticesNumber * sizeof(Vertex), l_indicesNumber * sizeof(Index), l_indices);

    l_meshFile.close();

    if (j.find("Bones") != j.end())
    {
        auto l_bones = j["Bones"];
        std::string l_skeletonName = l_bones["Name"];
    }

    g_Engine->Get<MeshResourceService>()->Initialize(&component, l_vertices, l_indices, owner);

    return true;
}

bool JSONWrapper::Load(const char* fileName, MaterialComponent& component, EntityID owner)
{
    json j;
    if (!Load(fileName, j))
        return false;

    auto l_registry = g_Engine->Get<EntityRegistry>();
    auto l_lifespan = (owner != INVALID_ENTITY) ? l_registry->GetLifespan(owner) : ObjectLifespan::Persistence;

    auto l_allocation = AssetService::AllocateMaterialAsset(component.m_InstanceName.c_str(), l_lifespan);
    component.m_Asset = l_allocation.m_Handle;

    auto* l_asset = AssetService::GetMaterialAsset(l_allocation.m_Handle);
    if (!l_asset)
        return false;

    l_asset->m_TextureNames.clear();
    if (!l_allocation.m_WasNewlyCreated)
        l_asset->m_Attributes = MaterialAttributes{};

    if (j.find("TextureComponents") != j.end())
    {
        auto l_j = j["TextureComponents"];
        if (l_j.size() > MaxTextureSlotCount)
        {
            Log(Warning, "Material '", component.m_InstanceName.c_str(),
                "' loads ", l_j.size(), " TextureComponents but GPU layout allows only ",
                MaxTextureSlotCount, "; the extra entries will be dropped at upload.");
        }
        l_asset->m_TextureNames.reserve(l_j.size());
        auto l_textureService = g_Engine->Get<TextureResourceService>();
        for (const auto& l_entry : l_j)
        {
            auto l_textureName = l_entry["Name"].get<std::string>();
            l_asset->m_TextureNames.push_back(l_textureName);

            if (l_textureName.empty())
                continue;

            auto l_existing = l_textureService->Find(l_textureName.c_str());
            if (l_existing)
                continue;

            auto l_filePath = AssetService::GetAssetFilePath(l_textureName.c_str());
            auto l_fullPath = g_Engine->Get<IOService>()->GetDataDirectory() + l_filePath;
            if (!std::filesystem::exists(l_fullPath))
                continue;

            auto l_texturePtr = l_textureService->Add(l_textureName.c_str());
            if (!l_texturePtr)
                continue;

            Load(l_filePath.c_str(), *l_texturePtr, INVALID_ENTITY);
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

    g_Engine->Get<MaterialResourceService>()->Initialize(&component, owner);

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

    if (j.find("PixelDataType")   != j.end()
     && j.find("PixelDataFormat") != j.end()
     && j.find("Width")           != j.end()
     && j.find("Height")          != j.end()
     && TexturePixelDataType(j["PixelDataType"]) == TexturePixelDataType::Compressed)
    {
        component.m_TextureDesc.PixelDataFormat = TexturePixelDataFormat(j["PixelDataFormat"]);
        component.m_TextureDesc.PixelDataType   = TexturePixelDataType::Compressed;
        component.m_TextureDesc.Width           = j["Width"];
        component.m_TextureDesc.Height          = j["Height"];
        component.m_TextureDesc.MipLevels       = 1;
    }

    // Off-load binary decode to the TextureResourceService background loader thread.
    // STBWrapper::Load reads ~64 MB per 4K texture; doing it here synchronously stalls scene loading.
    auto l_binaryPath = AssetService::GetBinaryFilePath(j["File"].get<std::string>().c_str());
    g_Engine->Get<TextureResourceService>()->EnqueueBinaryLoad(l_binaryPath, &component, owner);

    return true;
}

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

    component.m_CastShadow = j.value("CastShadow", component.m_CastShadow);

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

    component.m_ExposureMode = static_cast<ExposureMode>(j.value("ExposureMode", static_cast<uint32_t>(component.m_ExposureMode)));
    component.m_AutoExposureKey = j.value("AutoExposureKey", component.m_AutoExposureKey);
    component.m_AutoExposureCompensation = j.value("AutoExposureCompensation", component.m_AutoExposureCompensation);

    return true;
}