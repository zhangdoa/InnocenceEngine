#pragma once
#include "nlohmann/json.hpp"
using json = nlohmann::ordered_json;

#include "../../Common/ComponentHeaders.h"
#include "../../Component/RenderPassComponent.h"

namespace Inno
{
	namespace JSONWrapper
	{
		bool loadJsonDataFromDisk(const char* fileName, json& data);
		bool saveJsonDataToDisk(const char* fileName, const json& data);

		void to_json(json& j, const Entity& p);
		void to_json(json& j, const Vec4& p);
		void to_json(json& j, const Mat4& p);

		void to_json(json& j, const TransformComponent& p);
		void to_json(json& j, const TransformVector& p);
		void to_json(json& j, const ModelComponent& p);
		void to_json(json& j, const LightComponent& p);
		void to_json(json& j, const CameraComponent& p);
		void to_json(json& j, const RenderPassComponent& p);

		void from_json(const json& j, Vec4& p);
		void from_json(const json& j, Mat4& p);

		void from_json(const json& j, TransformComponent& p);
		void from_json(const json& j, TransformVector& p);
		void from_json(const json& j, ModelComponent& p);
		void from_json(const json& j, LightComponent& p);
		void from_json(const json& j, CameraComponent& p);
		void from_json(const json& j, RenderPassComponent& p);

		Model* loadModelFromDisk(const char* fileName, bool AsyncUploadGPUResource = true);

		bool saveScene(const char* fileName);
		bool loadScene(const char* fileName);
	}
}