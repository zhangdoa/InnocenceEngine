#pragma once
#include "../component/TransformComponent.h"
#include "../component/VisibleComponent.h"
#include "../component/DirectionalLightComponent.h"
#include "../component/PointLightComponent.h"
#include "../component/SphereLightComponent.h"
#include "../component/CameraComponent.h"
#include "../component/InputComponent.h"
#include "../component/EnvironmentCaptureComponent.h"

enum class componentType { TransformComponent, VisibleComponent, DirectionalLightComponent, PointLightComponent, SphereLightComponent, CameraComponent, InputComponent, EnvironmentCaptureComponent };

namespace InnoUtility
{
	template<typename T> std::string getClassName()
	{
		return std::string("Undefined");
	}

#define getClassNameTemplate( className ) \
inline std::string getClassName<className>() \
{ \
	return std::string(#className); \
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
	getClassNameTemplate(MeshDataComponent);
		template<>
	getClassNameTemplate(MaterialDataComponent);
		template<>
	getClassNameTemplate(TextureDataComponent);
		template<>
	getClassNameTemplate(PhysicsDataComponent);

	template<typename T>
	componentType getComponentType();

#define getComponentTypeDefi( className ) \
inline componentType getComponentType<className>() \
{ \
	return componentType::##className; \
}

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
}