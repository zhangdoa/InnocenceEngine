// TASK-66 / TASK-147 — point/sphere cube-shadow slot allocator + per-light
// PointShadowConstantBuffer populator. Extracted from LightDataService.cpp
// to keep the main translation unit under the file-size soft ratchet.
//
// Included once at translation-unit scope inside LightDataService.cpp;
// references LightDataServiceImpl (declared above), PointShadowConstantBuffer
// (GPUDataStructure.h), and Math::GeneratePerspectiveMatrix / Math::lookAt.

namespace
{
	// pos.w on PointLight_CB / SphereLight_CB carries the cube-shadow atlas
	// slot index (TASK-148). The shader recovers the uint via asuint(pos.w);
	// here we go the other way. Project is C++17 so std::bit_cast is unavailable
	// — memcpy is the canonical type-punning idiom (well-defined, no aliasing
	// violation, optimized to a register copy by every modern compiler).
	float SlotIndexAsFloat(uint32_t Slot)
	{
		float l_Result;
		std::memcpy(&l_Result, &Slot, sizeof(l_Result));
		return l_Result;
	}
}

bool LightDataServiceImpl::UpdatePointShadowData()
{
	m_PointShadowCBVector.clear();

	auto l_RenderingCapability = g_Engine->Get<RenderingConfigurationService>()->GetRenderingCapability();
	const uint32_t l_MaxPointShadows = l_RenderingCapability.maxPointShadows;

	auto& l_Storage = g_Engine->Get<EntityRegistry>()->Storage<LightComponent>();
	const auto& l_Lights = l_Storage.All();
	const auto& l_Owners = l_Storage.AllOwners();

	if (l_Lights.empty())
		return false;

	// Cube-face basis (right-handed, mirroring D3D TextureCube convention):
	// face 0 = +X, 1 = -X, 2 = +Y, 3 = -Y, 4 = +Z, 5 = -Z.
	// `up` for ±Y faces is ±Z to avoid the look-direction collision.
	// lookAt's Z axis points eyePos -> centerPos's negation, so target = pos + dir.
	const Vec4 l_FaceForward[6] = {
		Vec4( 1.0f,  0.0f,  0.0f, 0.0f), // +X
		Vec4(-1.0f,  0.0f,  0.0f, 0.0f), // -X
		Vec4( 0.0f,  1.0f,  0.0f, 0.0f), // +Y
		Vec4( 0.0f, -1.0f,  0.0f, 0.0f), // -Y
		Vec4( 0.0f,  0.0f,  1.0f, 0.0f), // +Z
		Vec4( 0.0f,  0.0f, -1.0f, 0.0f), // -Z
	};
	const Vec4 l_FaceUp[6] = {
		Vec4(0.0f, 1.0f, 0.0f, 0.0f),
		Vec4(0.0f, 1.0f, 0.0f, 0.0f),
		Vec4(0.0f, 0.0f, 1.0f, 0.0f),
		Vec4(0.0f, 0.0f, -1.0f, 0.0f),
		Vec4(0.0f, 1.0f, 0.0f, 0.0f),
		Vec4(0.0f, 1.0f, 0.0f, 0.0f),
	};

	// 90° FOV per face is the cube-shadow invariant; near plane needs to clear
	// self-shadowing at the light, far plane is the light's range/attenuation
	// radius (m_Shape.x for both Point and Sphere; see LightComponent comment).
	const float l_HalfPi    = 3.14159265358979323846f * 0.5f;
	const float l_NearPlane = 0.1f;

	uint32_t l_NextSlot          = 0;
	uint32_t l_PointParallelIdx  = 0;
	uint32_t l_SphereParallelIdx = 0;

	for (size_t i = 0; i < l_Lights.size(); i++)
	{
		const LightComponent& l_Light = l_Lights[i];
		const bool l_IsPoint  = (l_Light.m_LightType == LightType::Point);
		const bool l_IsSphere = (l_Light.m_LightType == LightType::Sphere);
		if (!l_IsPoint && !l_IsSphere)
			continue;

		// Track parallel-index into m_PointLightAtlasSlot / m_SphereLightAtlasSlot
		// so the per-light slot sidecar (TASK-149 contract) and the dense
		// PointShadowConstantBuffer agree on which light owns which slot.
		uint32_t l_ParallelIdx = l_IsPoint ? l_PointParallelIdx++ : l_SphereParallelIdx++;

		if (!l_Light.m_CastShadow)
			continue;

		if (l_NextSlot >= l_MaxPointShadows)
		{
			Log(Warning, "LightDataService: shadow-casting light at storage index ", i,
				" exceeded maxPointShadows budget (", l_MaxPointShadows,
				"); skipping shadow allocation. Remains lit but unshadowed.");
			continue;
		}

		EntityID l_EntityID = l_Owners[i];
		auto* l_Transform = g_Engine->Get<EntityRegistry>()->Get<TransformComponent>(l_EntityID);
		if (!l_Transform)
		{
			Log(Warning, "LightDataService: shadow-casting light at storage index ", i,
				" has no TransformComponent; skipping shadow allocation.");
			continue;
		}

		// Cube-shadow zFar / attenuation range:
		//   Point  → m_Shape.x is the author-set attenuation radius (per
		//            LightComponent.h schema). Use directly.
		//   Sphere → m_Shape.x is the *physical sphere radius*, not an
		//            attenuation distance. Using it as zFar would clip nearly
		//            everything beyond the sphere itself. Compute attenuation
		//            radius from luminous flux (point-source inverse-square
		//            falloff to a perceptual cutoff of 0.01 cd/m²):
		//                d = sqrt(φ / (4π · L_min))
		//            Same idiom as `EvaluateTiledPointLighting` uses internally
		//            for sphere lights. Keeps the LightComponent schema
		//            untouched (TASK-149 ECS POD invariant) and matches the
		//            light-attenuation falloff the lighting evaluator already
		//            applies.
		Vec4 l_LightPos = l_Transform->m_LocalPos;
		float l_Range   = l_Light.m_Shape.x;
		if (l_IsSphere)
		{
			const float l_LuminanceCutoff   = 0.01f; // cd/m², perceptual
			const float l_FourPi            = 4.0f * 3.14159265358979323846f;
			const float l_AttenuationRadius = std::sqrt(
				std::max(l_Light.m_LuminousFlux, 0.0f)
				/ (l_FourPi * l_LuminanceCutoff));
			l_Range = l_AttenuationRadius;
		}
		if (l_Range <= l_NearPlane)
		{
			Log(Warning, "LightDataService: shadow-casting light at storage index ", i,
				" has range (", l_Range, ") <= near plane (", l_NearPlane,
				"); skipping shadow allocation.");
			continue;
		}

		PointShadowConstantBuffer l_CB = {};
		l_CB.p = Math::GeneratePerspectiveMatrix<float>(l_HalfPi, 1.0f, l_NearPlane, l_Range);
		for (uint32_t f = 0; f < 6; f++)
		{
			const Vec4 l_Center = l_LightPos + l_FaceForward[f];
			l_CB.v[f] = Math::lookAt<float>(l_LightPos, l_Center, l_FaceUp[f]);
		}
		l_CB.lightPosWS_range = Vec4(l_LightPos.x, l_LightPos.y, l_LightPos.z, l_Range);
		l_CB.atlasBaseSlot    = l_NextSlot * 6;
		l_CB.isActive         = 1;

		m_PointShadowCBVector.emplace_back(l_CB);

		// Stamp the parallel-indexed sidecar so LightPass can correlate
		// PointLight_CB[k] with PointShadowConstantBuffer[slot]. Also stamp
		// the slot into PointLight_CB::pos.w / SphereLight_CB::pos.w so the
		// resolver shader can recover the slot from the cbuffer entry the
		// LightCullingPass already binds (no separate slot-sidecar SRV).
		const float l_SlotAsFloat = SlotIndexAsFloat(l_NextSlot);
		if (l_IsPoint && l_ParallelIdx < m_PointLightAtlasSlot.size())
		{
			m_PointLightAtlasSlot[l_ParallelIdx] = l_NextSlot;
			if (l_ParallelIdx < m_PointLightCBVector.size())
				m_PointLightCBVector[l_ParallelIdx].pos.w = l_SlotAsFloat;
		}
		else if (l_IsSphere && l_ParallelIdx < m_SphereLightAtlasSlot.size())
		{
			m_SphereLightAtlasSlot[l_ParallelIdx] = l_NextSlot;
			if (l_ParallelIdx < m_SphereLightCBVector.size())
				m_SphereLightCBVector[l_ParallelIdx].pos.w = l_SlotAsFloat;
		}

		l_NextSlot++;
	}

	return true;
}
