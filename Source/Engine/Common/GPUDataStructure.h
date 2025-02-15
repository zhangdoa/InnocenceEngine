#pragma once
#include "GraphicsPrimitive.h"
#include "MathHelper.h"

namespace Inno
{
	class MeshComponent;
	class TextureComponent;

	enum class VisibilityMask
	{
		Invalid = 0, MainCamera = 1, Sun = 2
	};

	INNO_ENUM_OPERATORS(VisibilityMask)

	struct alignas(16) PerFrameConstantBuffer
	{
		Mat4 p_original;
		Mat4 p_jittered;
		Mat4 v;
		Mat4 v_prev;
		Mat4 p_inv;
		Mat4 v_inv;
		float zNear;
		float zFar;
		float minLogLuminance;
		float maxLogLuminance;
		Vec4 sun_direction;
		Vec4 sun_illuminance;
		Vec4 viewportSize;
		Vec4 posWSNormalizer;
		Vec4 camera_posWS;
		float aperture;
		float shutterTime;
		float ISO;
		uint32_t activeCascade;
		Vec2 radianceCacheHaltonJitter;
		uint32_t frameIndex;
		float padding;
	};

	struct alignas(16) CSMConstantBuffer
	{
		Mat4 p;
		Mat4 v;
		Vec4 AABBMax;
		Vec4 AABBMin;
		float padding[24];
	};

	// w component of luminance is attenuationRadius
	struct alignas(16) PointLightConstantBuffer
	{
		Vec4 pos;
		Vec4 luminance;
		//float attenuationRadius;
	};

	// w component of luminance is sphereRadius
	struct alignas(16) SphereLightConstantBuffer
	{
		Vec4 pos;
		Vec4 luminance;
		//float sphereRadius;
	};

	struct alignas(16) PerObjectConstantBuffer
	{
		Mat4 m;
		Mat4 m_prev;
		Mat4 normalMat;
		float UUID;
		uint32_t m_MaterialIndex = 0;
		float padding[14];
	};

	struct MaterialAttributes
	{
		float AlbedoR = 1.0f;
		float AlbedoG = 1.0f;
		float AlbedoB = 1.0f;
		float Alpha = 1.0f;
		float Metallic = 0.0f;
		float Roughness = 1.0f;
		float AO = 0.0f;
		float Thickness = 1.0f;
	};

	const uint32_t MaxTextureSlotCount = 7;
	struct alignas(16) MaterialConstantBuffer
	{
		MaterialAttributes m_MaterialAttributes;
		int32_t m_TextureIndices[MaxTextureSlotCount] = { -1, -1, -1, -1, -1, -1, -1 };
		uint32_t m_MaterialType = 0;
	};

	struct alignas(16) DispatchParamsConstantBuffer
	{
		TVec4<uint32_t> numThreadGroups;
		TVec4<uint32_t> numThreads;
	};

	struct alignas(16) GIConstantBuffer
	{
		Mat4 p;
		Mat4 r[6];
		Mat4 t;
		Mat4 p_inv;
		Mat4 v_inv[6];
		Vec4 probeCount;
		Vec4 probeRange;
		Vec4 workload;
		Vec4 irradianceVolumeOffset;
	};

	struct alignas(16) VoxelizationConstantBuffer
	{
		Vec4 volumeCenter;
		float volumeExtend;
		float volumeExtendRcp;
		float volumeResolution;
		float volumeResolutionRcp;
		float voxelSize;
		float voxelSizeRcp;
		float numCones;
		float numConesRcp;
		float coneTracingStep;
		float coneTracingMaxDistance;
		float padding[2];
	};

	struct alignas(16) AnimationConstantBuffer
	{
		Mat4 rootOffsetMatrix;
		float duration;
		uint32_t numChannels;
		uint32_t numTicks;
		float currentTime;
		float padding[44];
	};

	struct DrawCallInfo
	{
		MeshComponent* mesh = 0;
		uint32_t m_PerObjectConstantBufferIndex = 0;
		VisibilityMask m_VisibilityMask = VisibilityMask::Invalid;
		MeshUsage meshUsage = MeshUsage::Invalid;
	};

	struct BillboardPassDrawCallInfo
	{
		TextureComponent* iconTexture;
		uint32_t meshConstantBufferOffset;
		uint32_t instanceCount;
	};

	struct DebugPassDrawCallInfo
	{
		MeshComponent* mesh;
	};

	// Sample point on geometry surface
	struct Surfel
	{
		Vec4 pos;
		Vec4 normal;
		Vec4 albedo;
		Vec4 MRAT;

		bool operator==(const Surfel& other) const
		{
			return (pos == other.pos);
		}
	};

	// 1x1x1 m^3 of surfels
	using SurfelGrid = Surfel;

	// 4x4x4 m^3 of surfels
	struct Brick
	{
		AABB boundBox;
		uint32_t surfelRangeBegin;
		uint32_t surfelRangeEnd;

		bool operator==(const Brick& other) const
		{
			return (boundBox.m_center == other.boundBox.m_center);
		}
	};

	struct BrickFactor
	{
		float basisWeight;
		uint32_t brickIndex;

		bool operator==(const BrickFactor& other) const
		{
			return (brickIndex == other.brickIndex);
		}
	};

	struct Probe
	{
		Vec4 pos;
		uint32_t brickFactorRange[12];
		float skyVisibility[6];
		uint32_t padding[10];
	};

	struct ProbeInfo
	{
		Vec4 probeCount;
		Vec4 probeRange;
	};
}