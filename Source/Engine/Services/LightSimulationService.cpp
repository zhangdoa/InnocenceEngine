#include "LightSimulationService.h"
#include "../Common/STL17.h"
#include "../Component/LightComponent.h"
#include "../Common/MathHelper.h"
#include "EntityRegistry.h"
#include "../Engine.h"

using namespace Inno;

bool LightSimulationService::Setup(IServiceConfig*)
{
	m_ObjectStatus = ObjectStatus::Created;
	return true;
}

bool LightSimulationService::Initialize()
{
	m_ObjectStatus = ObjectStatus::Activated;
	return true;
}

bool LightSimulationService::Update()
{
	if (m_ObjectStatus != ObjectStatus::Activated)
		return true;

	auto& l_Storage = g_Engine->Get<EntityRegistry>()->Storage<LightComponent>();
	auto& l_Lights = l_Storage.All();

	for (LightComponent& l_Light : l_Lights)
	{
		if (l_Light.m_UseColorTemperature)
			l_Light.m_RGBColor = Math::ColorTemperatureToRGB(l_Light.m_ColorTemperature);

		if (l_Light.m_LightType == LightType::Point)
		{
			auto l_NormalizedColor = l_Light.m_RGBColor.normalize();
			auto l_RelativeLuminanceRatio = 0.2126f * l_NormalizedColor.x + 0.7152f * l_NormalizedColor.y + 0.0722f * l_NormalizedColor.z;
			auto l_WeightedLuminousFlux = l_Light.m_LuminousFlux * l_RelativeLuminanceRatio;
			l_Light.m_Shape.x = std::sqrt(l_WeightedLuminousFlux / (4.0f * PI<float> * 0.03f));
		}
	}

	return true;
}

bool LightSimulationService::Terminate()
{
	m_ObjectStatus = ObjectStatus::Terminated;
	return true;
}

ObjectStatus LightSimulationService::GetStatus()
{
	return m_ObjectStatus;
}
