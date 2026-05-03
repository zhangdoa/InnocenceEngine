#include "../MaterialResourceService.h"
#include "../../Common/LogService.h"
#include "../../Common/LogServiceSpecialization.h"
#include "../../Engine.h"
#include "../../Services/RenderingConfigurationService.h"
#include "../../Services/EntityRegistry.h"

using namespace Inno;

bool MaterialResourceService::Setup(IServiceConfig* systemConfig)
{
	auto l_cap = g_Engine->Get<RenderingConfigurationService>()->GetRenderingCapability();
	m_Pool.Initialize(l_cap.maxMaterials);
	m_ObjectStatus = ObjectStatus::Activated;
	Log(Success, "MaterialResourceService Setup finished.");
	return true;
}

bool MaterialResourceService::Terminate()
{
	m_Pool.Terminate();
	m_ObjectStatus = ObjectStatus::Terminated;
	Log(Success, "MaterialResourceService has been terminated.");
	return true;
}

MaterialComponent* MaterialResourceService::Add(const char* name)
{
	return m_Pool.Allocate(name);
}

bool MaterialResourceService::Delete(MaterialComponent* ptr)
{
	m_Pool.Release(ptr);
	return true;
}

MaterialComponent* MaterialResourceService::Find(const char* name)
{
	return m_Pool.Find(name);
}

void MaterialResourceService::Initialize(MaterialComponent* material, EntityID owner)
{
	if (material->m_ObjectStatus == ObjectStatus::Activated)
		return;

	m_DeferredQueue.push(MaterialInitTask(material, owner));
	Log(Verbose, "MaterialComponent ", material->m_InstanceName, " queued for deferred initialization");
}

bool MaterialResourceService::InitializeImpl(MaterialComponent* material)
{
	return true;
}

bool MaterialResourceService::InitializeComponents()
{
	while (m_DeferredQueue.size() > 0)
	{
		MaterialInitTask l_task(nullptr);
		m_DeferredQueue.tryPop(l_task);

		if (!l_task.m_Component)
			continue;

		MaterialComponent* l_material = l_task.m_Component;
		if (l_task.m_Owner != INVALID_ENTITY)
		{
			MaterialComponent* l_current = g_Engine->Get<EntityRegistry>()->Get<MaterialComponent>(l_task.m_Owner);
			if (l_current)
				l_material = l_current;
			else
				Log(Warning, "MaterialInitTask: entity ", l_task.m_Owner, " no longer has MaterialComponent, using stored pointer");
		}

		Log(Verbose, "Processing deferred material initialization for: ", l_material->m_InstanceName);
		if (InitializeImpl(l_material))
			l_material->m_ObjectStatus = ObjectStatus::Activated;
		else
			m_DeferredQueue.push(std::move(l_task));
	}

	return true;
}

MaterialComponent* MaterialResourceService::GetFirstPendingComponent() const
{
	MaterialComponent* l_pending = nullptr;
	m_DeferredQueue.peekFront([&l_pending](const MaterialInitTask& in_Task)
	{
		l_pending = in_Task.m_Component;
	});
	return l_pending;
}

bool MaterialResourceService::OnSceneUnloading()
{
	auto l_registry = g_Engine->Get<EntityRegistry>();
	auto l_isSceneBound = [&](EntityID owner) {
		return owner != INVALID_ENTITY
			&& l_registry->GetLifespan(owner) == ObjectLifespan::Scene;
	};

	std::vector<MaterialInitTask> l_persistent;
	MaterialInitTask l_task(nullptr);
	while (m_DeferredQueue.tryPop(l_task))
	{
		if (!l_isSceneBound(l_task.m_Owner))
			l_persistent.push_back(std::move(l_task));
	}
	for (auto& t : l_persistent)
		m_DeferredQueue.push(std::move(t));

	return true;
}
