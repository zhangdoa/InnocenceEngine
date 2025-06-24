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
		IPipelineStateObject* AddPipelineStateObject() override;
		ISemaphore* AddSemaphore() override;
        bool Add(IOutputMergerTarget*& rhs) override;		

		virtual	bool Delete(MeshComponent* mesh) override;
		virtual	bool Delete(TextureComponent* texture) override;
		virtual	bool Delete(MaterialComponent* material) override;
		virtual	bool Delete(RenderPassComponent* renderPass) override;
		virtual	bool Delete(ShaderProgramComponent* shaderProgram) override;
		virtual	bool Delete(SamplerComponent* sampler) override;
		virtual	bool Delete(GPUBufferComponent* gpuBuffer) override;
		virtual	bool Delete(IPipelineStateObject* rhs) override;
		virtual	bool Delete(CommandListComponent* rhs) override;
		virtual	bool Delete(ISemaphore* rhs) override;
		virtual bool Delete(IOutputMergerTarget* rhs) override;

		bool CommandListBegin(RenderPassComponent* renderPass, CommandListComponent* commandList, size_t frameIndex) override;
		bool BindRenderPassComponent(RenderPassComponent* renderPass, CommandListComponent* commandList) override;
		bool ClearRenderTargets(RenderPassComponent* renderPass, CommandListComponent* commandList, size_t index = -1) override;
		bool BindGPUResource(RenderPassComponent* renderPass, CommandListComponent* commandList, ShaderStage shaderStage, GPUResourceComponent* resource, size_t resourceBindingLayoutDescIndex, size_t startOffset = 0, size_t elementCount = SIZE_MAX) override;
		bool TryToTransitState(TextureComponent* texture, CommandListComponent* commandList, Accessibility sourceAccessibility, Accessibility targetAccessibility) override;
		bool TryToTransitState(GPUBufferComponent* gpuBuffer, CommandListComponent* commandList, Accessibility sourceAccessibility, Accessibility targetAccessibility) override;
		
		bool ExecuteIndirect(RenderPassComponent* renderPass, CommandListComponent* commandList, GPUBufferComponent* indirectDrawCommand) override;
		void PushRootConstants(RenderPassComponent* renderPass, CommandListComponent* commandList, size_t rootConstants) override;
		bool DrawIndexedInstanced(RenderPassComponent* renderPass, CommandListComponent* commandList, MeshComponent* mesh, size_t instanceCount = 1) override;
		bool DrawInstanced(RenderPassComponent* renderPass, CommandListComponent* commandList, size_t instanceCount = 1) override;
		
		bool CommandListEnd(RenderPassComponent* renderPass, CommandListComponent* commandList) override;
		bool UnbindGPUResource(RenderPassComponent* renderPass, CommandListComponent* commandList, ShaderStage shaderStage, GPUResourceComponent* resource, size_t resourceBindingLayoutDescIndex, size_t startOffset = 0, size_t elementCount = SIZE_MAX) override;
		bool Execute(CommandListComponent* commandList, GPUEngineType GPUEngineType) override;
		bool Dispatch(RenderPassComponent* renderPass, CommandListComponent* commandList, uint32_t threadGroupX, uint32_t threadGroupY, uint32_t threadGroupZ) override;
		
		bool WaitOnGPU(RenderPassComponent* renderPass, GPUEngineType queueType, GPUEngineType semaphoreType) override;
		bool WaitOnCPU(uint64_t semaphoreValue, GPUEngineType queueType) override;

		std::optional<uint32_t> GetIndex(TextureComponent* texture, Accessibility bindingAccessibility) override;
		Vec4 ReadRenderTargetSample(RenderPassComponent* renderPass, size_t renderTargetIndex, size_t x, size_t y) override;
		std::vector<Vec4> ReadTextureBackToCPU(RenderPassComponent* canvas, TextureComponent* TextureComp) override;
		bool GenerateMipmap(TextureComponent* texture, CommandListComponent* commandList = nullptr) override;

		bool BeginCapture() override;
		bool EndCapture() override;

		void* GetVkInstance();
		void* GetVkSurface();

	protected:
		bool InitializeImpl(MeshComponent* mesh, std::vector<Vertex>& vertices, std::vector<Index>& indices) override;
		bool InitializeImpl(TextureComponent* texture, void* textureData) override;
		bool InitializeImpl(RenderPassComponent* renderPass) override;
		bool InitializeImpl(ShaderProgramComponent* shaderProgram) override;
		bool InitializeImpl(SamplerComponent* sampler) override;
		bool InitializeImpl(GPUBufferComponent* gpuBuffer) override;

        bool UploadToGPU(CommandListComponent* commandList, GPUBufferComponent* gpuBuffer) override;
		
		bool InitializePool() override;
		bool TerminatePool() override;

		bool CreateHardwareResources() override;
		bool ReleaseHardwareResources() override;
		bool GetSwapChainImages() override;
		bool AssignSwapChainImages() override;
		bool ReleaseSwapChainImages() override;

		bool PresentImpl() override;
		bool EndFrame() override;

		bool ResizeImpl() override;

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

#if defined(INNO_DEBUG) || defined(INNO_RELWITHDEBINFO)
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
		TObjectPool<VKPipelineStateObject>* m_PSOPool = nullptr;
		TObjectPool<VKSemaphore>* m_SemaphorePool = nullptr;
	};
}