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

		// Residency predicate (no-shadow-state discipline): returns the first
		// component in m_Pool whose status is still ObjectStatus::Created
		// (pre-activation), or nullptr if every live component is Activated.
		// Source of truth is the per-component m_ObjectStatus stamped by
		// InitializeComponents() / InitializeSynchronous(); no shadow flags.
		TextureComponent* GetFirstPendingComponent() const;

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
