#include "ProbeGenerator.h"

#include "../../Engine/Common/MathHelper.h"
#include "../../Engine/Services/RenderingConfigurationService.h"

#include "../../Engine/Engine.h"

using namespace Inno;


#include "../../Engine/Common/IOService.h"
#include "Baker.h"
#include "Serializer.h"



namespace Inno
{
    namespace Baker
    {
        void ProbeGenerator::setup()
        {
            auto l_renderingServer = g_Engine->getRenderingServer();

            auto l_RenderingCapability = g_Engine->Get<RenderingConfigurationService>()->GetRenderingCapability();

            Config::Get().m_staticMeshDrawCallInfo.reserve(l_RenderingCapability.maxMeshes);
            Config::Get().m_staticMeshPerObjectConstantBuffer.reserve(l_RenderingCapability.maxMeshes);
            Config::Get().m_staticMeshMaterialConstantBuffer.reserve(l_RenderingCapability.maxMaterials);

            auto l_RenderPassDesc = g_Engine->Get<RenderingConfigurationService>()->GetDefaultRenderPassDesc();
            l_RenderPassDesc.m_UseDepthBuffer = true;
            l_RenderPassDesc.m_UseStencilBuffer = true;

            m_SPC_Probe = l_renderingServer->AddShaderProgramComponent("GIBakeProbePass/");

            m_SPC_Probe->m_ShaderFilePaths.m_VSPath = "GIBakeProbePass.vert/";
            m_SPC_Probe->m_ShaderFilePaths.m_PSPath = "GIBakeProbePass.frag/";

            l_renderingServer->Initialize(m_SPC_Probe);

            m_RenderPassComp_Probe = l_renderingServer->AddRenderPassComponent("GIBakeProbePass/");

            m_RenderPassComp_Probe->m_RenderPassDesc = l_RenderPassDesc;
            m_RenderPassComp_Probe->m_RenderPassDesc.m_RenderTargetDesc.Sampler = TextureSampler::Sampler2D;
            m_RenderPassComp_Probe->m_RenderPassDesc.m_RenderTargetDesc.Width = Config::Get().m_probeMapResolution;
            m_RenderPassComp_Probe->m_RenderPassDesc.m_RenderTargetDesc.Height = Config::Get().m_probeMapResolution;
            m_RenderPassComp_Probe->m_RenderPassDesc.m_RenderTargetDesc.PixelDataType = TexturePixelDataType::Float32;

            m_RenderPassComp_Probe->m_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_DepthEnable = true;
            m_RenderPassComp_Probe->m_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_AllowDepthWrite = true;
            m_RenderPassComp_Probe->m_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_DepthComparisionFunction = ComparisionFunction::LessEqual;
            m_RenderPassComp_Probe->m_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_StencilEnable = true;
            m_RenderPassComp_Probe->m_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_AllowStencilWrite = true;
            m_RenderPassComp_Probe->m_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_FrontFaceStencilComparisionFunction = ComparisionFunction::Equal;

            m_RenderPassComp_Probe->m_RenderPassDesc.m_GraphicsPipelineDesc.m_RasterizerDesc.m_UseCulling = true;
            m_RenderPassComp_Probe->m_RenderPassDesc.m_GraphicsPipelineDesc.m_ViewportDesc.m_Width = (float)Config::Get().m_probeMapResolution;
            m_RenderPassComp_Probe->m_RenderPassDesc.m_GraphicsPipelineDesc.m_ViewportDesc.m_Height = (float)Config::Get().m_probeMapResolution;

            m_RenderPassComp_Probe->m_ResourceBindingLayoutDescs.resize(2);
            m_RenderPassComp_Probe->m_ResourceBindingLayoutDescs[0].m_GPUResourceType = GPUResourceType::Buffer;
            m_RenderPassComp_Probe->m_ResourceBindingLayoutDescs[0].m_DescriptorSetIndex = 0;
            m_RenderPassComp_Probe->m_ResourceBindingLayoutDescs[0].m_DescriptorIndex = 8;

            m_RenderPassComp_Probe->m_ResourceBindingLayoutDescs[1].m_GPUResourceType = GPUResourceType::Buffer;
            m_RenderPassComp_Probe->m_ResourceBindingLayoutDescs[1].m_DescriptorSetIndex = 0;
            m_RenderPassComp_Probe->m_ResourceBindingLayoutDescs[1].m_DescriptorIndex = 1;

            m_RenderPassComp_Probe->m_ShaderProgram = m_SPC_Probe;

            l_renderingServer->Initialize(m_RenderPassComp_Probe);
        }

        bool ProbeGenerator::gatherStaticMeshData()
        {
            auto l_renderingServer = g_Engine->getRenderingServer();
            
            Log(Success, "Gathering static meshes...");

            uint32_t l_index = 0;

            auto l_modelComponents = g_Engine->Get<ComponentManager>()->GetAll<ModelComponent>();
            for (auto modelComponent : l_modelComponents)
            {
                if (modelComponent->m_ObjectStatus == ObjectStatus::Activated && modelComponent->m_meshUsage == MeshUsage::Static)
                {
                    auto l_globalTm = Math::toTransformationMatrix(modelComponent->m_Transform);

                    for (uint64_t j = 0; j < modelComponent->m_Model->renderableSets.m_count; j++)
                    {
                        auto l_renderableSet = g_Engine->Get<AssetService>()->GetRenderableSet(modelComponent->m_Model->renderableSets.m_startOffset + j);

                        if (l_renderableSet->material->m_ShaderModel == ShaderModel::Opaque)
                        {
                            DrawCallInfo l_staticPerObjectConstantBuffer;

                            l_staticPerObjectConstantBuffer.mesh = l_renderableSet->mesh;
                            l_staticPerObjectConstantBuffer.material = l_renderableSet->material;

                            PerObjectConstantBuffer l_meshConstantBuffer;

                            l_meshConstantBuffer.m = Math::toTransformationMatrix(modelComponent->m_Transform);
                            l_meshConstantBuffer.m_prev = l_meshConstantBuffer.m; // @TODO: Need previous frame transform
                            l_meshConstantBuffer.normalMat = Math::toRotationMatrix(modelComponent->m_Transform.m_rot);
                            l_meshConstantBuffer.UUID = (float)modelComponent->m_UUID;

                            MaterialConstantBuffer l_materialConstantBuffer;

                            for (size_t i = 0; i < 8; i++)
                            {
                                uint32_t l_writeMask = l_renderableSet->material->m_TextureSlots[i].m_Activate ? 0x00000001 : 0x00000000;
                                l_writeMask = l_writeMask << i;
                                l_materialConstantBuffer.textureSlotMask |= l_writeMask;
                            }

                            l_materialConstantBuffer.materialAttributes = l_renderableSet->material->m_materialAttributes;

                            Config::Get().m_staticMeshDrawCallInfo.emplace_back(l_staticPerObjectConstantBuffer);
                            Config::Get().m_staticMeshPerObjectConstantBuffer.emplace_back(l_meshConstantBuffer);
                            Config::Get().m_staticMeshMaterialConstantBuffer.emplace_back(l_materialConstantBuffer);
                            l_index++;
                        }
                    }
                }
            }

            Config::Get().m_staticMeshDrawCallCount = l_index;

            Log(Success, "There are ", Config::Get().m_staticMeshDrawCallCount, " static meshes in current scene.");

            auto l_MeshGPUBufferComp = g_Engine->Get<RenderingContextService>()->GetGPUBufferComponent(GPUBufferUsageType::Transform);
            auto l_MaterialGPUBufferComp = g_Engine->Get<RenderingContextService>()->GetGPUBufferComponent(GPUBufferUsageType::Material);

            l_renderingServer->Upload(l_MeshGPUBufferComp, Config::Get().m_staticMeshPerObjectConstantBuffer, 0, Config::Get().m_staticMeshPerObjectConstantBuffer.size());
            l_renderingServer->Upload(l_MaterialGPUBufferComp, Config::Get().m_staticMeshMaterialConstantBuffer, 0, Config::Get().m_staticMeshMaterialConstantBuffer.size());

            return true;
        }

        bool ProbeGenerator::generateProbeCaches(std::vector<Probe>& probes)
        {
            auto l_renderingServer = g_Engine->getRenderingServer();

            Log(Success, "Generate probe caches...");

            auto l_sceneAABB = g_Engine->Get<PhysicsSimulationService>()->getStaticSceneAABB();

            auto l_eyePos = l_sceneAABB.m_center;
            auto l_extendedAxisSize = l_sceneAABB.m_extend;
            l_eyePos.y += l_extendedAxisSize.y / 2.0f;

            // Add a bit offset
            l_eyePos.y += 1.0f;

            auto l_p = Math::generateOrthographicMatrix(-l_extendedAxisSize.x / 2.0f, l_extendedAxisSize.x / 2.0f, -l_extendedAxisSize.z / 2.0f, l_extendedAxisSize.z / 2.0f, -l_extendedAxisSize.y / 2.0f, l_extendedAxisSize.y / 2.0f);

            std::vector<Mat4> l_GICameraConstantBuffer(8);
            l_GICameraConstantBuffer[0] = l_p;
            l_GICameraConstantBuffer[1] = Math::lookAt(Vec4(0.0f, 0.0f, 0.0f, 1.0f), Vec4(0.0f, -1.0f, 0.0f, 1.0f), Vec4(0.0f, 0.0f, 1.0f, 0.0f));
            l_GICameraConstantBuffer[7] = Math::getInvertTranslationMatrix(l_eyePos);

            l_renderingServer->Upload(g_Engine->Get<RenderingContextService>()->GetGPUBufferComponent(GPUBufferUsageType::GI), l_GICameraConstantBuffer);

            Log(Success, "Start to draw probe height map...");

            auto l_MeshGPUBufferComp = g_Engine->Get<RenderingContextService>()->GetGPUBufferComponent(GPUBufferUsageType::Transform);

            l_renderingServer->CommandListBegin(m_RenderPassComp_Probe, 0);
            l_renderingServer->BindRenderPassComponent(m_RenderPassComp_Probe);
            l_renderingServer->ClearRenderTargets(m_RenderPassComp_Probe);
            l_renderingServer->BindGPUResource(m_RenderPassComp_Probe, ShaderStage::Vertex, g_Engine->Get<RenderingContextService>()->GetGPUBufferComponent(GPUBufferUsageType::GI), 0);

            uint32_t l_offset = 0;

            for (uint32_t i = 0; i < Config::Get().m_staticMeshDrawCallCount; i++)
            {
                auto l_staticPerObjectConstantBuffer = Config::Get().m_staticMeshDrawCallInfo[i];

                if (l_staticPerObjectConstantBuffer.mesh->m_ObjectStatus == ObjectStatus::Activated)
                {
                    l_renderingServer->BindGPUResource(m_RenderPassComp_Probe, ShaderStage::Vertex, l_MeshGPUBufferComp, 1, l_offset, 1);

                    l_renderingServer->DrawIndexedInstanced(m_RenderPassComp_Probe, l_staticPerObjectConstantBuffer.mesh);
                }

                l_offset++;
            }

            l_renderingServer->CommandListEnd(m_RenderPassComp_Probe);

            l_renderingServer->Execute(m_RenderPassComp_Probe, GPUEngineType::Graphics);
            l_renderingServer->WaitOnGPU(m_RenderPassComp_Probe, GPUEngineType::Graphics, GPUEngineType::Graphics);
            l_renderingServer->WaitOnCPU(GPUEngineType::Graphics);

            Log(Success, "Start to generate probe location...");

            // Read back results and generate probes
            auto l_probePosTextureResults = l_renderingServer->ReadTextureBackToCPU(m_RenderPassComp_Probe, m_RenderPassComp_Probe->m_RenderTargets[0].m_Texture);

            //#ifdef DEBUG_
            auto l_TextureComp = l_renderingServer->AddTextureComponent();
            l_TextureComp->m_TextureDesc = m_RenderPassComp_Probe->m_RenderTargets[0].m_Texture->m_TextureDesc;
            l_TextureComp->m_InitialData = l_probePosTextureResults.data();
            g_Engine->Get<AssetService>()->Save("..//Res//Intermediate//ProbePosTexture", l_TextureComp);
            //#endif // DEBUG_

            auto l_probeInfos = generateProbes(probes, l_probePosTextureResults, Config::Get().m_probeInterval);

            serializeProbeInfos(l_probeInfos);

            Log(Success, "", probes.size(), " probes generated.");

            return true;
        }

        ProbeInfo ProbeGenerator::generateProbes(std::vector<Probe>& probes, const std::vector<Vec4>& heightMap, uint32_t probeMapSamplingInterval)
        {
            probes.reserve(heightMap.size());

            generateProbesAlongTheSurface(probes, heightMap, probeMapSamplingInterval);
            auto l_maxVerticalProbesCount = generateProbesAlongTheWall(probes, heightMap, probeMapSamplingInterval);

            ProbeInfo l_result;

            auto l_minProbePos = Math::maxVec4<float>;
            auto l_maxProbePos = Math::minVec4<float>;

            auto l_probesCount = probes.size();

            for (size_t i = 0; i < l_probesCount; i++)
            {
                if (probes[i].pos.x < l_minProbePos.x)
                {
                    l_minProbePos.x = probes[i].pos.x;
                }
                if (probes[i].pos.x > l_maxProbePos.x)
                {
                    l_maxProbePos.x = probes[i].pos.x;
                }
                if (probes[i].pos.y < l_minProbePos.y)
                {
                    l_minProbePos.y = probes[i].pos.y;
                }
                if (probes[i].pos.y > l_maxProbePos.y)
                {
                    l_maxProbePos.y = probes[i].pos.y;
                }
                if (probes[i].pos.z < l_minProbePos.z)
                {
                    l_minProbePos.z = probes[i].pos.z;
                }
                if (probes[i].pos.z > l_maxProbePos.z)
                {
                    l_maxProbePos.z = probes[i].pos.z;
                }
            }

            l_result.probeRange.x = l_maxProbePos.x - l_minProbePos.x;
            l_result.probeRange.y = l_maxProbePos.y - l_minProbePos.y;
            l_result.probeRange.z = l_maxProbePos.z - l_minProbePos.z;
            l_result.probeRange.w = 1.0f;

            auto l_probesCountPerLine = Config::Get().m_probeMapResolution / probeMapSamplingInterval;

            l_result.probeCount.x = (float)l_probesCountPerLine;
            l_result.probeCount.y = (float)l_maxVerticalProbesCount;
            l_result.probeCount.z = (float)l_probesCountPerLine;
            l_result.probeCount.w = 1.0f;

            return l_result;
        }

        bool ProbeGenerator::generateProbesAlongTheSurface(std::vector<Probe>& probes, const std::vector<Vec4>& heightMap, uint32_t probeMapSamplingInterval)
        {
            auto l_probesCountPerLine = Config::Get().m_probeMapResolution / probeMapSamplingInterval;

            for (size_t i = 1; i < l_probesCountPerLine; i++)
            {
                for (size_t j = 1; j < l_probesCountPerLine; j++)
                {
                    auto l_currentIndex = i * probeMapSamplingInterval * Config::Get().m_probeMapResolution + j * probeMapSamplingInterval;
                    auto l_textureResult = heightMap[l_currentIndex];

                    Probe l_Probe;
                    l_Probe.pos = l_textureResult;

                    // Align the probe height over the surface
                    auto l_adjustedHeight = std::ceil(l_textureResult.y / Config::Get().m_probeHeightOffset);

                    // Edge case
                    if (l_textureResult.y == 0.0f)
                    {
                        l_adjustedHeight = Config::Get().m_probeHeightOffset;
                    }
                    else
                    {
                        l_adjustedHeight = l_adjustedHeight * Config::Get().m_probeHeightOffset;
                    }

                    l_Probe.pos.y = l_adjustedHeight;

                    probes.emplace_back(l_Probe);
                }
            }

            Log(Success, "", probes.size(), " probe location generated over the surface.");

            return true;
        }

        uint32_t ProbeGenerator::generateProbesAlongTheWall(std::vector<Probe>& probes, const std::vector<Vec4>& heightMap, uint32_t probeMapSamplingInterval)
        {
            std::vector<Probe> l_wallProbes;
            l_wallProbes.reserve(heightMap.size());

            uint32_t l_maxVerticalProbesCount = 1;

            auto l_probesCount = probes.size();
            auto l_probesCountPerLine = Config::Get().m_probeMapResolution / probeMapSamplingInterval;

            for (size_t i = 0; i < l_probesCount; i++)
            {
                // Not the last one in all, not any one in last column, and not the last one each row
                if ((i + 1 < l_probesCount) && ((i + l_probesCountPerLine) < l_probesCount) && ((i + 1) % l_probesCountPerLine))
                {
                    auto l_currentProbe = probes[i];
                    auto l_nextRowProbe = probes[i + 1];
                    auto l_nextColumnProbe = probes[i + l_probesCountPerLine];

                    // Eliminate sampling error
                    auto epsX = std::abs(l_currentProbe.pos.x - l_nextColumnProbe.pos.x);
                    auto epsZ = std::abs(l_currentProbe.pos.z - l_nextRowProbe.pos.z);

                    if (epsX)
                    {
                        l_nextColumnProbe.pos.x = l_currentProbe.pos.x;
                    }
                    if (epsZ)
                    {
                        l_nextRowProbe.pos.z = l_currentProbe.pos.z;
                    }

                    // Texture space partial derivatives
                    auto ddx = l_currentProbe.pos.y - l_nextRowProbe.pos.y;
                    auto ddy = l_currentProbe.pos.y - l_nextColumnProbe.pos.y;

                    if (ddx > Config::Get().m_probeHeightOffset)
                    {
                        auto l_verticalProbesCount = std::floor(ddx / Config::Get().m_probeHeightOffset);

                        l_maxVerticalProbesCount = std::max((uint32_t)l_verticalProbesCount + 1, l_maxVerticalProbesCount);

                        for (size_t k = 0; k < l_verticalProbesCount; k++)
                        {
                            Probe l_verticalProbe;
                            l_verticalProbe.pos = l_nextRowProbe.pos;
                            l_verticalProbe.pos.y += Config::Get().m_probeHeightOffset * (k + 1);

                            l_wallProbes.emplace_back(l_verticalProbe);
                        }
                    }
                    if (ddx < -Config::Get().m_probeHeightOffset)
                    {
                        auto l_verticalProbesCount = std::floor(std::abs(ddx) / Config::Get().m_probeHeightOffset);

                        l_maxVerticalProbesCount = std::max((uint32_t)l_verticalProbesCount + 1, l_maxVerticalProbesCount);

                        for (size_t k = 0; k < l_verticalProbesCount; k++)
                        {
                            Probe l_verticalProbe;
                            l_verticalProbe.pos = l_currentProbe.pos;
                            l_verticalProbe.pos.y += Config::Get().m_probeHeightOffset * (k + 1);

                            l_wallProbes.emplace_back(l_verticalProbe);
                        }
                    }
                    if (ddy > Config::Get().m_probeHeightOffset)
                    {
                        auto l_verticalProbesCount = std::floor(ddy / Config::Get().m_probeHeightOffset);

                        l_maxVerticalProbesCount = std::max((uint32_t)l_verticalProbesCount + 1, l_maxVerticalProbesCount);

                        for (size_t k = 0; k < l_verticalProbesCount; k++)
                        {
                            Probe l_verticalProbe;
                            l_verticalProbe.pos = l_nextColumnProbe.pos;
                            l_verticalProbe.pos.y += Config::Get().m_probeHeightOffset * (k + 1);

                            l_wallProbes.emplace_back(l_verticalProbe);
                        }
                    }
                    if (ddy < -Config::Get().m_probeHeightOffset)
                    {
                        auto l_verticalProbesCount = std::floor(std::abs(ddy) / Config::Get().m_probeHeightOffset);

                        l_maxVerticalProbesCount = std::max((uint32_t)l_verticalProbesCount + 1, l_maxVerticalProbesCount);

                        for (size_t k = 0; k < l_verticalProbesCount; k++)
                        {
                            Probe l_verticalProbe;
                            l_verticalProbe.pos = l_currentProbe.pos;
                            l_verticalProbe.pos.y += Config::Get().m_probeHeightOffset * (k + 1);

                            l_wallProbes.emplace_back(l_verticalProbe);
                        }
                    }
                }
            }

            l_wallProbes.shrink_to_fit();
            probes.insert(probes.end(), l_wallProbes.begin(), l_wallProbes.end());
            probes.shrink_to_fit();

            Log(Success, "", probes.size() - l_probesCount, " probe location generated along the wall.");

            return l_maxVerticalProbesCount;
        }
    }
}