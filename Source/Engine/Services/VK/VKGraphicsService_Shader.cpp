#include "VKGraphicsService.h"
#include "../GraphicsResourceService.h"

#include "../../Engine.h"

using namespace Inno;

#include "VKHelper_Common.h"
#include "VKHelper_Texture.h"
#include "VKHelper_Pipeline.h"
using namespace VKHelper;

#include "../../Common/LogService.h"
#include "../../Common/Memory.h"
#include "../../Common/Randomizer.h"
#include "../../Common/ObjectPool.h"

#include "../../Services/RenderingConfigurationService.h"
#include "../../Services/TemplateAssetService.h"

bool VKGraphicsService::InitializeImpl(ShaderProgramComponent *rhs)
{
	auto l_rhs = reinterpret_cast<VKShaderProgramComponent *>(rhs);

	bool l_result = true;

	l_rhs->m_vertexInputStateCInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	l_rhs->m_vertexInputStateCInfo.vertexBindingDescriptionCount = 1;
	l_rhs->m_vertexInputStateCInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(m_vertexAttributeDescriptions.size());
	l_rhs->m_vertexInputStateCInfo.pVertexBindingDescriptions = &m_vertexBindingDescription;
	l_rhs->m_vertexInputStateCInfo.pVertexAttributeDescriptions = m_vertexAttributeDescriptions.data();

	if (l_rhs->m_ShaderFilePaths.m_VSPath != "")
	{
		l_result &= CreateShaderModule(l_rhs->m_VSHandle, l_rhs->m_ShaderFilePaths.m_VSPath);
		l_rhs->m_VSCInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		l_rhs->m_VSCInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		l_rhs->m_VSCInfo.module = l_rhs->m_VSHandle;
		l_rhs->m_VSCInfo.pName = "main";
	}
	if (l_rhs->m_ShaderFilePaths.m_HSPath != "")
	{
		l_result &= CreateShaderModule(l_rhs->m_HSHandle, l_rhs->m_ShaderFilePaths.m_HSPath);
		l_rhs->m_HSCInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		l_rhs->m_HSCInfo.stage = VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
		l_rhs->m_HSCInfo.module = l_rhs->m_HSHandle;
		l_rhs->m_HSCInfo.pName = "main";
	}
	if (l_rhs->m_ShaderFilePaths.m_DSPath != "")
	{
		l_result &= CreateShaderModule(l_rhs->m_DSHandle, l_rhs->m_ShaderFilePaths.m_DSPath);
		l_rhs->m_DSCInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		l_rhs->m_DSCInfo.stage = VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
		l_rhs->m_DSCInfo.module = l_rhs->m_DSHandle;
		l_rhs->m_DSCInfo.pName = "main";
	}
	if (l_rhs->m_ShaderFilePaths.m_GSPath != "")
	{
		l_result &= CreateShaderModule(l_rhs->m_GSHandle, l_rhs->m_ShaderFilePaths.m_GSPath);
		l_rhs->m_GSCInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		l_rhs->m_GSCInfo.stage = VK_SHADER_STAGE_GEOMETRY_BIT;
		l_rhs->m_GSCInfo.module = l_rhs->m_GSHandle;
		l_rhs->m_GSCInfo.pName = "main";
	}
	if (l_rhs->m_ShaderFilePaths.m_PSPath != "")
	{
		l_result &= CreateShaderModule(l_rhs->m_PSHandle, l_rhs->m_ShaderFilePaths.m_PSPath);
		l_rhs->m_PSCInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		l_rhs->m_PSCInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		l_rhs->m_PSCInfo.module = l_rhs->m_PSHandle;
		l_rhs->m_PSCInfo.pName = "main";
	}
	if (l_rhs->m_ShaderFilePaths.m_CSPath != "")
	{
		l_result &= CreateShaderModule(l_rhs->m_CSHandle, l_rhs->m_ShaderFilePaths.m_CSPath);
		l_rhs->m_CSCInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		l_rhs->m_CSCInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
		l_rhs->m_CSCInfo.module = l_rhs->m_CSHandle;
		l_rhs->m_CSCInfo.pName = "main";
	}

	l_rhs->m_ObjectStatus = ObjectStatus::Activated;

	return l_result;
}
