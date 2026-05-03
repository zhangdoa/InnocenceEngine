#pragma once
#include "../Interface/IService.h"
#include "../Common/NamedObjectPool.h"
#include "../Common/ThreadSafeQueue.h"
#include "../Component/TextureComponent.h"
#include "../Component/RenderPassComponent.h"
#include "../Common/EntityID.h"
#include "../Common/Math.h"

namespace Inno
{
	class CommandListComponent;

	class TextureResourceService : public IService
	{
	public:
		INNO_CLASS_INTERFACE_NON_COPYABLE(TextureResourceService);

		bool Setup(IServiceConfig* systemConfig = nullptr) override;
		bool Initialize() override { return true; }
		bool Update() override { return true; }
		bool Terminate() override;
		ObjectStatus GetStatus() override { return m_ObjectStatus; }

		TextureComponent* Add(const char* name);
		virtual bool Delete(TextureComponent* ptr);
		TextureComponent* Find(const char* name);

		void Initialize(TextureComponent* texture, void* textureData = nullptr, EntityID owner = INVALID_ENTITY);
		bool InitializeSynchronous(TextureComponent* texture, void* textureData);
		bool InitializeComponents();
		bool OnSceneUnloading();

		// Returns the component at the head of the deferred-init queue (the
		// queue is THE source of truth for "pending init work"), or nullptr if
		// the queue is empty. Per no-shadow-state discipline (a86e6e93): the
		// queue's contents ARE the pending-work data, not a shadow of it.
		// Replaces CL 3's pool-iteration shape, which conflated pending-init
		// with scaffolding components (e.g. RayTracingResult texture) whose
		// ObjectStatus is intentionally frozen at Created.
		// Caveat: TextureResourceService also enqueues async binary-load
		// requests via EnqueueBinaryLoad / s_BinaryLoadQueue (file-side, not
		// m_DeferredQueue). A texture pending a binary decode that has not
		// yet landed in m_DeferredQueue will not be observed here. The
		// CL 4 residency aggregator must consult both queues for textures.
		TextureComponent* GetFirstPendingComponent() const;

		// TASK-213 CL 3.5: deferred-init queue empty signal. Read-only;
		// mirror of GetFirstPendingComponent() == nullptr but cheaper for the
		// CL 4 aggregator's bool-check fast path. Does NOT account for the
		// async s_BinaryLoadQueue (see GetFirstPendingComponent caveat).
		bool IsDeferredQueueEmpty() const { return m_DeferredQueue.empty(); }

		// Enqueue a texture binary decode + GPU-init on the dedicated background loader thread.
		// Safe to call from any thread; returns immediately without blocking.
		void EnqueueBinaryLoad(const std::string& binaryPath, TextureComponent* component, EntityID owner);

		virtual bool Clear(CommandListComponent* commandList, TextureComponent* texture) { return false; }
		virtual bool Copy(CommandListComponent* commandList, TextureComponent* src, TextureComponent* dst) { return false; }
		virtual bool GenerateMipmap(TextureComponent* texture, CommandListComponent* commandList = nullptr) { return false; }
		virtual std::optional<uint32_t> GetIndex(TextureComponent* texture, Accessibility bindingAccessibility) { return std::nullopt; }
		virtual std::vector<Math::Vec4> ReadTextureBackToCPU(RenderPassComponent* canvas, TextureComponent* textureComp) { return {}; }

	protected:
		virtual bool InitializeImpl(TextureComponent* texture, void* textureData) { return false; }

		NamedObjectPool<TextureComponent> m_Pool;
		ObjectStatus m_ObjectStatus = ObjectStatus::Invalid;

	private:
		struct TextureInitTask
		{
			TextureInitTask(TextureComponent* component, void* textureData, EntityID owner = INVALID_ENTITY)
				: m_Component(component), m_TextureData(textureData), m_Owner(owner) {}

			TextureComponent* m_Component;
			void* m_TextureData;
			EntityID m_Owner;
		};

		ThreadSafeQueue<TextureInitTask> m_DeferredQueue;
	};
}
