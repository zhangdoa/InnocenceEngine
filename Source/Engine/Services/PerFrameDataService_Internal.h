#pragma once
#include "PerFrameDataService.h"

#include "../Common/Array.h"
#include "../Component/GPUBufferComponent.h"
#include "../Common/GPUDataStructure.h"

#include <atomic>
#include <shared_mutex>

namespace Inno
{
    struct PerFrameDataServiceImpl
    {
        ObjectStatus m_ObjectStatus = ObjectStatus::Terminated;

        mutable std::shared_mutex m_Mutex;

        Inno::Array<PerFrameConstantBuffer> m_perFrameCBs;

        GPUBufferComponent* m_PerFrameCBufferGPUBufferComp;
        GPUBufferComponent* m_PerFrameCBufferPrevGPUBufferComp;
        std::atomic<uint32_t> m_DebugViewMode{ static_cast<uint32_t>(DebugViewMode::None) };
        std::atomic<uint32_t> m_PointShadowBypass{ 0u };


        bool Setup(IServiceConfig* systemConfig);
        bool Initialize();
        bool Update();
        bool Terminate();

        float RadicalInverse(uint32_t n, uint32_t base);
        bool UpdatePerFrameConstantBuffer();

        GPUBufferComponent* GetCurrentFramePerFrameBuffer();
        GPUBufferComponent* GetPreviousFramePerFrameBuffer();
    };
}
