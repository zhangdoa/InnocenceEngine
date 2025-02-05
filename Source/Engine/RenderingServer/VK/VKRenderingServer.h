#pragma once
#include "../IRenderingServer.h"
#include "VKHeaders.h"

#include "../../Common/ObjectPool.h"

#include "../../Component/VKMeshComponent.h"
#include "../../Component/VKTextureComponent.h"
#include "../../Component/VKMaterialComponent.h"
#include "../../Component/VKRenderPassComponent.h"
#include "../../Component/VKShaderProgramComponent.h"
#include "../../Component/VKSamplerComponent.h"
#include "../../Component/VKGPUBufferComponent.h"

namespace Inno
{
	class VKRenderingServer : public IRenderingServer
	{
	public:
		INNO_CLASS_CONCRETE_NON_COPYABLE(VKRenderingServer);

		// Inherited via IRenderingServer
		MeshComponent* AddMeshComponent(const char* name = "") override;
		TextureComponent* AddTextureComponent(const char* name = "") override;
		MaterialComponent* AddMaterialComponent(const char* name = "") override;
		RenderPassComponent* AddRenderPassComponent(const char* name = "") override;
		ShaderProgramComponent* AddShaderProgramComponent(const char* name = "") override;
		SamplerComponent* AddSamplerComponent(const char* name = "") override;
		GPUBufferComponent* AddGPUBufferComponent(const char* name = "") override;
		IPipelineStateObject* AddPipelineStateObject() override;
		ICommandList* AddCommandList() override;
		ISemaphore* AddSemaphore() override;
        bool Add(IOutputMergerTarget*& rhs) override;		

		virtual	bool DeleteMeshComponent(MeshComponent* rhs) override;
		virtual	bool DeleteTextureComponent(TextureComponent* rhs) override;
		virtual	bool DeleteMaterialComponent(MaterialComponent* rhs) override;
		virtual	bool DeleteRenderPassComponent(RenderPassComponent* rhs) override;
		virtual	bool DeleteShaderProgramComponent(ShaderProgramComponent* rhs) override;
		virtual	bool DeleteSamplerComponent(SamplerComponent* rhs) override;
		virtual	bool DeleteGPUBufferComponent(GPUBufferComponent* rhs) override;
		virtual	bool Delete(IPipelineStateObject* rhs) override;
		virtual	bool Delete(ICommandList* rhs) override;
		virtual	bool Delete(ISemaphore* rhs) override;
		virtual bool Delete(IOutputMergerTarget* rhs) override;

		bool ClearTextureComponent(TextureComponent* rhs) override;
		bool CopyTextureComponent(TextureComponent* lhs, TextureComponent* rhs) override;
		bool ClearGPUBufferComponent(GPUBufferComponent* rhs) override;

		bool WaitOnCPU(uint64_t semaphoreValue, GPUEngineType queueType) override;

		uint32_t GetIndex(TextureComponent* rhs, Accessibility bindingAccessibility) override;

		bool CommandListBegin(RenderPassComponent* rhs, size_t frameIndex) override;
		bool BindRenderPassComponent(RenderPassComponent* rhs) override;
		bool ClearRenderTargets(RenderPassComponent* rhs, size_t index = -1) override;
		bool Bind(RenderPassComponent* renderPass, uint32_t rootParameterIndex, const ResourceBindingLayoutDesc& resourceBindingLayoutDesc) override;
		bool BindGPUResource(RenderPassComponent* renderPass, ShaderStage shaderStage, GPUResourceComponent* resource, size_t resourceBindingLayoutDescIndex, size_t startOffset = 0, size_t elementCount = SIZE_MAX) override;
		void PushRootConstants(RenderPassComponent* rhs, size_t rootConstants) override;
		bool DrawIndexedInstanced(RenderPassComponent* renderPass, MeshComponent* mesh, size_t instanceCount) override;
		bool DrawInstanced(RenderPassComponent* renderPass, size_t instanceCount) override;
		bool UnbindGPUResource(RenderPassComponent* renderPass, ShaderStage shaderStage, GPUResourceComponent* resource, size_t resourceBindingLayoutDescIndex, size_t startOffset = 0, size_t elementCount = SIZE_MAX) override;
		bool CommandListEnd(RenderPassComponent* rhs) override;
		bool ExecuteCommandList(RenderPassComponent* rhs, GPUEngineType GPUEngineType) override;
		bool WaitOnGPU(RenderPassComponent* rhs, GPUEngineType queueType, GPUEngineType semaphoreType) override;

		bool TryToTransitState(TextureComponent* rhs, ICommandList* commandList, Accessibility accessibility) override;

		bool Dispatch(RenderPassComponent* renderPass, uint32_t threadGroupX, uint32_t threadGroupY, uint32_t threadGroupZ) override;

		Vec4 ReadRenderTargetSample(RenderPassComponent* rhs, size_t renderTargetIndex, size_t x, size_t y) override;
		std::vector<Vec4> ReadTextureBackToCPU(RenderPassComponent* canvas, TextureComponent* TextureComp) override;
		bool GenerateMipmap(TextureComponent* rhs) override;

		bool BeginCapture() override;
		bool EndCapture() override;

		void* GetVkInstance();
		void* GetVkSurface();

	protected:
		bool InitializeImpl(MeshComponent* rhs) override;
		bool InitializeImpl(TextureComponent* rhs) override;
		bool InitializeImpl(RenderPassComponent* rhs) override;
		bool InitializeImpl(ShaderProgramComponent* rhs) override;
		bool InitializeImpl(SamplerComponent* rhs) override;
		bool InitializeImpl(GPUBufferComponent* rhs) override;

        bool UploadToGPU(ICommandList* commandList, GPUBufferComponent* rhs) override;
		
		bool InitializePool() override;
		bool TerminatePool() override;

		bool CreateHardwareResources() override;
		bool ReleaseHardwareResources() override;
		bool GetSwapChainImages() override;
		bool AssignSwapChainImages() override;

		bool PresentImpl() override;
		bool UpdateFrameIndex() override;

		bool ResizeImpl() override;
		bool PostResize(const TVec2<uint32_t>& screenResolution, RenderPassComponent* rhs) override;

	private:
		template <typename U, typename T>
		bool SetObjectName(U *owner, const T &rhs, VkObjectType objectType, const char *objectTypeSuffix);

		std::vector<const char*> GetRequiredExtensions();
		
		// Global initialization functions
		bool CreateVkInstance();
		bool CreateDebugCallback();

		bool CreatePhysicalDevice();
		bool CreateLogicalDevice();

		bool CreateTextureSamplers();
		bool CreateVertexInputAttributions();
		bool CreateMaterialDescriptorPool();
		bool CreateGlobalCommandPool();
		bool CreateSwapChain();
		bool CreateSyncPrimitives();

		// APIs for Vulkan objects
		VkResult CreateDebugUtilsMessengerEXT(const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkDebugUtilsMessengerEXT *pCallback);
		void DestroyDebugUtilsMessengerEXT(VkDebugUtilsMessengerEXT callback, const VkAllocationCallbacks *pAllocator);
		VkResult SetDebugUtilsObjectNameEXT(const VkDebugUtilsObjectNameInfoEXT *pNameInfo);

		bool CheckValidationLayerSupport(const std::vector<const char*>& validationLayers);
		VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
		VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
		VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
	
		bool CheckDeviceExtensionSupport(const std::vector<const char*>& deviceExtensions);
		QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice physicalDevice, VkSurfaceKHR windowSurface);
		SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice physicalDevice, VkSurfaceKHR windowSurface);
		bool IsDeviceSuitable(VkPhysicalDevice physicalDevice, VkSurfaceKHR windowSurface, const std::vector<const char*>& deviceExtensions);

		bool CreateHostStagingBuffer(size_t bufferSize, VkBufferUsageFlagBits usageFlags, VkBuffer &buffer, VkDeviceMemory &deviceMemory);
		bool CreateDeviceLocalBuffer(size_t bufferSize, VkBufferUsageFlagBits usageFlags, VkBuffer &buffer, VkDeviceMemory &deviceMemory);
		bool CopyHostMemoryToDeviceMemory(void *hostMemory, size_t bufferSize, VkDeviceMemory &deviceMemory);	
		bool InitializeDeviceLocalBuffer(void *hostMemory, size_t bufferSize, VkBuffer &buffer, VkDeviceMemory &deviceMemory);	
		
		VkCommandBuffer OpenTemporaryCommandBuffer(VkCommandPool commandPool);
		void CloseTemporaryCommandBuffer(VkCommandPool commandPool, VkQueue commandQueue, VkCommandBuffer commandBuffer);

		uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

		bool CreateCommandPool(VkSurfaceKHR windowSurface, GPUEngineType GPUEngineType, VkCommandPool &commandPool);
		
		bool TransitImageLayout(VkCommandBuffer commandBuffer, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, VkImageLayout oldLayout, VkImageLayout newLayout, ShaderStage shaderStage = ShaderStage::Invalid);
		bool CreateBuffer(const VkBufferCreateInfo& bufferCInfo, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
		bool CopyBuffer(VkCommandPool commandPool, VkQueue commandQueue, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
		bool CopyBufferToImage(VkCommandBuffer commandBuffer, VkBuffer buffer, VkImage image, VkImageAspectFlags aspectFlags, uint32_t width, uint32_t height);
		bool CreateImage(const VkImageCreateInfo& imageCInfo, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);

		bool CreateDescriptorPool(VkDescriptorPoolSize* poolSize, uint32_t poolSizeCount, uint32_t maxSets, VkDescriptorPool& poolHandle);
		bool CreateDescriptorSetLayout(VkDescriptorSetLayoutBinding* setLayoutBindings, uint32_t setLayoutBindingsCount, VkDescriptorSetLayout& setLayout);
		bool CreateDescriptorSets(VkDescriptorPool pool, const VkDescriptorSetLayout* setLayout, VkDescriptorSet& setHandle, uint32_t count);
		bool UpdateDescriptorSet(VkWriteDescriptorSet* writeDescriptorSets, uint32_t writeDescriptorSetsCount);
		
		VkWriteDescriptorSet GetWriteDescriptorSet(uint32_t dstBinding, VkDescriptorType descriptorType, const VkDescriptorSet& descriptorSet);
		VkWriteDescriptorSet GetWriteDescriptorSet(const VkDescriptorImageInfo& imageInfo, uint32_t dstBinding, VkDescriptorType descriptorType, const VkDescriptorSet& descriptorSet);
		VkWriteDescriptorSet GetWriteDescriptorSet(const VkDescriptorBufferInfo& bufferInfo, uint32_t dstBinding, VkDescriptorType descriptorType, const VkDescriptorSet& descriptorSet);
		
		bool CreateShaderModule(VkShaderModule& vkShaderModule, const ShaderFilePath& shaderFilePath);
		
		// APIs for engine components
		bool CreateImageView(VKTextureComponent* VKTextureComp);

		bool ReserveFramebuffer(VKRenderPassComponent* VKRenderPassComp);
		bool CreateDescriptorSetLayoutBindings(VKRenderPassComponent* VKRenderPassComp);
		bool CreateDescriptorPool(VKRenderPassComponent* VKRenderPassComp);
		bool CreateDescriptorSetLayout(const VkDescriptorSetLayout& dummyEmptyDescriptorLayout, VKRenderPassComponent* VKRenderPassComp);
		bool CreateDescriptorSets(VKRenderPassComponent* VKRenderPassComp);
		bool CreateRenderPass(VKRenderPassComponent* VKRenderPassComp, VkFormat* overrideFormat = nullptr);
		bool CreateViewportAndScissor(VKRenderPassComponent* VKRenderPassComp);
		bool CreateSingleFramebuffer(VKRenderPassComponent* VKRenderPassComp);
		bool CreateFramebuffers(VKRenderPassComponent* VKRenderPassComp);
		bool CreatePipelineLayout(VKRenderPassComponent* VKRenderPassComp);
		bool CreateGraphicsPipelines(VKRenderPassComponent* VKRenderPassComp);
		bool CreateComputePipelines(VKRenderPassComponent* VKRenderPassComp);
		bool CreateCommandBuffers(VKRenderPassComponent* VKRenderPassComp);
		bool CreateSyncPrimitives(VKRenderPassComponent* VKRenderPassComp);

		bool GenerateViewportState(ViewportDesc viewportDesc, VKPipelineStateObject* PSO);
		bool GenerateRasterizerState(RasterizerDesc rasterizerDesc, VKPipelineStateObject* PSO);
		bool GenerateDepthStencilState(DepthStencilDesc depthStencilDesc, VKPipelineStateObject* PSO);
		bool GenerateBlendState(BlendDesc blendDesc, size_t RTCount, VKPipelineStateObject* PSO);

		// Vulkan objects
		VkInstance m_instance;
		VkSurfaceKHR m_windowSurface;
		std::vector<VkImage> m_swapChainImages;
		VkQueue m_presentQueue;
		VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
		VkDevice m_device;
		VkCommandPool m_globalCommandPool;
		VkQueue m_graphicsQueue;
		VkFence m_graphicsQueueFence;
		VkQueue m_computeQueue;
		VkFence m_computeQueueFence;
		std::atomic<uint64_t> m_graphicCommandQueueSemaphore = 0;
		std::atomic<uint64_t> m_computeCommandQueueSemaphore = 0;
		std::vector<VkSemaphore> m_imageAvailableSemaphores;
		std::vector<VkSemaphore> m_swapChainRenderedSemaphores;
		VkSwapchainKHR m_swapChain = 0;
		VkExtent2D m_presentSurfaceExtent = {};
		VkFormat m_presentSurfaceFormat = {};

		const std::vector<const char*> m_deviceExtensions =
		{
			VK_KHR_SWAPCHAIN_EXTENSION_NAME,
			VK_KHR_TIMELINE_SEMAPHORE_EXTENSION_NAME,
			VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME,
			VK_KHR_MAINTENANCE2_EXTENSION_NAME,		 // For imageless framebuffer
			VK_KHR_IMAGE_FORMAT_LIST_EXTENSION_NAME, // For imageless framebuffer
			VK_KHR_IMAGELESS_FRAMEBUFFER_EXTENSION_NAME,
			VK_EXT_CUSTOM_BORDER_COLOR_EXTENSION_NAME 
		};

#ifdef INNO_DEBUG
		const bool m_enableValidationLayers = true;
#else
		const bool m_enableValidationLayers = false;
#endif

		const std::vector<const char*> m_validationLayers =
		{
			"VK_LAYER_KHRONOS_validation" 
		};

		VkDebugUtilsMessengerEXT m_messengerCallback;

		VkVertexInputBindingDescription m_vertexBindingDescription;
		std::array<VkVertexInputAttributeDescription, 6> m_vertexAttributeDescriptions;

		VkDescriptorPool m_materialDescriptorPool;
		VkDescriptorSetLayout m_materialDescriptorLayout;
		VkDescriptorSetLayout m_dummyEmptyDescriptorLayout;

		// Component pools
		TObjectPool<VKMeshComponent>* m_MeshComponentPool = nullptr;
		TObjectPool<VKMaterialComponent>* m_MaterialComponentPool = nullptr;
		TObjectPool<VKTextureComponent>* m_TextureComponentPool = nullptr;
		TObjectPool<VKRenderPassComponent>* m_RenderPassComponentPool = nullptr;
		TObjectPool<VKPipelineStateObject>* m_PSOPool = nullptr;
		TObjectPool<VKCommandList>* m_CommandListPool = nullptr;
		TObjectPool<VKSemaphore>* m_SemaphorePool = nullptr;
		TObjectPool<VKShaderProgramComponent>* m_ShaderProgramComponentPool = nullptr;
		TObjectPool<VKSamplerComponent>* m_SamplerComponentPool = nullptr;
		TObjectPool<VKGPUBufferComponent>* m_GPUBufferComponentPool = nullptr;
	};
}