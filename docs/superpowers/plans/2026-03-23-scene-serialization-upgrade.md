# Scene Serialization Upgrade — Implementation Plan

> **STATUS: COMPLETED** — All tasks implemented and committed on ecs-overhaul branch.

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Restore functional `SaveScene` / `LoadScene` against the current ECS component shapes. Three targeted fixes to broken code left by the ECS migration, plus new `MeshComponent` / `MaterialComponent` scene support and a refreshed `UnitTest.InnoScene`.

**What broke and why:** `SaveScene` was working via `SaveComponentAndAddReference` (which matched components to entities by UUID and called `AssetService::Save`). That function was silently dropped during the `EntityManager → EntityRegistry` migration because the new entities have sequential integer IDs, not UUIDs — the match never fired. End result: `entityJson["Components"] = json::array()` — an empty stub. `AssetService::Save(Camera/Light)` also writes to an empty file path for the same reason (it expected `component->m_InstanceName` from the old `Component` base class; Camera/Light no longer inherit from it).

**Not in scope here — tracked as next plan:** Decoupling asset data (`m_TextureComponents`, `m_MappedMemory_VB/IB`) from `MeshComponent` / `MaterialComponent` into `AssetService`. No component struct changes in this plan.

---

## Scene Format (canonical, as-built)

Every entity lists all its components in a flat `Components` array. **There is no inline `Transform` block at entity level.** `TransformComponent` is serialized as a separate file (type 1) just like any other component.

```json
{
    "Name": "Sun",
    "Components": [
        {"Type": 1, "Name": "Sun.TransformComponent"},
        {"Type": 3, "Name": "Sun.LightComponent"}
    ]
}
```

| Type ID | Component |
|---------|-----------|
| 1 | TransformComponent |
| 3 | LightComponent |
| 4 | CameraComponent |
| 6 | MeshComponent |
| 7 | MaterialComponent |

`Name` in a component entry = base name used with `AssetService::GetAssetFilePath`. For Mesh/Material it is also stored in `m_InstanceName` after load. For TransformComponent, Light, and Camera the name is `<EntityName>.<ComponentTypeName>`.

**Shared preset assets** (e.g. `UnitCubeMesh.MeshComponent`, `DefaultMaterial.MaterialComponent`) are referenced by their preset name rather than entity name. Multiple entities can share the same preset file.

### TransformComponent file format

```json
{
    "ComponentType": 1,
    "Position": {"X": 0.0, "Y": 4.0, "Z": 0.0},
    "Rotation": {"X": 0.0, "Y": 0.0, "Z": 0.0, "W": 1.0},
    "Scale":    {"X": 1.0, "Y": 1.0, "Z": 1.0}
}
```

---

## File Map

| File | Change |
|------|--------|
| `Source/Engine/ThirdParty/JSONWrapper/JSONWrapper.cpp` | Fix `SaveScene`; fix `LoadScene` (add TransformComponent, Mesh, Material branches; remove deprecated branches) |
| `Res/Scenes/UnitTest.InnoScene` | All entities use type-1 TransformComponent file references |
| `Res/Scenes/GITestBox.InnoScene` | All entities use type-1 TransformComponent file references |
| `Data/Components/*.TransformComponent.json` | All per-entity transform files (pre-existing for Sun, Main Camera, Player Character, Landscape Box; created for all GITestBox-specific entities) |

---

## Task 1 — Fix `SaveScene` ✅

**File:** `Source/Engine/ThirdParty/JSONWrapper/JSONWrapper.cpp`

The new `SaveScene` replaces the entity-name-only stub. For each scene entity it:
- Strips the trailing `/` that `Spawn` appends
- Saves TransformComponent to `<EntityName>.TransformComponent.json` and adds a type-1 reference
- For each non-Transform component: pushes `{"Type", "Name"}` and (where appropriate) re-saves the component file

Mesh component files are **not** regenerated — the asset file on disk is the source of truth for mesh shape. Light, Camera, Transform, and Material component files are regenerated because their runtime state can change.

`m_InstanceName` on `MeshComponent` / `MaterialComponent` is the component file base name. Light, Camera, and Transform use `<EntityName>.<TypeName>` naming.

**Implemented `SaveScene`:**
```cpp
bool JSONWrapper::SaveScene(const char* fileName)
{
    auto l_registry = g_Engine->Get<EntityRegistry>();
    auto l_EntityIDs = l_registry->GetAllEntityIDs(ObjectLifespan::Scene);

    json topLevel;
    topLevel["Name"] = g_Engine->Get<IOService>()->getFileName(fileName);
    topLevel["Entities"] = json::array();

    for (auto l_EntityID : l_EntityIDs)
    {
        std::string l_FullName = l_registry->GetName(l_EntityID);
        std::string l_Name = (!l_FullName.empty() && l_FullName.back() == '/')
            ? l_FullName.substr(0, l_FullName.size() - 1)
            : l_FullName;

        json entityJson;
        entityJson["Name"] = l_Name;
        entityJson["Components"] = json::array();

        // TransformComponent — save to file, add type-1 reference
        auto* l_xf = l_registry->Get<TransformComponent>(l_EntityID);
        if (l_xf)
        {
            std::string l_CompName = l_Name + ".TransformComponent";
            json j;
            to_json(j, *l_xf);
            Save(AssetService::GetAssetFilePath(l_CompName.c_str()).c_str(), j);
            entityJson["Components"].push_back({{"Type", TransformComponent::GetTypeID()}, {"Name", l_CompName}});
        }

        // LightComponent, CameraComponent, MeshComponent, MaterialComponent ...
        // (see JSONWrapper.cpp for full implementation)
    }

    Save(fileName, topLevel);
    return true;
}
```

---

## Task 2 — Fix `LoadScene` ✅

**File:** `Source/Engine/ThirdParty/JSONWrapper/JSONWrapper.cpp`

The `Components` loop dispatches on `Type` for all five component types including TransformComponent. No inline `Transform` key is read or supported.

**Implemented `LoadScene`:**
```cpp
for (auto& compJson : entityJson["Components"])
{
    uint32_t    l_TypeID   = compJson["Type"];
    std::string l_CompName = compJson["Name"];
    std::string l_FilePath = AssetService::GetAssetFilePath(l_CompName.c_str());

    if (l_TypeID == TransformComponent::GetTypeID())
    {
        auto& l_Transform = l_registry->Emplace<TransformComponent>(l_EntityID);
        AssetService::Load(l_FilePath.c_str(), l_Transform);
    }
    else if (l_TypeID == LightComponent::GetTypeID()) { ... }
    else if (l_TypeID == CameraComponent::GetTypeID()) { ... }
    else if (l_TypeID == MeshComponent::GetTypeID()) { ... }
    else if (l_TypeID == MaterialComponent::GetTypeID()) { ... }
    else { Log(Warning, "LoadScene: skipping unknown component type ..."); }
}
```

---

## Task 3 — Scene files ✅

`UnitTest.InnoScene` and `GITestBox.InnoScene` both use the type-1 TransformComponent reference format. All required `*.TransformComponent.json` files exist in `Data/Components/`.

**GITestBox entities** (all have separate TransformComponent files):
Sun, Main Camera, pointLight, sphereLight, landscape, building1–4, wall1–5, ceiling, ceiling_middle, Player Character.

---

## Next Plan: Asset Management Decoupling

The following is tracked for a follow-up plan (no implementation here):

- Move `m_TextureComponents` (`std::vector<std::string>`) off `MaterialComponent` → `AssetService` cache keyed by material name
- Move `m_MappedMemory_VB/IB` off `MeshComponent` → local variables in DX12 init (used only for the upload, never read again)
- Remove dead fields from `MaterialComponent`: `m_GPUResourceType`, `m_CPUAccessibility`, `m_GPUAccessibility`, `m_ReadState`, `m_WriteState`, `m_ReadHandles`, `m_WriteHandles`
- `DrawCallService` queries textures via `AssetService::GetMaterialTextures(instanceName)` instead of reading the component directly
