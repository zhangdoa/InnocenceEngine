#pragma once
#include "GPUUploadable.h"
#include "GraphicsPrimitive.h"
#include "MathHelper.h"

namespace Inno
{
	static constexpr uint32_t INVALID_TEXTURE_INDEX = 0xFFFFFFFF;

	class MeshComponent;
	class TextureComponent;

	enum class VisibilityMask
	{
		Invalid = 0, MainCamera = 1, Sun = 2
	};

	INNO_ENUM_OPERATORS(VisibilityMask)

	enum class DebugViewMode : uint32_t
	{
		None                    = 0u,
		DirectLightingOnly      = 1u,
		IndirectLightingOnly    = 2u,
		SunShadowVisibility     = 3u,
		TileLightCountHeatmap   = 4u,
	};

	struct alignas(16) PerFrameConstantBuffer : GPUUploadable<PerFrameConstantBuffer>
	{
		Mat4 p_original;
		Mat4 p_jittered;
		Mat4 v;
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
		uint32_t debugViewMode;
		Vec2 radianceCacheHaltonJitter;
		uint32_t frameIndex;
		uint32_t modelCount;
		uint32_t exposureMode; // 0 = Manual (aperture/shutter/ISO), 1 = Auto (K + log-luminance)
		float autoExposureKey;
		float autoExposureCompensation; // EV stops applied on top of auto-exposure result
		float exposurePadding;
		uint32_t pointShadowBypass; // 1 forces inline-RT visibility=1 in EvaluateTiledPointLighting (A/B toggle).
		uint32_t padding[11];

		// padding is std140 cbuffer trailing alignment, not a real field.
		// The producer cannot write it; the HLSL side reads these 44 bytes
		// as zero. Exclude it from the validator's scan.
		static constexpr std::array<std::pair<size_t, size_t>, 4> SkipByteRanges() noexcept
		{
			std::array<std::pair<size_t, size_t>, 4> r{};
			r[0] = { offsetof(PerFrameConstantBuffer, padding), sizeof(padding) };
			return r;
		}
};

// EBO invariant: deriving GPUUploadable<PerFrameConstantBuffer> must not grow
	// the struct or shift any field. The std140 HLSL layout depends on this.
	static_assert(sizeof(PerFrameConstantBuffer) == 512,
		"PerFrameConstantBuffer size changed after CRTP base; std140 layout broken.");
	static_assert(alignof(PerFrameConstantBuffer) == 16,
		"PerFrameConstantBuffer alignment changed after CRTP base; std140 layout broken.");
	static_assert(std::is_standard_layout_v<PerFrameConstantBuffer>,
		"PerFrameConstantBuffer must be standard-layout so offsetof and "
		"raw byte upload are well-defined.");

// w component of luminance is attenuationRadius.
// m_CastShadow + padding[3] keep the struct 16-byte-aligned for HLSL std140
// cbuffer-array element alignment.
struct alignas(16) PointLightConstantBuffer : GPUUploadable<PointLightConstantBuffer>
{
	Vec4 pos;
	Vec4 luminance;
	uint32_t m_CastShadow;
	uint32_t padding[3];

	// padding is std140 cbuffer trailing alignment, not a real field.
	static constexpr std::array<std::pair<size_t, size_t>, 4> SkipByteRanges() noexcept
	{
		std::array<std::pair<size_t, size_t>, 4> r{};
		r[0] = { offsetof(PointLightConstantBuffer, padding), sizeof(padding) };
		return r;
	}
};
static_assert(sizeof(PointLightConstantBuffer) == 48,
	"PointLightConstantBuffer size changed after CRTP base; std140 layout broken.");
static_assert(alignof(PointLightConstantBuffer) == 16,
	"PointLightConstantBuffer alignment changed after CRTP base; std140 layout broken.");
static_assert(std::is_standard_layout_v<PointLightConstantBuffer>,
	"PointLightConstantBuffer must be standard-layout so offsetof and "
	"raw byte upload are well-defined.");

// w component of luminance is sphereRadius.
struct alignas(16) SphereLightConstantBuffer : GPUUploadable<SphereLightConstantBuffer>
{
	Vec4 pos;
	Vec4 luminance;
};
static_assert(sizeof(SphereLightConstantBuffer) == 32,
	"SphereLightConstantBuffer size changed after CRTP base; std140 layout broken.");
static_assert(alignof(SphereLightConstantBuffer) == 16,
	"SphereLightConstantBuffer alignment changed after CRTP base; std140 layout broken.");
static_assert(std::is_standard_layout_v<SphereLightConstantBuffer>,
	"SphereLightConstantBuffer must be standard-layout so offsetof and "
	"raw byte upload are well-defined.");

struct alignas(16) TransformConstantBuffer : GPUUploadable<TransformConstantBuffer>
{
	Mat4 m;
	Mat4 normalMat;
};
static_assert(sizeof(TransformConstantBuffer) == 128,
	"TransformConstantBuffer size changed after CRTP base; std140 layout broken.");
static_assert(alignof(TransformConstantBuffer) == 16,
	"TransformConstantBuffer alignment changed after CRTP base; std140 layout broken.");
static_assert(std::is_standard_layout_v<TransformConstantBuffer>,
	"TransformConstantBuffer must be standard-layout so offsetof and "
	"raw byte upload are well-defined.");

	enum class ShaderModel { Invalid, Opaque, Transparent, Emissive, Volumetric, Debug };

	struct MaterialAttributes
	{
		float AlbedoR;
		float AlbedoG;
		float AlbedoB;
		float Alpha;
		float Metallic;
		float Roughness;
		float AO;
		float Thickness;
	};

	const uint32_t MaxTextureSlotCount = 7;
	struct alignas(16) MaterialConstantBuffer : GPUUploadable<MaterialConstantBuffer>
	{
		MaterialAttributes m_MaterialAttributes;
		uint32_t m_TextureIndices[MaxTextureSlotCount];
		uint32_t m_MaterialType;
	};
	static_assert(sizeof(MaterialConstantBuffer) == 64,
		"MaterialConstantBuffer size changed after CRTP base; std140 layout broken.");
	static_assert(alignof(MaterialConstantBuffer) == 16,
		"MaterialConstantBuffer alignment changed after CRTP base; std140 layout broken.");
	static_assert(std::is_standard_layout_v<MaterialConstantBuffer>,
		"MaterialConstantBuffer must be standard-layout so offsetof and "
		"raw byte upload are well-defined.");

struct alignas(16) DispatchParamsConstantBuffer : GPUUploadable<DispatchParamsConstantBuffer>
{
	TVec4<uint32_t> numThreadGroups;
	TVec4<uint32_t> numThreads;
};
static_assert(sizeof(DispatchParamsConstantBuffer) == 32,
	"DispatchParamsConstantBuffer size changed after CRTP base; std140 layout broken.");
static_assert(alignof(DispatchParamsConstantBuffer) == 16,
	"DispatchParamsConstantBuffer alignment changed after CRTP base; std140 layout broken.");
static_assert(std::is_standard_layout_v<DispatchParamsConstantBuffer>,
	"DispatchParamsConstantBuffer must be standard-layout so offsetof and "
	"raw byte upload are well-defined.");

struct alignas(16) GIConstantBuffer : GPUUploadable<GIConstantBuffer>
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
static_assert(sizeof(GIConstantBuffer) == 1024,
	"GIConstantBuffer size changed after CRTP base; std140 layout broken.");
static_assert(alignof(GIConstantBuffer) == 16,
	"GIConstantBuffer alignment changed after CRTP base; std140 layout broken.");
static_assert(std::is_standard_layout_v<GIConstantBuffer>,
	"GIConstantBuffer must be standard-layout so offsetof and "
	"raw byte upload are well-defined.");

struct alignas(16) VoxelizationConstantBuffer : GPUUploadable<VoxelizationConstantBuffer>
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

		// padding is std140 cbuffer trailing alignment, not a real field.
		static constexpr std::array<std::pair<size_t, size_t>, 4> SkipByteRanges() noexcept
		{
			std::array<std::pair<size_t, size_t>, 4> r{};
			r[0] = { offsetof(VoxelizationConstantBuffer, padding), sizeof(padding) };
			return r;
		}
	};
static_assert(sizeof(VoxelizationConstantBuffer) == 64,
	"VoxelizationConstantBuffer size changed after CRTP base; std140 layout broken.");
static_assert(alignof(VoxelizationConstantBuffer) == 16,
	"VoxelizationConstantBuffer alignment changed after CRTP base; std140 layout broken.");
static_assert(std::is_standard_layout_v<VoxelizationConstantBuffer>,
	"VoxelizationConstantBuffer must be standard-layout so offsetof and "
	"raw byte upload are well-defined.");

struct alignas(16) AnimationConstantBuffer : GPUUploadable<AnimationConstantBuffer>
	{
		Mat4 rootOffsetMatrix;
		float duration;
		uint32_t numChannels;
		uint32_t numTicks;
		float currentTime;
		float padding[44];

		// padding is std140 cbuffer trailing alignment, not a real field.
		static constexpr std::array<std::pair<size_t, size_t>, 4> SkipByteRanges() noexcept
		{
			std::array<std::pair<size_t, size_t>, 4> r{};
			r[0] = { offsetof(AnimationConstantBuffer, padding), sizeof(padding) };
			return r;
		}
	};
static_assert(sizeof(AnimationConstantBuffer) == 256,
	"AnimationConstantBuffer size changed after CRTP base; std140 layout broken.");
static_assert(alignof(AnimationConstantBuffer) == 16,
	"AnimationConstantBuffer alignment changed after CRTP base; std140 layout broken.");
static_assert(std::is_standard_layout_v<AnimationConstantBuffer>,
	"AnimationConstantBuffer must be standard-layout so offsetof and "
	"raw byte upload are well-defined.");

	struct alignas(16) CollisionPrimitives
	{
		AABB m_AABB = {};
		Sphere m_Sphere = {};
	};

	struct DrawCallInfo
	{
		MeshComponent* mesh = 0;
		uint32_t m_PerObjectConstantBufferIndex = 0;
		VisibilityMask m_VisibilityMask = VisibilityMask::Invalid;
		MeshUsage meshUsage = MeshUsage::Invalid;
	};

struct alignas(16) GPUModelData : GPUUploadable<GPUModelData>
	{
		uint64_t m_VertexBufferAddress = 0;
		uint64_t m_IndexBufferAddress = 0;

		uint32_t m_VertexCount = 0;
		uint32_t m_IndexCount = 0;
		uint32_t m_VertexStride = 0;
		uint32_t m_IndexStride = 0;

		uint32_t m_MaterialIndex = 0;
		uint32_t m_ShaderProgramIndex = 0;
		float m_UUID = 0.0f;
		uint32_t m_RenderPassIndex = 0;

		uint32_t m_VisibilityMask = 0;
		uint32_t m_MeshUsage = 0;

		Vec4 m_BoundingBoxMin;
		Vec4 m_BoundingBoxMax;

		uint32_t m_InstanceCount = 1;
		uint32_t m_FirstInstance = 0;

		float padding[16];

		// Explicit padding[16] is std140 cbuffer trailing alignment,
		// not a real field. The producer cannot write it; the HLSL
		// shader reads it as zero.
		static constexpr std::array<std::pair<size_t, size_t>, 4> SkipByteRanges() noexcept
		{
			std::array<std::pair<size_t, size_t>, 4> r{};
			r[0] = { offsetof(GPUModelData, padding), sizeof(padding) };
			return r;
		}
	};
static_assert(sizeof(GPUModelData) == 160,
	"GPUModelData size changed after CRTP base; std140 layout broken.");
static_assert(alignof(GPUModelData) == 16,
	"GPUModelData alignment changed after CRTP base; std140 layout broken.");
static_assert(std::is_standard_layout_v<GPUModelData>,
	"GPUModelData must be standard-layout so offsetof and "
	"raw byte upload are well-defined.");

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

}