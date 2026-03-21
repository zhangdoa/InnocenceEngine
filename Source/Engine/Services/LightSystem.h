#pragma once
#include "../Interface/ISystem.h"
#include "../Common/Math.h"
#include "../Common/EntityID.h"

namespace Inno
{
	struct LightSystemImpl;
	class LightSystem : public ISystem
	{
	public:
		INNO_CLASS_CONCRETE_NON_COPYABLE(LightSystem);

		bool Setup(ISystemConfig* systemConfig = nullptr) override;
		bool Initialize() override;
		bool Update() override;
		bool Terminate() override;

		ObjectStatus GetStatus() override;

		const std::unordered_map<EntityID, std::vector<Math::AABB>>& GetLitRegionWorldSpace() const;
		const std::unordered_map<EntityID, std::vector<Math::AABB>>& GetLitRegionLightSpace() const;
		const std::unordered_map<EntityID, std::vector<Math::Mat4>>& GetViewMatrices() const;
		const std::unordered_map<EntityID, std::vector<Math::Mat4>>& GetProjectionMatrices() const;

	private:
		LightSystemImpl* m_Impl;
	};
}