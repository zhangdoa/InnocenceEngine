#include "VKGraphicsService.h"

#include "../../Engine.h"

using namespace Inno;

#include "VKHelper_Common.h"
using namespace VKHelper;

#include "../../Common/LogService.h"
#include "../../Common/Memory.h"
#include "../../Common/Randomizer.h"
#include "../../Common/ObjectPool.h"

#include "../../Services/RenderingConfigurationService.h"
#include "../../Services/TemplateAssetService.h"

bool VKGraphicsService::DrawIndexedInstanced(RenderPassComponent* renderPass, CommandListComponent* commandList, MeshComponent* mesh, size_t instanceCount)
{
	auto l_renderPass = reinterpret_cast<VKRenderPassComponent*>(renderPass);
	auto l_vkCommandBuffer = reinterpret_cast<VkCommandBuffer>(commandList->m_CommandList);
	auto l_mesh = reinterpret_cast<VKMeshComponent *>(mesh);

	VkBuffer vertexBuffers[] = {l_mesh->m_VBO};
	VkDeviceSize offsets[] = {0};

	vkCmdBindVertexBuffers(l_vkCommandBuffer, 0, 1, vertexBuffers, offsets);
	vkCmdBindIndexBuffer(l_vkCommandBuffer, l_mesh->m_IBO, 0, VK_INDEX_TYPE_UINT32);
	//vkCmdDrawIndexed(l_vkCommandBuffer, static_cast<uint32_t>(l_mesh->m_IndexCount), 1, 0, 0, 0);

	return true;
}

bool VKGraphicsService::DrawInstanced(RenderPassComponent* renderPass, CommandListComponent* commandList, size_t instanceCount)
{
	auto l_renderPass = reinterpret_cast<VKRenderPassComponent*>(renderPass);
	auto l_vkCommandBuffer = reinterpret_cast<VkCommandBuffer>(commandList->m_CommandList);

	vkCmdDraw(l_vkCommandBuffer, 1, static_cast<uint32_t>(instanceCount), 0, 0);

	return true;
}

bool VKGraphicsService::ExecuteIndirect(RenderPassComponent* renderPass, CommandListComponent* commandList, GPUBufferComponent* indirectDrawCommand)
{
	// Vulkan ExecuteIndirect not implemented yet
	// TODO: Implement vkCmdDrawIndirect functionality
	return true;
}

bool VKGraphicsService::Dispatch(RenderPassComponent* renderPass, CommandListComponent* commandList, uint32_t threadGroupX, uint32_t threadGroupY, uint32_t threadGroupZ)
{
	auto l_vkCommandBuffer = reinterpret_cast<VkCommandBuffer>(commandList->m_CommandList);

	vkCmdDispatch(l_vkCommandBuffer, threadGroupX, threadGroupY, threadGroupZ);

	return true;
}
