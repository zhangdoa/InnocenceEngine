#include "SurfelGenerator.h"

#include "../../Client/DefaultGPUBuffers/DefaultGPUBuffers.h"

#include "../../Engine/Common/MathHelper.h"

#include "../../Engine/Interface/IEngine.h"

using namespace Inno;
extern INNO_ENGINE_API IEngine* g_Engine;

#include "../../Engine/Core/IOService.h"
#include "Baker.h"
#include "Serializer.h"

using namespace DefaultGPUBuffers;

namespace Inno
{
    namespace Baker
    {
        void SurfelGenerator::setup()
        {
            m_SPC_Surfel = g_Engine->getRenderingServer()->AddShaderProgramComponent("GIBakeSurfelPass/");

            m_SPC_Surfel->m_ShaderFilePaths.m_VSPath = "GIBakeSurfelPass.vert/";
            m_SPC_Surfel->m_ShaderFilePaths.m_GSPath = "GIBakeSurfelPass.geom/";
            m_SPC_Surfel->m_ShaderFilePaths.m_PSPath = "GIBakeSurfelPass.frag/";

            g_Engine->getRenderingServer()->InitializeShaderProgramComponent(m_SPC_Surfel);

            m_RenderPassComp_Surfel = g_Engine->getRenderingServer()->AddRenderPassComponent("GIBakeSurfelPass/");

            auto l_RenderPassDesc = g_Engine->getRenderingFrontend()->GetDefaultRenderPassDesc();
            l_RenderPassDesc.m_UseDepthBuffer = true;
            l_RenderPassDesc.m_UseStencilBuffer = true;
            
            m_RenderPassComp_Surfel->m_RenderPassDesc = l_RenderPassDesc;

            m_RenderPassComp_Surfel->m_RenderPassDesc.m_RenderTargetCount = 3;

            m_RenderPassComp_Surfel->m_RenderPassDesc.m_RenderTargetDesc.Sampler = TextureSampler::SamplerCubemap;
            m_RenderPassComp_Surfel->m_RenderPassDesc.m_RenderTargetDesc.Width = Config::Get().m_captureResolution;
            m_RenderPassComp_Surfel->m_RenderPassDesc.m_RenderTargetDesc.Height = Config::Get().m_captureResolution;
            m_RenderPassComp_Surfel->m_RenderPassDesc.m_RenderTargetDesc.PixelDataType = TexturePixelDataType::Float32;

            m_RenderPassComp_Surfel->m_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_DepthEnable = true;
            m_RenderPassComp_Surfel->m_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_AllowDepthWrite = true;
            m_RenderPassComp_Surfel->m_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_AllowDepthClamp = true;
            m_RenderPassComp_Surfel->m_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_DepthComparisionFunction = ComparisionFunction::LessEqual;

            m_RenderPassComp_Surfel->m_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_StencilEnable = true;
            m_RenderPassComp_Surfel->m_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_AllowStencilWrite = true;
            m_RenderPassComp_Surfel->m_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_StencilReference = 0x01;
            m_RenderPassComp_Surfel->m_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_FrontFaceStencilPassOperation = StencilOperation::Replace;
            m_RenderPassComp_Surfel->m_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_FrontFaceStencilComparisionFunction = ComparisionFunction::Always;
            m_RenderPassComp_Surfel->m_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_BackFaceStencilPassOperation = StencilOperation::Replace;
            m_RenderPassComp_Surfel->m_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_BackFaceStencilComparisionFunction = ComparisionFunction::Always;

            m_RenderPassComp_Surfel->m_RenderPassDesc.m_GraphicsPipelineDesc.m_RasterizerDesc.m_UseCulling = true;
            m_RenderPassComp_Surfel->m_RenderPassDesc.m_GraphicsPipelineDesc.m_ViewportDesc.m_Width = (float)Config::Get().m_captureResolution;
            m_RenderPassComp_Surfel->m_RenderPassDesc.m_GraphicsPipelineDesc.m_ViewportDesc.m_Height = (float)Config::Get().m_captureResolution;

            m_RenderPassComp_Surfel->m_ResourceBindingLayoutDescs.resize(9);
            m_RenderPassComp_Surfel->m_ResourceBindingLayoutDescs[0].m_GPUResourceType = GPUResourceType::Buffer;
            m_RenderPassComp_Surfel->m_ResourceBindingLayoutDescs[0].m_DescriptorSetIndex = 0;
            m_RenderPassComp_Surfel->m_ResourceBindingLayoutDescs[0].m_DescriptorIndex = 8;

            m_RenderPassComp_Surfel->m_ResourceBindingLayoutDescs[1].m_GPUResourceType = GPUResourceType::Buffer;
            m_RenderPassComp_Surfel->m_ResourceBindingLayoutDescs[1].m_DescriptorSetIndex = 0;
            m_RenderPassComp_Surfel->m_ResourceBindingLayoutDescs[1].m_DescriptorIndex = 1;

            m_RenderPassComp_Surfel->m_ResourceBindingLayoutDescs[2].m_GPUResourceType = GPUResourceType::Buffer;
            m_RenderPassComp_Surfel->m_ResourceBindingLayoutDescs[2].m_DescriptorSetIndex = 0;
            m_RenderPassComp_Surfel->m_ResourceBindingLayoutDescs[2].m_DescriptorIndex = 2;

            m_RenderPassComp_Surfel->m_ResourceBindingLayoutDescs[3].m_GPUResourceType = GPUResourceType::Image;
            m_RenderPassComp_Surfel->m_ResourceBindingLayoutDescs[3].m_DescriptorSetIndex = 1;
            m_RenderPassComp_Surfel->m_ResourceBindingLayoutDescs[3].m_DescriptorIndex = 0;
            m_RenderPassComp_Surfel->m_ResourceBindingLayoutDescs[3].m_IndirectBinding = true;

            m_RenderPassComp_Surfel->m_ResourceBindingLayoutDescs[4].m_GPUResourceType = GPUResourceType::Image;
            m_RenderPassComp_Surfel->m_ResourceBindingLayoutDescs[4].m_DescriptorSetIndex = 1;
            m_RenderPassComp_Surfel->m_ResourceBindingLayoutDescs[4].m_DescriptorIndex = 1;
            m_RenderPassComp_Surfel->m_ResourceBindingLayoutDescs[4].m_IndirectBinding = true;

            m_RenderPassComp_Surfel->m_ResourceBindingLayoutDescs[5].m_GPUResourceType = GPUResourceType::Image;
            m_RenderPassComp_Surfel->m_ResourceBindingLayoutDescs[5].m_DescriptorSetIndex = 1;
            m_RenderPassComp_Surfel->m_ResourceBindingLayoutDescs[5].m_DescriptorIndex = 2;
            m_RenderPassComp_Surfel->m_ResourceBindingLayoutDescs[5].m_SubresourceCount = 1;
            m_RenderPassComp_Surfel->m_ResourceBindingLayoutDescs[5].m_IndirectBinding = true;

            m_RenderPassComp_Surfel->m_ResourceBindingLayoutDescs[6].m_GPUResourceType = GPUResourceType::Image;
            m_RenderPassComp_Surfel->m_ResourceBindingLayoutDescs[6].m_DescriptorSetIndex = 1;
            m_RenderPassComp_Surfel->m_ResourceBindingLayoutDescs[6].m_DescriptorIndex = 3;
            m_RenderPassComp_Surfel->m_ResourceBindingLayoutDescs[6].m_IndirectBinding = true;

            m_RenderPassComp_Surfel->m_ResourceBindingLayoutDescs[7].m_GPUResourceType = GPUResourceType::Image;
            m_RenderPassComp_Surfel->m_ResourceBindingLayoutDescs[7].m_DescriptorSetIndex = 1;
            m_RenderPassComp_Surfel->m_ResourceBindingLayoutDescs[7].m_DescriptorIndex = 4;
            m_RenderPassComp_Surfel->m_ResourceBindingLayoutDescs[7].m_IndirectBinding = true;

            m_RenderPassComp_Surfel->m_ResourceBindingLayoutDescs[8].m_GPUResourceType = GPUResourceType::Sampler;
            m_RenderPassComp_Surfel->m_ResourceBindingLayoutDescs[8].m_DescriptorSetIndex = 2;
            m_RenderPassComp_Surfel->m_ResourceBindingLayoutDescs[8].m_DescriptorIndex = 0;
            m_RenderPassComp_Surfel->m_ResourceBindingLayoutDescs[8].m_IndirectBinding = true;

            m_RenderPassComp_Surfel->m_ShaderProgram = m_SPC_Surfel;

            g_Engine->getRenderingServer()->InitializeRenderPassComponent(m_RenderPassComp_Surfel);

            m_SamplerComp_Surfel = g_Engine->getRenderingServer()->AddSamplerComponent("GIBakeSurfelPass/");

            m_SamplerComp_Surfel->m_SamplerDesc.m_WrapMethodU = TextureWrapMethod::Repeat;
            m_SamplerComp_Surfel->m_SamplerDesc.m_WrapMethodV = TextureWrapMethod::Repeat;

            g_Engine->getRenderingServer()->InitializeSamplerComponent(m_SamplerComp_Surfel);
        }

        bool SurfelGenerator::captureSurfels(std::vector<Probe>& probes)
        {
            g_Engine->getLogSystem()->Log(LogLevel::Success, "SurfelGenerator: Start to capture surfels...");

            auto l_perFrameConstantBuffer = g_Engine->getRenderingFrontend()->GetPerFrameConstantBuffer();

            auto l_p = Math::generatePerspectiveMatrix((90.0f / 180.0f) * PI<float>, 1.0f, l_perFrameConstantBuffer.zNear, l_perFrameConstantBuffer.zFar);

            auto l_rPX = Math::lookAt(Vec4(0.0f, 0.0f, 0.0f, 1.0f), Vec4(1.0f, 0.0f, 0.0f, 1.0f), Vec4(0.0f, -1.0f, 0.0f, 0.0f));
            auto l_rNX = Math::lookAt(Vec4(0.0f, 0.0f, 0.0f, 1.0f), Vec4(-1.0f, 0.0f, 0.0f, 1.0f), Vec4(0.0f, -1.0f, 0.0f, 0.0f));
            auto l_rPY = Math::lookAt(Vec4(0.0f, 0.0f, 0.0f, 1.0f), Vec4(0.0f, 1.0f, 0.0f, 1.0f), Vec4(0.0f, 0.0f, 1.0f, 0.0f));
            auto l_rNY = Math::lookAt(Vec4(0.0f, 0.0f, 0.0f, 1.0f), Vec4(0.0f, -1.0f, 0.0f, 1.0f), Vec4(0.0f, 0.0f, 1.0f, 0.0f));
            auto l_rPZ = Math::lookAt(Vec4(0.0f, 0.0f, 0.0f, 1.0f), Vec4(0.0f, 0.0f, 1.0f, 1.0f), Vec4(0.0f, -1.0f, 0.0f, 0.0f));
            auto l_rNZ = Math::lookAt(Vec4(0.0f, 0.0f, 0.0f, 1.0f), Vec4(0.0f, 0.0f, -1.0f, 1.0f), Vec4(0.0f, -1.0f, 0.0f, 0.0f));

            std::vector<Mat4> l_v =
            {
                l_rPX, l_rNX, l_rPY, l_rNY, l_rPZ, l_rNZ
            };

            auto l_probeForSurfelCachesCount = probes.size();

            std::vector<Surfel> l_surfelCaches;
            l_surfelCaches.reserve(l_probeForSurfelCachesCount * Config::Get().m_surfelSampleCountPerFace * Config::Get().m_surfelSampleCountPerFace * 6);

            for (uint32_t i = 0; i < l_probeForSurfelCachesCount; i++)
            {
                drawObjects(probes[i], l_p, l_v);

                readBackSurfelCaches(probes[i], l_surfelCaches);

                g_Engine->getLogSystem()->Log(LogLevel::Verbose, "SurfelGenerator: Progress: ", (float)i * 100.0f / (float)l_probeForSurfelCachesCount, "%...");
            }

            g_Engine->getLogSystem()->Log(LogLevel::Success, "SurfelGenerator: ", l_surfelCaches.size(), " surfel caches captured.");

            serializeProbes(probes);

            eliminateDuplicatedSurfels(l_surfelCaches);

            serializeSurfelCaches(l_surfelCaches);

            return true;
        }

        bool SurfelGenerator::drawObjects(Probe& probeCache, const Mat4& p, const std::vector<Mat4>& v)
        {
            auto l_t = Math::getInvertTranslationMatrix(probeCache.pos);

            std::vector<Mat4> l_GICameraConstantBuffer(8);
            l_GICameraConstantBuffer[0] = p;
            for (size_t i = 0; i < 6; i++)
            {
                l_GICameraConstantBuffer[i + 1] = v[i];
            }
            l_GICameraConstantBuffer[7] = l_t;

            g_Engine->getRenderingServer()->UploadGPUBufferComponent(GetGPUBufferComponent(GPUBufferUsageType::GI), l_GICameraConstantBuffer);

            auto l_MeshGPUBufferComp = GetGPUBufferComponent(GPUBufferUsageType::Mesh);
            auto l_MaterialGPUBufferComp = GetGPUBufferComponent(GPUBufferUsageType::Material);

            g_Engine->getRenderingServer()->CommandListBegin(m_RenderPassComp_Surfel, 0);
            g_Engine->getRenderingServer()->BindRenderPassComponent(m_RenderPassComp_Surfel);
            g_Engine->getRenderingServer()->ClearRenderTargets(m_RenderPassComp_Surfel);
            g_Engine->getRenderingServer()->BindGPUResource(m_RenderPassComp_Surfel, ShaderStage::Pixel, m_SamplerComp_Surfel, 8);
            g_Engine->getRenderingServer()->BindGPUResource(m_RenderPassComp_Surfel, ShaderStage::Geometry, GetGPUBufferComponent(GPUBufferUsageType::GI), 0, Accessibility::ReadOnly);

            uint32_t l_offset = 0;

            for (uint32_t i = 0; i < Config::Get().m_staticMeshDrawCallCount; i++)
            {
                auto l_staticPerObjectConstantBuffer = Config::Get().m_staticMeshDrawCallInfo[i];

                if (l_staticPerObjectConstantBuffer.mesh->m_ObjectStatus == ObjectStatus::Activated)
                {
                    g_Engine->getRenderingServer()->BindGPUResource(m_RenderPassComp_Surfel, ShaderStage::Vertex, l_MeshGPUBufferComp, 1, Accessibility::ReadOnly, l_offset, 1);
                    g_Engine->getRenderingServer()->BindGPUResource(m_RenderPassComp_Surfel, ShaderStage::Pixel, l_MaterialGPUBufferComp, 2, Accessibility::ReadOnly, l_offset, 1);

                    if (l_staticPerObjectConstantBuffer.material->m_ObjectStatus == ObjectStatus::Activated)
                    {
                        g_Engine->getRenderingServer()->BindGPUResource(m_RenderPassComp_Surfel, ShaderStage::Pixel, l_staticPerObjectConstantBuffer.material->m_TextureSlots[0].m_Texture, 3);
                        g_Engine->getRenderingServer()->BindGPUResource(m_RenderPassComp_Surfel, ShaderStage::Pixel, l_staticPerObjectConstantBuffer.material->m_TextureSlots[1].m_Texture, 4);
                        g_Engine->getRenderingServer()->BindGPUResource(m_RenderPassComp_Surfel, ShaderStage::Pixel, l_staticPerObjectConstantBuffer.material->m_TextureSlots[2].m_Texture, 5);
                        g_Engine->getRenderingServer()->BindGPUResource(m_RenderPassComp_Surfel, ShaderStage::Pixel, l_staticPerObjectConstantBuffer.material->m_TextureSlots[3].m_Texture, 6);
                        g_Engine->getRenderingServer()->BindGPUResource(m_RenderPassComp_Surfel, ShaderStage::Pixel, l_staticPerObjectConstantBuffer.material->m_TextureSlots[4].m_Texture, 7);
                    }

                    g_Engine->getRenderingServer()->DrawIndexedInstanced(m_RenderPassComp_Surfel, l_staticPerObjectConstantBuffer.mesh);

                    if (l_staticPerObjectConstantBuffer.material->m_ObjectStatus == ObjectStatus::Activated)
                    {
                        g_Engine->getRenderingServer()->UnbindGPUResource(m_RenderPassComp_Surfel, ShaderStage::Pixel, l_staticPerObjectConstantBuffer.material->m_TextureSlots[0].m_Texture, 3);
                        g_Engine->getRenderingServer()->UnbindGPUResource(m_RenderPassComp_Surfel, ShaderStage::Pixel, l_staticPerObjectConstantBuffer.material->m_TextureSlots[1].m_Texture, 4);
                        g_Engine->getRenderingServer()->UnbindGPUResource(m_RenderPassComp_Surfel, ShaderStage::Pixel, l_staticPerObjectConstantBuffer.material->m_TextureSlots[2].m_Texture, 5);
                        g_Engine->getRenderingServer()->UnbindGPUResource(m_RenderPassComp_Surfel, ShaderStage::Pixel, l_staticPerObjectConstantBuffer.material->m_TextureSlots[3].m_Texture, 6);
                        g_Engine->getRenderingServer()->UnbindGPUResource(m_RenderPassComp_Surfel, ShaderStage::Pixel, l_staticPerObjectConstantBuffer.material->m_TextureSlots[4].m_Texture, 7);
                    }
                }

                l_offset++;
            }

            g_Engine->getRenderingServer()->CommandListEnd(m_RenderPassComp_Surfel);

            g_Engine->getRenderingServer()->ExecuteCommandList(m_RenderPassComp_Surfel, GPUEngineType::Graphics);
            g_Engine->getRenderingServer()->WaitCommandQueue(m_RenderPassComp_Surfel, GPUEngineType::Graphics, GPUEngineType::Graphics);
            g_Engine->getRenderingServer()->WaitFence(m_RenderPassComp_Surfel, GPUEngineType::Graphics);

            return true;
        }

        bool SurfelGenerator::readBackSurfelCaches(Probe& probe, std::vector<Surfel>& surfelCaches)
        {
            static uint32_t l_index = 0;

            auto l_posWSMetallic = g_Engine->getRenderingServer()->ReadTextureBackToCPU(m_RenderPassComp_Surfel, m_RenderPassComp_Surfel->m_RenderTargets[0].m_Texture);
            auto l_normalRoughness = g_Engine->getRenderingServer()->ReadTextureBackToCPU(m_RenderPassComp_Surfel, m_RenderPassComp_Surfel->m_RenderTargets[1].m_Texture);
            auto l_albedoAO = g_Engine->getRenderingServer()->ReadTextureBackToCPU(m_RenderPassComp_Surfel, m_RenderPassComp_Surfel->m_RenderTargets[2].m_Texture);
            auto l_depthStencilRT = g_Engine->getRenderingServer()->ReadTextureBackToCPU(m_RenderPassComp_Surfel, m_RenderPassComp_Surfel->m_DepthStencilRenderTarget.m_Texture);

            g_Engine->getAssetSystem()->SaveTexture(("..//Res//Intermediate//SurfelTextureAlbedo_" + std::to_string(l_index)).c_str(), m_RenderPassComp_Surfel->m_RenderTargets[2].m_Texture->m_TextureDesc, l_albedoAO.data());

            auto l_surfelsCount = Config::Get().m_surfelSampleCountPerFace * Config::Get().m_surfelSampleCountPerFace * 6;
            auto l_sampleStep = Config::Get().m_captureResolution / Config::Get().m_surfelSampleCountPerFace;

            std::vector<Surfel> l_surfels(l_surfelsCount);
            for (size_t i = 0; i < l_surfelsCount; i++)
            {
                l_surfels[i].pos = l_posWSMetallic[i * l_sampleStep];
                l_surfels[i].pos.w = 1.0f;
                l_surfels[i].normal = l_normalRoughness[i * l_sampleStep];
                l_surfels[i].normal.w = 0.0f;
                l_surfels[i].albedo = l_albedoAO[i * l_sampleStep];
                l_surfels[i].albedo.w = 1.0f;
                l_surfels[i].MRAT.x = l_posWSMetallic[i * l_sampleStep].w;
                l_surfels[i].MRAT.y = l_normalRoughness[i * l_sampleStep].w;
                l_surfels[i].MRAT.z = l_albedoAO[i * l_sampleStep].w;
                l_surfels[i].MRAT.w = 1.0f;
            }

            auto l_depthStencilRTSize = l_depthStencilRT.size();

            std::vector<Vec4> l_DSTextureCompData(l_depthStencilRTSize);

            l_depthStencilRTSize /= 6;

            for (size_t i = 0; i < 6; i++)
            {
                uint32_t l_stencil = 0;
                for (size_t j = 0; j < l_depthStencilRTSize; j++)
                {
                    auto& l_depthStencil = l_depthStencilRT[i * l_depthStencilRTSize + j];

                    if (l_depthStencil.y == 1.0f)
                    {
                        l_stencil++;
                        l_DSTextureCompData[i * l_depthStencilRTSize + j] = Vec4(1.0f, 1.0f, 1.0f, 0.0f);
                    }
                }

                probe.skyVisibility[i] = 1.0f - ((float)l_stencil / (float)l_depthStencilRTSize);
            }

            g_Engine->getAssetSystem()->SaveTexture(("..//Res//Intermediate//SurfelTextureDS_" + std::to_string(l_index)).c_str(), m_RenderPassComp_Surfel->m_DepthStencilRenderTarget.m_Texture->m_TextureDesc, l_DSTextureCompData.data());
            surfelCaches.insert(surfelCaches.end(), l_surfels.begin(), l_surfels.end());

            l_index++;

            return true;
        }

        bool SurfelGenerator::eliminateDuplicatedSurfels(std::vector<Surfel>& surfelCaches)
        {
            g_Engine->getLogSystem()->Log(LogLevel::Success, "SurfelGenerator: Start to eliminate duplicated surfels...");

            std::sort(surfelCaches.begin(), surfelCaches.end(), [&](Surfel A, Surfel B)
                {
                    if (A.pos.x != B.pos.x) {
                        return A.pos.x < B.pos.x;
                    }
                    if (A.pos.y != B.pos.y) {
                        return A.pos.y < B.pos.y;
                    }
                    return A.pos.z < B.pos.z;
                });

            surfelCaches.erase(std::unique(surfelCaches.begin(), surfelCaches.end()), surfelCaches.end());
            surfelCaches.shrink_to_fit();

            g_Engine->getLogSystem()->Log(LogLevel::Success, "SurfelGenerator: Duplicated surfels have been removed, there are ", surfelCaches.size(), " surfels now.");

            return true;
        }
    }
}
