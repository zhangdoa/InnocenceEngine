#pragma once
#include "../component/TransformComponent.h"
#include "../component/VisibleComponent.h"
#include "../component/DirectionalLightComponent.h"
#include "../component/PointLightComponent.h"
#include "../component/SphereLightComponent.h"
#include "../component/CameraComponent.h"
#include "../component/InputComponent.h"
#include "../component/EnvironmentCaptureComponent.h"
#include "../component/PhysicsDataComponent.h"
#include "../component/MeshDataComponent.h"
#include "../component/MaterialDataComponent.h"
#include "../component/TextureDataComponent.h"

#define getClassNameTemplate( className ) \
inline std::string InnoUtility::getClassName<className>() \
{ \
	return std::string(#className); \
}

#define getComponentTypeDefi( className ) \
inline ComponentType InnoUtility::getComponentType<className>() \
{ \
	return ComponentType::className; \
}

namespace InnoUtility
{
	template<typename T>
	std::string getClassName()
	{
		return std::string("Undefined");
	}

	template<typename T>
	ComponentType getComponentType();

	template<typename T>
	inline std::string pointerToString(T* ptr)
	{
		std::stringstream ss;
		ss << ptr;
		auto l_result = ss.str();
		return l_result;
	}
}

template<>
getClassNameTemplate(TransformComponent);
template<>
getClassNameTemplate(VisibleComponent);
template<>
getClassNameTemplate(DirectionalLightComponent);
template<>
getClassNameTemplate(PointLightComponent);
template<>
getClassNameTemplate(SphereLightComponent);
template<>
getClassNameTemplate(CameraComponent);
template<>
getClassNameTemplate(InputComponent);
template<>
getClassNameTemplate(EnvironmentCaptureComponent);
template<>
getClassNameTemplate(PhysicsDataComponent);
template<>
getClassNameTemplate(MeshDataComponent);
template<>
getClassNameTemplate(MaterialDataComponent);
template<>
getClassNameTemplate(TextureDataComponent);

template<>
getComponentTypeDefi(TransformComponent);
template<>
getComponentTypeDefi(VisibleComponent);
template<>
getComponentTypeDefi(DirectionalLightComponent);
template<>
getComponentTypeDefi(PointLightComponent);
template<>
getComponentTypeDefi(SphereLightComponent);
template<>
getComponentTypeDefi(CameraComponent);
template<>
getComponentTypeDefi(InputComponent);
template<>
getComponentTypeDefi(EnvironmentCaptureComponent);
template<>
getComponentTypeDefi(PhysicsDataComponent);
template<>
getComponentTypeDefi(MeshDataComponent);
template<>
getComponentTypeDefi(MaterialDataComponent);
template<>
getComponentTypeDefi(TextureDataComponent);
