#include "PointLightComponentManager.h"
#include "../Component/PointLightComponent.h"
#include "../Core/InnoMemory.h"
#include "../Core/InnoLogger.h"
#include "../Common/CommonMacro.inl"
#include "CommonFunctionDefinitionMacro.inl"
#include "../Common/InnoMathHelper.h"

#include "../ModuleManager/IModuleManager.h"

extern IModuleManager* g_pModuleManager;

namespace PointLightComponentManagerNS
{
	const size_t m_MaxComponentCount = 1024;
	size_t m_CurrentComponentIndex = 0;
	IObjectPool* m_ComponentPool;
	ThreadSafeVector<PointLightComponent*> m_Components;
	ThreadSafeUnorderedMap<InnoEntity*, PointLightComponent*> m_ComponentsMap;

	std::function<void()> f_SceneLoadingStartCallback;
	std::function<void()> f_SceneLoadingFinishCallback;

	void UpdateColorTemperature(PointLightComponent* rhs);
	void UpdateAttenuationRadius(PointLightComponent* rhs);
}

void PointLightComponentManagerNS::UpdateColorTemperature(PointLightComponent * rhs)
{
	if (rhs->m_UseColorTemperature)
	{
		rhs->m_RGBColor = InnoMath::colorTemperatureToRGB(rhs->m_ColorTemperature);
	}
}

void PointLightComponentManagerNS::UpdateAttenuationRadius(PointLightComponent* rhs)
{
	auto l_RGBColor = rhs->m_RGBColor.normalize();
	// "Real-Time Rendering", 4th Edition, p.278
	// https://en.wikipedia.org/wiki/Relative_luminance
	// weight with respect to CIE photometric curve
	auto l_relativeLuminanceRatio = (0.2126f * l_RGBColor.x + 0.7152f * l_RGBColor.y + 0.0722f * l_RGBColor.z);

	// Luminance (nt) is illuminance (lx) per solid angle, while luminous intensity (cd) is luminous flux (lm) per solid angle, thus for one area unit (m^2), the ratio of nt/lx is same as cd/lm
	// For omni isotropic light, after the intergration per solid angle, the luminous flux (lm) is 4 pi times the luminous intensity (cd)
	auto l_weightedLuminousFlux = rhs->m_LuminousFlux * l_relativeLuminanceRatio;

	// 1. get luminous efficacy (lm/w), assume 683 lm/w (100% luminous efficiency) always
	// 2. luminous flux (lm) to radiant flux (w), omitted because linearity assumption in step 1
	// 3. apply inverse square attenuation law with a low threshold of eye sensitivity at 0.03 lx, in ideal situation, lx could convert back to lm with respect to a sphere surface area 4 * PI * r^2
#if defined INNO_PLATFORM_WIN
	rhs->m_attenuationRadius = std::sqrtf(l_weightedLuminousFlux / (4.0f * PI<float> * 0.03f));
#else
	rhs->m_attenuationRadius = sqrtf(l_weightedLuminousFlux / (4.0f * PI<float> * 0.03f));
#endif
}

using namespace PointLightComponentManagerNS;

bool InnoPointLightComponentManager::Setup()
{
	m_ComponentPool = InnoMemory::CreateObjectPool(sizeof(PointLightComponent), m_MaxComponentCount);
	m_Components.reserve(m_MaxComponentCount);

	f_SceneLoadingStartCallback = [&]() {
		CleanComponentContainers(PointLightComponent);
	};

	f_SceneLoadingFinishCallback = [&]() {
	};

	g_pModuleManager->getFileSystem()->addSceneLoadingStartCallback(&f_SceneLoadingStartCallback);
	g_pModuleManager->getFileSystem()->addSceneLoadingFinishCallback(&f_SceneLoadingFinishCallback);

	return true;
}

bool InnoPointLightComponentManager::Initialize()
{
	return true;
}

bool InnoPointLightComponentManager::Simulate()
{
	for (auto i : m_Components)
	{
		UpdateColorTemperature(i);
		UpdateAttenuationRadius(i);
	}
	return true;
}

bool InnoPointLightComponentManager::Terminate()
{
	return true;
}

InnoComponent * InnoPointLightComponentManager::Spawn(const InnoEntity* parentEntity, ObjectSource objectSource, ObjectUsage objectUsage)
{
	SpawnComponentImpl(PointLightComponent);
}

void InnoPointLightComponentManager::Destory(InnoComponent * component)
{
	DestroyComponentImpl(PointLightComponent);
}

InnoComponent* InnoPointLightComponentManager::Find(const InnoEntity * parentEntity)
{
	GetComponentImpl(PointLightComponent, parentEntity);
}

const std::vector<PointLightComponent*>& InnoPointLightComponentManager::GetAllComponents()
{
	return m_Components.getRawData();
}