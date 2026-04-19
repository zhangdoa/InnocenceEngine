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

		void to_json(json& j, const TransformComponent& p);
		void to_json(json& j, const MeshComponent& p);
		void to_json(json& j, const MaterialComponent& p);
		void to_json(json& j, const TextureComponent& p);
		void to_json(json& j, const LightComponent& p);
		void to_json(json& j, const CameraComponent& p);
		void to_json(json& j, const RenderPassComponent& p);

		void from_json(const json& j, Vec4& p);
		void from_json(const json& j, Mat4& p);
		void from_json(const json& j, Transform& p);

		void from_json(const json& j, RenderPassComponent& p);

		bool SaveChildScene(const char* exportName, const std::vector<std::pair<std::string, std::string>>& drawCalls);

		bool SaveScene(const char* fileName);
		bool LoadScene(const char* fileName);
		bool LoadChildScene(const char* sceneFilePath, EntityID parentEntity);

		// Per-scene map of the filename each component was loaded from.
		// Keyed by (EntityID, component type id). Save consults this so
		// e.g. "Main Camera"'s transform is written back to the original
		// "GITestBox.Camera.TransformComponent" file instead of a new
		// entity-name-based filename. Cleared on scene unload.
		void ClearLoadedCompFilenames();

		bool Load(const char* fileName, TransformComponent& component);
		bool Load(const char* fileName, MeshComponent& component, EntityID owner = INVALID_ENTITY);
		bool Load(const char* fileName, MaterialComponent& component, EntityID owner = INVALID_ENTITY);
		bool Load(const char* fileName, TextureComponent& component, EntityID owner = INVALID_ENTITY);
		bool Load(const char* fileName, CameraComponent& component);
		bool Load(const char* fileName, LightComponent& component);
	}
}