#include "BrickGenerator.h"

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
        void BrickGenerator::setup()
        {
            m_SPC_BrickFactor = g_Engine->getRenderingServer()->AddShaderProgramComponent("GIBakeBrickFactorPass/");

            m_SPC_BrickFactor->m_ShaderFilePaths.m_VSPath = "GIBakeBrickFactorPass.vert/";
            m_SPC_BrickFactor->m_ShaderFilePaths.m_GSPath = "GIBakeBrickFactorPass.geom/";
            m_SPC_BrickFactor->m_ShaderFilePaths.m_PSPath = "GIBakeBrickFactorPass.frag/";

            g_Engine->getRenderingServer()->InitializeShaderProgramComponent(m_SPC_BrickFactor);

            m_RenderPassComp_BrickFactor = g_Engine->getRenderingServer()->AddRenderPassComponent("GIBakeBrickFactorPass/");

            auto l_RenderPassDesc = g_Engine->getRenderingFrontend()->GetDefaultRenderPassDesc();
            l_RenderPassDesc.m_UseDepthBuffer = true;
            l_RenderPassDesc.m_UseStencilBuffer = true;

            m_RenderPassComp_BrickFactor->m_RenderPassDesc = l_RenderPassDesc;
            m_RenderPassComp_BrickFactor->m_RenderPassDesc.m_RenderTargetDesc.Sampler = TextureSampler::SamplerCubemap;
            m_RenderPassComp_BrickFactor->m_RenderPassDesc.m_RenderTargetDesc.Width = 64;
            m_RenderPassComp_BrickFactor->m_RenderPassDesc.m_RenderTargetDesc.Height = 64;
            m_RenderPassComp_BrickFactor->m_RenderPassDesc.m_RenderTargetDesc.PixelDataType = TexturePixelDataType::Float32;

            m_RenderPassComp_BrickFactor->m_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_DepthEnable = true;
            m_RenderPassComp_BrickFactor->m_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_AllowDepthWrite = true;
            m_RenderPassComp_BrickFactor->m_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_AllowDepthClamp = true;
            m_RenderPassComp_BrickFactor->m_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_DepthComparisionFunction = ComparisionFunction::LessEqual;
            m_RenderPassComp_BrickFactor->m_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_StencilEnable = true;
            m_RenderPassComp_BrickFactor->m_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_AllowStencilWrite = true;
            m_RenderPassComp_BrickFactor->m_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_StencilReference = 0x01;
            m_RenderPassComp_BrickFactor->m_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_FrontFaceStencilPassOperation = StencilOperation::Replace;
            m_RenderPassComp_BrickFactor->m_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_FrontFaceStencilComparisionFunction = ComparisionFunction::Always;
            m_RenderPassComp_BrickFactor->m_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_BackFaceStencilPassOperation = StencilOperation::Replace;
            m_RenderPassComp_BrickFactor->m_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_BackFaceStencilComparisionFunction = ComparisionFunction::Always;

            m_RenderPassComp_BrickFactor->m_RenderPassDesc.m_GraphicsPipelineDesc.m_RasterizerDesc.m_UseCulling = true;
            m_RenderPassComp_BrickFactor->m_RenderPassDesc.m_GraphicsPipelineDesc.m_ViewportDesc.m_Width = 64;
            m_RenderPassComp_BrickFactor->m_RenderPassDesc.m_GraphicsPipelineDesc.m_ViewportDesc.m_Height = 64;

            m_RenderPassComp_BrickFactor->m_ResourceBindingLayoutDescs.resize(2);
            m_RenderPassComp_BrickFactor->m_ResourceBindingLayoutDescs[0].m_GPUResourceType = GPUResourceType::Buffer;
            m_RenderPassComp_BrickFactor->m_ResourceBindingLayoutDescs[0].m_DescriptorSetIndex = 0;
            m_RenderPassComp_BrickFactor->m_ResourceBindingLayoutDescs[0].m_DescriptorIndex = 8;

            m_RenderPassComp_BrickFactor->m_ResourceBindingLayoutDescs[1].m_GPUResourceType = GPUResourceType::Buffer;
            m_RenderPassComp_BrickFactor->m_ResourceBindingLayoutDescs[1].m_DescriptorSetIndex = 0;
            m_RenderPassComp_BrickFactor->m_ResourceBindingLayoutDescs[1].m_DescriptorIndex = 1;

            m_RenderPassComp_BrickFactor->m_ShaderProgram = m_SPC_BrickFactor;

            g_Engine->getRenderingServer()->InitializeRenderPassComponent(m_RenderPassComp_BrickFactor);
        }

        bool BrickGenerator::generateBrickCaches(std::vector<Surfel>& surfelCaches)
        {
            g_Engine->getLogSystem()->Log(LogLevel::Success, "BrickGenerator: Start to generate brick caches...");

            // Find bound corner position
            auto l_surfelsCount = surfelCaches.size();

            auto l_startPos = Math::maxVec4<double>;
            l_startPos.w = 1.0;

            auto l_endPos = Math::minVec4<double>;
            l_endPos.w = 1.0;

            for (size_t i = 0; i < l_surfelsCount; i++)
            {
                auto l_surfelPos = precisionConvert<float, double>(surfelCaches[i].pos);

                l_startPos = Math::elementWiseMin(l_surfelPos, l_startPos);
                l_endPos = Math::elementWiseMax(l_surfelPos, l_endPos);
            }

            // Fit the end corner to contain at least one brick in each axis
            auto l_extends = l_endPos - l_startPos;
            l_extends.w = 1.0;

            if (l_extends.x < Config::Get().m_brickSize.x)
            {
                l_endPos.x = l_startPos.x + Config::Get().m_brickSize.x;
            }
            if (l_extends.y < Config::Get().m_brickSize.y)
            {
                l_endPos.y = l_startPos.y + Config::Get().m_brickSize.y;
            }
            if (l_extends.z < Config::Get().m_brickSize.z)
            {
                l_endPos.z = l_startPos.z + Config::Get().m_brickSize.z;
            }

            // Adjusted extends
            auto l_adjustedExtends = l_endPos - l_startPos;
            l_adjustedExtends.w = 1.0f;

            auto l_bricksCountX = (size_t)std::ceil(l_adjustedExtends.x / Config::Get().m_brickSize.x);
            auto l_bricksCountY = (size_t)std::ceil(l_adjustedExtends.y / Config::Get().m_brickSize.y);
            auto l_bricksCountZ = (size_t)std::ceil(l_adjustedExtends.z / Config::Get().m_brickSize.z);
            auto l_brickCount = TVec4<size_t>(l_bricksCountX, l_bricksCountY, l_bricksCountZ, 1);

            // Adjusted end
            TVec4<double> l_adjustedEndPos = TVec4<double>();
            l_adjustedEndPos.x = l_startPos.x + l_bricksCountX * Config::Get().m_brickSize.x;
            l_adjustedEndPos.y = l_startPos.y + l_bricksCountY * Config::Get().m_brickSize.y;
            l_adjustedEndPos.z = l_startPos.z + l_bricksCountZ * Config::Get().m_brickSize.z;
            l_adjustedEndPos.w = 1.0;

            // generate all possible brick position
            auto l_totalBricksWorkCount = l_bricksCountX * l_bricksCountY * l_bricksCountZ;

            std::vector<BrickCache> l_brickCaches;
            l_brickCaches.reserve(l_totalBricksWorkCount);

            auto l_currentMaxPos = l_startPos + Config::Get().m_brickSize;
            auto l_currentMinPos = l_startPos;

            auto l_averangeSurfelInABrick = l_surfelsCount / l_totalBricksWorkCount;

            while (l_currentMaxPos.z <= l_adjustedEndPos.z)
            {
                l_currentMaxPos.y = l_startPos.y + Config::Get().m_brickSize.y;
                l_currentMinPos.y = l_startPos.y;

                while (l_currentMaxPos.y <= l_adjustedEndPos.y)
                {
                    l_currentMaxPos.x = l_startPos.x + Config::Get().m_brickSize.x;
                    l_currentMinPos.x = l_startPos.x;

                    while (l_currentMaxPos.x <= l_adjustedEndPos.x)
                    {
                        BrickCache l_brickCache;
                        l_brickCache.pos = precisionConvert<double, float>(l_currentMinPos + Config::Get().m_halfBrickSize);
                        l_brickCache.surfelCaches.reserve(l_averangeSurfelInABrick);

                        l_brickCaches.emplace_back(std::move(l_brickCache));

                        l_currentMaxPos.x += Config::Get().m_brickSize.x;
                        l_currentMinPos.x += Config::Get().m_brickSize.x;
                    }

                    l_currentMaxPos.y += Config::Get().m_brickSize.y;
                    l_currentMinPos.y += Config::Get().m_brickSize.y;
                }

                l_currentMaxPos.z += Config::Get().m_brickSize.z;
                l_currentMinPos.z += Config::Get().m_brickSize.z;
            }

            // Assign surfels to brick cache
            for (size_t i = 0; i < l_surfelsCount; i++)
            {
                auto l_posVS = surfelCaches[i].pos - precisionConvert<double, float>(l_startPos);
                auto l_normalizedPos = l_posVS.scale(precisionConvert<double, float>(l_extends.reciprocal()));
                auto l_brickIndexX = (size_t)std::floor((float)(l_brickCount.x - 1) * l_normalizedPos.x);
                auto l_brickIndexY = (size_t)std::floor((float)(l_brickCount.y - 1) * l_normalizedPos.y);
                auto l_brickIndexZ = (size_t)std::floor((float)(l_brickCount.z - 1) * l_normalizedPos.z);
                auto l_brickIndex = l_brickIndexX + l_brickIndexY * l_brickCount.x + l_brickIndexZ * l_brickCount.x * l_brickCount.y;

                l_brickCaches[l_brickIndex].surfelCaches.emplace_back(surfelCaches[i]);

                g_Engine->getLogSystem()->Log(LogLevel::Verbose, "BrickGenerator: Progress: ", (float)i * 100.0f / (float)l_totalBricksWorkCount, "%...");
            }

            // Remove empty bricks
            l_brickCaches.erase(
                std::remove_if(l_brickCaches.begin(), l_brickCaches.end(),
                    [&](auto val) {
                        return val.surfelCaches.size() == 0;
                    }), l_brickCaches.end());

            l_brickCaches.shrink_to_fit();

            auto l_finalBrickCount = l_brickCaches.size();

            for (size_t i = 0; i < l_finalBrickCount; i++)
            {
                l_brickCaches[i].surfelCaches.shrink_to_fit();
            }

            g_Engine->getLogSystem()->Log(LogLevel::Success, "BrickGenerator: ", l_brickCaches.size(), " brick caches have been generated.");

            serializeBrickCaches(l_brickCaches);

            return true;
        }

        bool BrickGenerator::generateBricks(const std::vector<BrickCache>& brickCaches)
        {
            g_Engine->getLogSystem()->Log(LogLevel::Success, "BrickGenerator: Start to generate bricks...");

            // Generate real bricks with surfel range
            auto l_bricksCount = brickCaches.size();
            std::vector<Brick> l_bricks;
            l_bricks.reserve(l_bricksCount);
            std::vector<Surfel> l_surfels;

            size_t l_surfelsCount = 0;
            for (size_t i = 0; i < l_bricksCount; i++)
            {
                l_surfelsCount += brickCaches[i].surfelCaches.size();
            }

            l_surfels.reserve(l_surfelsCount);

            size_t l_offset = 0;

            for (size_t i = 0; i < l_bricksCount; i++)
            {
                Brick l_brick;
                auto l_halfBrickSizeFloat = precisionConvert<double, float>(Config::Get().m_halfBrickSize);
                l_brick.boundBox = Math::generateAABB(brickCaches[i].pos + l_halfBrickSizeFloat, brickCaches[i].pos - l_halfBrickSizeFloat);
                l_brick.surfelRangeBegin = (uint32_t)l_offset;
                l_brick.surfelRangeEnd = (uint32_t)(l_offset + brickCaches[i].surfelCaches.size() - 1);
                l_offset += brickCaches[i].surfelCaches.size();

                l_surfels.insert(l_surfels.end(), std::make_move_iterator(brickCaches[i].surfelCaches.begin()), std::make_move_iterator(brickCaches[i].surfelCaches.end()));

                l_bricks.emplace_back(l_brick);

                g_Engine->getLogSystem()->Log(LogLevel::Verbose, "BrickGenerator: Progress: ", (float)i * 100.0f / (float)l_bricksCount, "%...");
            }

            g_Engine->getLogSystem()->Log(LogLevel::Success, "BrickGenerator: ", l_bricksCount, " bricks have been generated.");

            serializeSurfels(l_surfels);
            serializeBricks(l_bricks);

            return true;
        }

        bool BrickGenerator::drawBricks(Vec4 pos, uint32_t bricksCount, const Mat4& p, const std::vector<Mat4>& v)
        {
            std::vector<Mat4> l_GICameraConstantBuffer(8);
            l_GICameraConstantBuffer[0] = p;
            for (size_t i = 0; i < 6; i++)
            {
                l_GICameraConstantBuffer[i + 1] = v[i];
            }
            l_GICameraConstantBuffer[7] = Math::getInvertTranslationMatrix(pos);

            g_Engine->getRenderingServer()->UploadGPUBufferComponent(GetGPUBufferComponent(GPUBufferUsageType::GI), l_GICameraConstantBuffer);

            auto l_MeshGPUBufferComp = GetGPUBufferComponent(GPUBufferUsageType::Mesh);

            auto l_mesh = g_Engine->getRenderingFrontend()->GetMeshComponent(ProceduralMeshShape::Cube);

            uint32_t l_offset = 0;

            g_Engine->getRenderingServer()->CommandListBegin(m_RenderPassComp_BrickFactor, 0);
            g_Engine->getRenderingServer()->BindRenderPassComponent(m_RenderPassComp_BrickFactor);
            g_Engine->getRenderingServer()->ClearRenderTargets(m_RenderPassComp_BrickFactor);
            g_Engine->getRenderingServer()->BindGPUResource(m_RenderPassComp_BrickFactor, ShaderStage::Geometry, GetGPUBufferComponent(GPUBufferUsageType::GI), 0, Accessibility::ReadOnly);

            for (uint32_t i = 0; i < bricksCount; i++)
            {
                g_Engine->getRenderingServer()->BindGPUResource(m_RenderPassComp_BrickFactor, ShaderStage::Vertex, l_MeshGPUBufferComp, 1, Accessibility::ReadOnly, l_offset, 1);

                g_Engine->getRenderingServer()->DrawIndexedInstanced(m_RenderPassComp_BrickFactor, l_mesh);

                l_offset++;
            }

            g_Engine->getRenderingServer()->CommandListEnd(m_RenderPassComp_BrickFactor);

            g_Engine->getRenderingServer()->ExecuteCommandList(m_RenderPassComp_BrickFactor, GPUEngineType::Graphics);

            return true;
        }

        bool BrickGenerator::readBackBrickFactors(Probe& probe, std::vector<BrickFactor>& brickFactors, const std::vector<Brick>& bricks)
        {
            static int l_index = 0;

            auto l_brickIDResults = g_Engine->getRenderingServer()->ReadTextureBackToCPU(m_RenderPassComp_BrickFactor, m_RenderPassComp_BrickFactor->m_RenderTargets[0].m_Texture);

            auto l_TextureComp = g_Engine->getRenderingServer()->AddTextureComponent();
            l_TextureComp->m_TextureDesc = m_RenderPassComp_BrickFactor->m_RenderTargets[0].m_Texture->m_TextureDesc;
            l_TextureComp->m_TextureData = l_brickIDResults.data();
            g_Engine->getAssetSystem()->SaveTexture(("..//Res//Intermediate//BrickTexture_" + std::to_string(l_index)).c_str(), l_TextureComp);
            l_index++;

            auto l_brickIDResultSize = l_brickIDResults.size();

            l_brickIDResultSize /= 6;

            // 6 axis-aligned coefficients
            for (size_t i = 0; i < 6; i++)
            {
                std::vector<BrickFactor> l_brickFactors;
                l_brickFactors.reserve(l_brickIDResultSize);

                for (size_t j = 0; j < l_brickIDResultSize; j++)
                {
                    auto& l_brickIDResult = l_brickIDResults[i * l_brickIDResultSize + j];
                    if (l_brickIDResult.y != 0.0f)
                    {
                        BrickFactor l_BrickFactor;

                        // Index start from 1
                        l_BrickFactor.brickIndex = (uint32_t)(std::round(l_brickIDResult.y) - 1.0f);
                        l_brickFactors.emplace_back(l_BrickFactor);
                    }
                }

                // Calculate brick weight
                if (l_brickFactors.size() > 0)
                {
                    std::sort(l_brickFactors.begin(), l_brickFactors.end(), [&](BrickFactor A, BrickFactor B)
                        {
                            return A.brickIndex < B.brickIndex;
                        });

                    l_brickFactors.erase(std::unique(l_brickFactors.begin(), l_brickFactors.end()), l_brickFactors.end());
                    l_brickFactors.shrink_to_fit();

                    if (l_brickFactors.size() == 1)
                    {
                        l_brickFactors[0].basisWeight = 1.0f;
                    }
                    else
                    {
                        // Weight
                        auto l_brickFactorSize = l_brickFactors.size();

                        // World space distance
                        for (size_t j = 0; j < l_brickFactorSize; j++)
                        {
                            l_brickFactors[j].basisWeight = (bricks[l_brickFactors[j].brickIndex].boundBox.m_center - probe.pos).length();
                        }

                        auto l_min = std::numeric_limits<float>().max();
                        auto l_max = std::numeric_limits<float>().min();

                        for (size_t j = 0; j < l_brickFactorSize; j++)
                        {
                            l_min = l_brickFactors[j].basisWeight <= l_min ? l_brickFactors[j].basisWeight : l_min;
                            l_max = l_brickFactors[j].basisWeight >= l_max ? l_brickFactors[j].basisWeight : l_max;
                        }

                        auto l_range = l_max + l_min;

                        // Reverse along the view space Z axis
                        for (size_t j = 0; j < l_brickFactorSize; j++)
                        {
                            l_brickFactors[j].basisWeight = (l_range - l_brickFactors[j].basisWeight);
                        }

                        // Normalize
                        float denom = 0.0f;
                        for (size_t j = 0; j < l_brickFactorSize; j++)
                        {
                            denom += l_brickFactors[j].basisWeight;
                        }

                        for (size_t j = 0; j < l_brickFactorSize; j++)
                        {
                            l_brickFactors[j].basisWeight /= denom;
                        }
                    }

                    // Assign brick factor range to probes
                    auto l_brickFactorRangeBegin = brickFactors.size();
                    auto l_brickFactorRangeEnd = l_brickFactorRangeBegin + l_brickFactors.size() - 1;

                    probe.brickFactorRange[i * 2] = (uint32_t)l_brickFactorRangeBegin;
                    probe.brickFactorRange[i * 2 + 1] = (uint32_t)l_brickFactorRangeEnd;

                    brickFactors.insert(brickFactors.end(), std::make_move_iterator(l_brickFactors.begin()), std::make_move_iterator(l_brickFactors.end()));
                }
                else
                {
                    probe.brickFactorRange[i * 2] = -1;
                    probe.brickFactorRange[i * 2 + 1] = -1;
                }
            }

            return true;
        }

        bool BrickGenerator::assignBrickFactorToProbesByGPU(const std::vector<Brick>& bricks, std::vector<Probe>& probes)
        {
            g_Engine->getLogSystem()->Log(LogLevel::Success, "BrickGenerator: Start to generate brick factor and assign to probes...");

            // Upload camera data and brick cubes data to GPU memory
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

            auto l_p = Math::generatePerspectiveMatrix((90.0f / 180.0f) * PI<float>, 1.0f, 0.1f, 2000.0f);

            auto l_bricksCount = bricks.size();

            std::vector<PerObjectConstantBuffer> l_bricksCubePerObjectConstantBuffer;
            l_bricksCubePerObjectConstantBuffer.resize(l_bricksCount);

            for (size_t i = 0; i < l_bricksCount; i++)
            {
                auto l_t = Math::toTranslationMatrix(bricks[i].boundBox.m_center);

                l_bricksCubePerObjectConstantBuffer[i].m = l_t;

                // @TODO: Find a better way to assign without error
                l_bricksCubePerObjectConstantBuffer[i].m.m00 *= (bricks[i].boundBox.m_extend.x / 2.0f) - 0.1f;
                l_bricksCubePerObjectConstantBuffer[i].m.m11 *= (bricks[i].boundBox.m_extend.y / 2.0f) - 0.1f;
                l_bricksCubePerObjectConstantBuffer[i].m.m22 *= (bricks[i].boundBox.m_extend.z / 2.0f) - 0.1f;

                // Index start from 1
                l_bricksCubePerObjectConstantBuffer[i].UUID = (float)i + 1.0f;
            }

            auto l_MeshGPUBufferComp = GetGPUBufferComponent(GPUBufferUsageType::Mesh);
            g_Engine->getRenderingServer()->UploadGPUBufferComponent(l_MeshGPUBufferComp, l_bricksCubePerObjectConstantBuffer, 0, l_bricksCubePerObjectConstantBuffer.size());

            // assign bricks to probe by the depth test result
            auto l_probesCount = probes.size();

            std::vector<BrickFactor> l_brickFactors;
            l_brickFactors.reserve(l_probesCount * l_bricksCount);

            for (size_t i = 0; i < l_probesCount; i++)
            {
                drawBricks(probes[i].pos, (uint32_t)l_bricksCount, l_p, l_v);
                readBackBrickFactors(probes[i], l_brickFactors, bricks);

                g_Engine->getLogSystem()->Log(LogLevel::Verbose, "BrickGenerator: Progress: ", (float)i * 100.0f / (float)l_probesCount, "%...");
            }

            l_brickFactors.shrink_to_fit();

            g_Engine->getLogSystem()->Log(LogLevel::Success, "BrickGenerator: ", l_brickFactors.size(), " brick factors have been generated.");

            serializeBrickFactors(l_brickFactors);
            serializeProbes(probes);

            return true;
        }
    }
}