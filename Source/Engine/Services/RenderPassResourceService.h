#pragma once
#include "../Interface/IService.h"
#include "../Common/NamedObjectPool.h"
#include "../Common/ThreadSafeQueue.h"
#include "../Component/RenderPassComponent.h"
#include "../Common/Math.h"

namespace Inno
{
	class TextureResourceService;

	class RenderPassResourceService : public IService
	{
	public:
		INNO_CLASS_INTERFACE_NON_COPYABLE(RenderPassResourceService);

		bool Setup(IServiceConfig* systemConfig = nullptr) override;
		bool Initialize() override { return true; }
		bool Update() override { return true; }
		bool Terminate() override;
		ObjectStatus GetStatus() override { return m_ObjectStatus; }

		RenderPassComponent* Add(const char* name);
		RenderPassComponent* Find(const char* name);
		virtual bool Delete(RenderPassComponent* ptr);

		void ForEach(std::function<void(RenderPassComponent*)> func);

		void Initialize(RenderPassComponent* renderPass);
		bool InitializeComponents();

		// Residency predicate (no-shadow-state discipline): returns the first
		// component in m_Pool whose status is still ObjectStatus::Created
		// (pre-activation), or nullptr if every live component is Activated.
		// Source of truth is the per-component m_ObjectStatus stamped by
		// InitializeComponents(); no shadow counters.
		RenderPassComponent* GetFirstPendingComponent() const;

		bool InitializeRenderPass(RenderPassComponent* renderPass);
		bool CreateOutputMergerTargets(RenderPassComponent* renderPass);
		bool InitializeOutputMergerTargets(RenderPassComponent* renderPass);
		bool DeleteRenderTargets(RenderPassComponent* renderPass);

		virtual IPipelineStateObject* AddPipelineStateObject() = 0;
		virtual ISemaphore* AddSemaphore() = 0;
		virtual bool Add(IOutputMergerTarget*& rhs) = 0;
		virtual bool Delete(IPipelineStateObject* rhs) = 0;
		virtual bool Delete(ISemaphore* rhs) = 0;
		virtual bool Delete(IOutputMergerTarget* rhs) = 0;

		virtual Math::Vec4 ReadRenderTargetSample(RenderPassComponent* renderPass, size_t renderTargetIndex, size_t x, size_t y) { return Math::Vec4(); }

		virtual bool OnOutputMergerTargetsCreated(RenderPassComponent* renderPass) { return false; }
		virtual bool CreatePipelineStateObject(RenderPassComponent* renderPass) { return false; }

	protected:
		virtual bool CreateFenceEvents(RenderPassComponent* renderPass) { return false; }

		NamedObjectPool<RenderPassComponent> m_Pool;
		ObjectStatus m_ObjectStatus = ObjectStatus::Invalid;

	private:
		ThreadSafeQueue<RenderPassComponent*> m_DeferredQueue;
	};
}
