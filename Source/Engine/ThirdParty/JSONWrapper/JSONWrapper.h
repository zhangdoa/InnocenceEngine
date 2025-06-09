#pragma once
#include "nlohmann/json.hpp"
using json = nlohmann::ordered_json;

#include "../../Common/ComponentHeaders.h"
#include "../../Component/RenderPassComponent.h"

namespace Inno
{
	namespace JSONWrapper
	{
		bool Load(const char* fileName, json& data);
		bool Save(const char* fileName, const json& data);

		void to_json(json& j, const Vec4& p);
		void to_json(json& j, const Mat4& p);
		void to_json(json& j, const TransformVector& p);
		
		void to_json(json& j, const Entity& p);

		void to_json(json& j, const TransformComponent& p);
		void to_json(json& j, const ModelComponent& p);
		void to_json(json& j, const LightComponent& p);
		void to_json(json& j, const CameraComponent& p);
		void to_json(json& j, const RenderPassComponent& p);

		void from_json(const json& j, Vec4& p);
		void from_json(const json& j, Mat4& p);
		void from_json(const json& j, TransformVector& p);

		void from_json(const json& j, Entity& p);
		void from_json(const json& j, RenderPassComponent& p);

		bool SaveScene(const char* fileName);
		bool LoadScene(const char* fileName);
		
		void Load(const char* fileName, TransformComponent& component);
		void Load(const char* fileName, ModelComponent& component);
		void Load(const char* fileName, DrawCallComponent& component);
		void Load(const char* fileName, MeshComponent& component);
		void Load(const char* fileName, MaterialComponent& component);
		void Load(const char* fileName, TextureComponent& component);
		void Load(const char* fileName, CameraComponent& component);
		void Load(const char* fileName, LightComponent& component);
	}
}