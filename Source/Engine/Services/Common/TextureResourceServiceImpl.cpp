#include "../TextureResourceService.h"
#include "../../Common/LogService.h"
#include "../../Engine.h"
#include "../../Services/RenderingConfigurationService.h"
#include "../../Services/EntityRegistry.h"

using namespace Inno;

bool TextureResourceService::Setup(IServiceConfig* systemConfig)
{
	auto l_cap = g_Engine->Get<RenderingConfigurationService>()->GetRenderingCapability();
	m_Pool.Initialize(l_cap.maxTextures);
	m_ObjectStatus = ObjectStatus::Activated;
	Log(Success, "TextureResourceService Setup finished.");
	return true;
}

bool TextureResourceService::Terminate()
{
	m_Pool.Terminate();
	m_ObjectStatus = ObjectStatus::Terminated;
	Log(Success, "TextureResourceService has been terminated.");
	return true;
}

TextureComponent* TextureResourceService::Add(const char* name)
{
	return m_Pool.Allocate(name);
}

bool TextureResourceService::Delete(TextureComponent* ptr)
{
	m_Pool.Release(ptr);
	return true;
}

TextureComponent* TextureResourceService::Find(const char* name)
{
	return m_Pool.Find(name);
}

void TextureResourceService::Initialize(TextureComponent* texture, void* textureData, EntityID owner)
{
	if (texture->m_ObjectStatus == ObjectStatus::Activated)
		return;

	m_DeferredQueue.push(TextureInitTask(texture, textureData, owner));
	Log(Verbose, "TextureComponent ", texture->m_InstanceName, " queued for deferred initialization");
}

bool TextureResourceService::InitializeSynchronous(TextureComponent* texture, void* textureData)
{
	if (texture->m_ObjectStatus == ObjectStatus::Activated)
		return true;

	bool l_result = InitializeImpl(texture, textureData);
	if (l_result)
		texture->m_ObjectStatus = ObjectStatus::Activated;

	return l_result;
}

bool TextureResourceService::InitializeComponents()
{
	while (m_DeferredQueue.size() > 0)
	{
		TextureInitTask l_task(nullptr, nullptr);
		m_DeferredQueue.tryPop(l_task);

		if (!l_task.m_Component)
			continue;

		TextureComponent* l_texture = l_task.m_Component;
		if (l_task.m_Owner != INVALID_ENTITY)
		{
			TextureComponent* l_current = g_Engine->Get<EntityRegistry>()->Get<TextureComponent>(l_task.m_Owner);
			if (l_current)
				l_texture = l_current;
			else
				Log(Warning, "TextureInitTask: entity ", l_task.m_Owner, " no longer has TextureComponent, using stored pointer");
		}

		Log(Verbose, "Processing deferred texture initialization for: ", l_texture->m_InstanceName);
		if (InitializeImpl(l_texture, l_task.m_TextureData))
			l_texture->m_ObjectStatus = ObjectStatus::Activated;
		else
			m_DeferredQueue.push(std::move(l_task));
	}

	return true;
}

bool TextureResourceService::OnSceneUnloading()
{
	auto l_registry = g_Engine->Get<EntityRegistry>();
	auto l_isSceneBound = [&](EntityID owner) {
		return owner != INVALID_ENTITY
			&& l_registry->GetLifespan(owner) == ObjectLifespan::Scene;
	};

	std::vector<TextureInitTask> l_persistent;
	TextureInitTask l_task(nullptr, nullptr);
	while (m_DeferredQueue.tryPop(l_task))
	{
		if (!l_isSceneBound(l_task.m_Owner))
			l_persistent.push_back(std::move(l_task));
	}
	for (auto& t : l_persistent)
		m_DeferredQueue.push(std::move(t));

	return true;
}
