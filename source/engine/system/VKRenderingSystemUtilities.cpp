#pragma once
#include "VKRenderingSystemUtilities.h"
#include "../component/VKRenderingSystemComponent.h"

#include "ICoreSystem.h"

extern ICoreSystem* g_pCoreSystem;

INNO_PRIVATE_SCOPE VKRenderingSystemNS
{
	bool createShaderModule(VkShaderModule& vkShaderModule, const std::string& shaderFilePath);
}

VKRenderPassComponent* VKRenderingSystemNS::addVKRenderPassComponent(unsigned int RTNum, TextureDataDesc RTDesc)
{
	return nullptr;
}

VKMeshDataComponent * VKRenderingSystemNS::generateVKMeshDataComponent(MeshDataComponent * rhs)
{
	return nullptr;
}

VKTextureDataComponent * VKRenderingSystemNS::generateVKTextureDataComponent(TextureDataComponent * rhs)
{
	return nullptr;
}

VKMeshDataComponent * VKRenderingSystemNS::addVKMeshDataComponent(EntityID rhs)
{
	return nullptr;
}

VKTextureDataComponent * VKRenderingSystemNS::addVKTextureDataComponent(EntityID rhs)
{
	return nullptr;
}

VKMeshDataComponent * VKRenderingSystemNS::getVKMeshDataComponent(EntityID rhs)
{
	return nullptr;
}

VKTextureDataComponent * VKRenderingSystemNS::getVKTextureDataComponent(EntityID rhs)
{
	return nullptr;
}

void VKRenderingSystemNS::drawMesh(EntityID rhs)
{
}

void VKRenderingSystemNS::drawMesh(MeshDataComponent * MDC)
{
}

void VKRenderingSystemNS::drawMesh(size_t indicesSize, VKMeshDataComponent * VKMDC)
{
}

bool VKRenderingSystemNS::createShaderModule(VkShaderModule& vkShaderModule, const std::string& shaderFilePath)
{
	auto l_binData = g_pCoreSystem->getFileSystem()->loadBinaryFile(shaderFilePath);

	VkShaderModuleCreateInfo l_createInfo = {};
	l_createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	l_createInfo.codeSize = l_binData.size();
	l_createInfo.pCode = reinterpret_cast<const uint32_t*>(l_binData.data());

	if (vkCreateShaderModule(VKRenderingSystemComponent::get().m_device, &l_createInfo, nullptr, &vkShaderModule) != VK_SUCCESS)
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "VKRenderingSystem: Failed to create shader module for: " + shaderFilePath + "!");
		return false;
	}

	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "VKRenderingSystem: innoShader: " + shaderFilePath + " has been loaded.");
	return true;
}

bool VKRenderingSystemNS::initializeVKShaderProgramComponent(VKShaderProgramComponent * rhs, const ShaderFilePaths & shaderFilePaths)
{
	bool l_result = true;
	if (shaderFilePaths.m_VSPath != "")
	{
		l_result = l_result && createShaderModule(rhs->m_vertexShaderModule, shaderFilePaths.m_VSPath);
	}
	if (shaderFilePaths.m_FSPath != "")
	{
		l_result = l_result && createShaderModule(rhs->m_fragmentShaderModule, shaderFilePaths.m_FSPath);
	}

	return l_result;
}

bool VKRenderingSystemNS::activateVKShaderProgramComponent(VKShaderProgramComponent * rhs)
{
	return false;
}
