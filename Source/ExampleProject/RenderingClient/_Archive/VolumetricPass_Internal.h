#pragma once
// Internal header for the partial-TU split of VolumetricPass. The pass is a
// namespace (not a class), so this header declares the namespace-scope state
// as extern and forward-declares the per-pass helpers that live in the
// sibling _Setup.cpp / _ExecuteCommands.cpp TUs.
//
// Per `disciplines/on-implement/file-splitting.md`, the pass-class typical
// pattern (same class, different responsibility cluster) applies here with
// the namespace acting as the class.

#include "VolumetricPass.h"

#include "../../Engine/Common/MathHelper.h"
#include "../../Engine/Component/SamplerComponent.h"
#include "../../Engine/Component/RenderPassComponent.h"
#include "../../Engine/Component/ShaderProgramComponent.h"
#include "../../Engine/Component/TextureComponent.h"
#include "../../Engine/Component/CommandListComponent.h"

namespace VolumetricPass
{
	bool setupGeometryProcessPass();
	bool setupIrradianceInjectionPass();
	bool setupRayMarchingPass();
	bool setupVisualizationPass();

	bool froxelization();
	bool irraidanceInjection();
	bool rayMarching();
	bool visualization(Inno::GPUResourceComponent* input);

	extern Inno::SamplerComponent* m_SamplerComp;

	extern Inno::RenderPassComponent* m_froxelizationRenderPassComp;
	extern Inno::ShaderProgramComponent* m_froxelizationSPC;

	extern Inno::RenderPassComponent* m_visualizationRenderPassComp;
	extern Inno::ShaderProgramComponent* m_visualizationSPC;

	extern Inno::RenderPassComponent* m_irraidanceInjectionRenderPassComp;
	extern Inno::ShaderProgramComponent* m_irraidanceInjectionSPC;

	extern Inno::RenderPassComponent* m_rayMarchingRenderPassComp;
	extern Inno::ShaderProgramComponent* m_rayMarchingSPC;

	extern Inno::TextureComponent* m_irraidanceInjectionResult;
	extern Inno::TextureComponent* m_rayMarchingResult_A;
	extern Inno::TextureComponent* m_rayMarchingResult_B;

	extern Inno::CommandListComponent* m_froxelizationCommandListComp;
	extern Inno::CommandListComponent* m_irraidanceInjectionCommandListComp;
	extern Inno::CommandListComponent* m_rayMarchingCommandListComp;
	extern Inno::CommandListComponent* m_visualizationCommandListComp;

	extern TVec4<uint32_t> m_voxelizationResolution;
	extern bool m_isPassA;
}
