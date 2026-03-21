#include "EntityRegistry.h"

#include "../Engine.h"
#include "../Common/LogService.h"

using namespace Inno;

bool EntityRegistry::Setup(ISystemConfig*)
{
    m_Valid.assign(MAX_ENTITIES, false);
    m_Lifespans.assign(MAX_ENTITIES, ObjectLifespan::Invalid);
    m_Names.assign(MAX_ENTITIES, {});

    m_ObjectStatus = ObjectStatus::Created;
    Log(Success, "EntityRegistry: Setup.");
    return true;
}

bool EntityRegistry::Initialize()
{
    m_ObjectStatus = ObjectStatus::Activated;
    Log(Success, "EntityRegistry: Initialized.");
    return true;
}

bool EntityRegistry::Update()
{
    return true;
}

bool EntityRegistry::Terminate()
{
    m_Storages.clear();
    m_FreeList.clear();
    m_Valid.assign(MAX_ENTITIES, false);
    m_Lifespans.assign(MAX_ENTITIES, ObjectLifespan::Invalid);
    m_Names.assign(MAX_ENTITIES, {});
    m_NextID = 1;
    m_ObjectStatus = ObjectStatus::Terminated;
    Log(Success, "EntityRegistry: Terminated.");
    return true;
}

ObjectStatus EntityRegistry::GetStatus()
{
    return m_ObjectStatus;
}

EntityID EntityRegistry::Spawn(ObjectLifespan Lifespan, const char* Name)
{
    EntityID l_Id;
    if (!m_FreeList.empty())
    {
        l_Id = m_FreeList.back();
        m_FreeList.pop_back();
    }
    else
    {
        if (m_NextID >= MAX_ENTITIES)
        {
            Log(Error, "EntityRegistry: MAX_ENTITIES reached, cannot spawn more entities.");
            return INVALID_ENTITY;
        }
        l_Id = m_NextID++;
    }

    m_Valid[l_Id]     = true;
    m_Lifespans[l_Id] = Lifespan;
    m_Names[l_Id]     = Name ? Name : "";
    return l_Id;
}

void EntityRegistry::Destroy(EntityID Entity)
{
    if (!IsValid(Entity))
        return;

    // Remove all components for this entity before freeing the slot
    for (auto& [l_Key, l_Wrapper] : m_Storages)
        l_Wrapper->Remove(Entity);

    m_Valid[Entity]      = false;
    m_Lifespans[Entity]  = ObjectLifespan::Invalid;
    m_Names[Entity]      = {};
    m_FreeList.push_back(Entity);
}

bool EntityRegistry::IsValid(EntityID Entity) const
{
    return Entity != INVALID_ENTITY && Entity < MAX_ENTITIES && m_Valid[Entity];
}

const char* EntityRegistry::GetName(EntityID Entity) const
{
    if (!IsValid(Entity))
        return nullptr;
    return m_Names[Entity].c_str();
}

EntityID EntityRegistry::FindByName(const char* Name) const
{
    if (!Name)
        return INVALID_ENTITY;
    for (EntityID l_Id = 1; l_Id < m_NextID; ++l_Id)
    {
        if (m_Valid[l_Id] && m_Names[l_Id] == Name)
            return l_Id;
    }
    return INVALID_ENTITY;
}

std::vector<EntityID> EntityRegistry::GetAllEntityIDs(ObjectLifespan Lifespan) const
{
    std::vector<EntityID> l_Result;
    for (EntityID l_Id = 1; l_Id < m_NextID; ++l_Id)
    {
        if (m_Valid[l_Id] && m_Lifespans[l_Id] == Lifespan)
            l_Result.push_back(l_Id);
    }
    return l_Result;
}

ObjectLifespan EntityRegistry::GetLifespan(EntityID Entity) const
{
    if (!IsValid(Entity))
        return ObjectLifespan::Invalid;
    return m_Lifespans[Entity];
}

void EntityRegistry::CleanUp(ObjectLifespan Lifespan)
{
    // Step 1: Remove all components for matching entities (before freeing slots)
    for (auto& [l_Key, l_Wrapper] : m_Storages)
        l_Wrapper->CleanUp(Lifespan);

    // Step 2: Free entity slots
    for (EntityID l_Id = 1; l_Id < m_NextID; ++l_Id)
    {
        if (m_Valid[l_Id] && m_Lifespans[l_Id] == Lifespan)
        {
            m_Valid[l_Id]     = false;
            m_Lifespans[l_Id] = ObjectLifespan::Invalid;
            m_Names[l_Id]     = {};
            m_FreeList.push_back(l_Id);
        }
    }
}
