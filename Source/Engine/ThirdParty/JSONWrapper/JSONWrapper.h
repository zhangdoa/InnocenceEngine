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
		void to_json(json& j, const Transform& p);

		void to_json(json& j, const Entity& p);

		void to_json(json& j, const ModelComponent& p);
		void to_json(json& j, const DrawCallComponent& p);
		void to_json(json& j, const MeshComponent& p);
		void to_json(json& j, const MaterialComponent& p);
		void to_json(json& j, const TextureComponent& p);
		void to_json(json& j, const LightComponent& p);
		void to_json(json& j, const CameraComponent& p);
		void to_json(json& j, const RenderPassComponent& p);

		void from_json(const json& j, Vec4& p);
		void from_json(const json& j, Mat4& p);
		void from_json(const json& j, Transform& p);

		void from_json(const json& j, Entity& p);
		void from_json(const json& j, RenderPassComponent& p);

		bool SaveScene(const char* fileName);
		bool LoadScene(const char* fileName);

		bool Load(const char* fileName, ModelComponent& component);
		bool Load(const char* fileName, DrawCallComponent& component);
		bool Load(const char* fileName, MeshComponent& component);
		bool Load(const char* fileName, MaterialComponent& component);
		bool Load(const char* fileName, TextureComponent& component);
		bool Load(const char* fileName, CameraComponent& component);
		bool Load(const char* fileName, LightComponent& component);
	}
}