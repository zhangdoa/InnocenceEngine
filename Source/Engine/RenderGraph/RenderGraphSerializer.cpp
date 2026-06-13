#include "RenderGraphSerializer.h"
#include "RenderGraphEnumStrings.h"

using namespace Inno;
namespace ES = Inno::RenderGraphEnumStrings;

namespace
{
	json TextureDescToJson(const TextureDesc& d)
	{
		return json{
			{ "Width", d.Width }, { "Height", d.Height }, { "DepthOrArraySize", d.DepthOrArraySize },
			{ "MipLevels", d.MipLevels },
			{ "Sampler", ES::ToString(d.Sampler) }, { "Usage", ES::ToString(d.Usage) },
			{ "PixelDataType", ES::ToString(d.PixelDataType) }, { "PixelDataFormat", ES::ToString(d.PixelDataFormat) },
			{ "GPUAccessibility", ES::ToString(d.GPUAccessibility) } };
	}

	void TextureDescFromJson(const json& j, TextureDesc& d)
	{
		d.Width = j.value("Width", 0u);
		d.Height = j.value("Height", 0u);
		d.DepthOrArraySize = j.value("DepthOrArraySize", 1u);
		d.MipLevels = j.value("MipLevels", 1u);
		d.Sampler = ES::TextureSamplerFromString(j.value("Sampler", std::string("Invalid")));
		d.Usage = ES::TextureUsageFromString(j.value("Usage", std::string("Invalid")));
		d.PixelDataType = ES::TexturePixelDataTypeFromString(j.value("PixelDataType", std::string("Invalid")));
		d.PixelDataFormat = ES::TexturePixelDataFormatFromString(j.value("PixelDataFormat", std::string("Invalid")));
		d.GPUAccessibility = ES::AccessibilityFromString(j.value("GPUAccessibility", std::string("ReadOnly")));
	}

	json BufferDescToJson(const BufferDesc& d)
	{
		return json{
			{ "ElementCount", d.m_ElementCount }, { "ElementSize", d.m_ElementSize },
			{ "BufferUsage", ES::ToString(d.m_Usage) },
			{ "CPUAccessibility", ES::ToString(d.m_CPUAccessibility) },
			{ "GPUAccessibility", ES::ToString(d.m_GPUAccessibility) } };
	}

	void BufferDescFromJson(const json& j, BufferDesc& d)
	{
		d.m_ElementCount = j.value("ElementCount", size_t(0));
		d.m_ElementSize = j.value("ElementSize", size_t(0));
		d.m_Usage = ES::GPUBufferUsageFromString(j.value("BufferUsage", std::string("Generic")));
		d.m_CPUAccessibility = ES::AccessibilityFromString(j.value("CPUAccessibility", std::string("Immutable")));
		d.m_GPUAccessibility = ES::AccessibilityFromString(j.value("GPUAccessibility", std::string("ReadWrite")));
	}

	json ResourceToJson(const ResourceDesc& r)
	{
		json j = (r.m_Type == RenderGraphResourceType::Buffer)
			? BufferDescToJson(r.m_BufferDesc)
			: TextureDescToJson(r.m_TextureDesc);
		j["Name"] = r.m_Name;
		j["Type"] = ES::ToString(r.m_Type);
		j["Lifetime"] = ES::ToString(r.m_Lifetime);
		if (r.m_Imported)
			j["Imported"] = true;
		if (!r.m_SizeExpr.empty())
			j["Size"] = r.m_SizeExpr;
		if (r.m_PingPong)
			j["PingPong"] = true;
		return j;
	}

	void ResourceFromJson(const json& j, ResourceDesc& r)
	{
		r.m_Name = j.value("Name", std::string());
		r.m_Type = ES::ResourceTypeFromString(j.value("Type", std::string("Texture")));
		r.m_Lifetime = ES::LifetimeFromString(j.value("Lifetime", std::string("Persistent")));
		r.m_Imported = j.value("Imported", false);
		r.m_SizeExpr = j.value("Size", std::string());
		r.m_PingPong = j.value("PingPong", false);
		if (r.m_Type == RenderGraphResourceType::Buffer)
			BufferDescFromJson(j, r.m_BufferDesc);
		else
			TextureDescFromJson(j, r.m_TextureDesc);
	}

	json BindingToJson(const BindingDesc& b)
	{
		json j = json{
			{ "Resource", b.m_Resource }, { "Type", ES::ToString(b.m_GPUResourceType) },
			{ "Set", b.m_DescriptorSetIndex }, { "Index", b.m_DescriptorIndex },
			{ "BindingAccess", ES::ToString(b.m_BindingAccessibility) },
			{ "ResourceAccess", ES::ToString(b.m_ResourceAccessibility) },
			{ "TextureUsage", ES::ToString(b.m_TextureUsage) }, { "Stage", ES::ToString(b.m_ShaderStage) } };
		if (b.m_PingPongHistory)
			j["PingPongHistory"] = true;
		if (b.m_IsRootConstant)
			j["RootConstant"] = true;
		if (b.m_SubresourceCount != 1)
			j["SubresourceCount"] = b.m_SubresourceCount;
		if (b.m_GPUBufferUsage != GPUBufferUsage::Generic)
			j["BufferUsage"] = ES::ToString(b.m_GPUBufferUsage);
		return j;
	}

	void BindingFromJson(const json& j, BindingDesc& b)
	{
		b.m_Resource = j.value("Resource", std::string());
		b.m_GPUResourceType = ES::GPUResourceTypeFromString(j.value("Type", std::string("Image")));
		b.m_DescriptorSetIndex = j.value("Set", 0u);
		b.m_DescriptorIndex = j.value("Index", 0u);
		b.m_BindingAccessibility = ES::AccessibilityFromString(j.value("BindingAccess", std::string("ReadOnly")));
		b.m_ResourceAccessibility = ES::AccessibilityFromString(j.value("ResourceAccess", std::string("ReadOnly")));
		b.m_TextureUsage = ES::TextureUsageFromString(j.value("TextureUsage", std::string("Invalid")));
		b.m_ShaderStage = ES::ShaderStageFromString(j.value("Stage", std::string("Invalid")));
		b.m_PingPongHistory = j.value("PingPongHistory", false);
		b.m_IsRootConstant = j.value("RootConstant", false);
		b.m_SubresourceCount = j.value("SubresourceCount", 1u);
		b.m_GPUBufferUsage = ES::GPUBufferUsageFromString(j.value("BufferUsage", std::string("Generic")));
	}

	json TransitionToJson(const TransitionDesc& t)
	{
		json j = json{
			{ "Resource", t.m_Resource },
			{ "From", ES::ToString(t.m_From) },
			{ "To", ES::ToString(t.m_To) } };
		if (t.m_PingPongHistory)
			j["PingPongHistory"] = true;
		return j;
	}

	void TransitionFromJson(const json& j, TransitionDesc& t)
	{
		t.m_Resource = j.value("Resource", std::string());
		t.m_From = ES::AccessibilityFromString(j.value("From", std::string("WriteOnly")));
		t.m_To = ES::AccessibilityFromString(j.value("To", std::string("ReadOnly")));
		t.m_PingPongHistory = j.value("PingPongHistory", false);
	}

	json PassToJson(const PassNodeDesc& p)
	{
		json reads = json::array();
		for (const auto& r : p.m_Reads) reads.push_back(r);
		json writes = json::array();
		for (const auto& w : p.m_Writes) writes.push_back(w);
		json bindings = json::array();
		for (const auto& b : p.m_Bindings) bindings.push_back(BindingToJson(b));
		json shader = json::object();
		if (p.m_ShaderFilePaths.m_VSPath.c_str()[0]) shader["VS"] = p.m_ShaderFilePaths.m_VSPath.c_str();
		if (p.m_ShaderFilePaths.m_PSPath.c_str()[0]) shader["PS"] = p.m_ShaderFilePaths.m_PSPath.c_str();
		if (p.m_ShaderFilePaths.m_CSPath.c_str()[0]) shader["CS"] = p.m_ShaderFilePaths.m_CSPath.c_str();
		if (p.m_ShaderFilePaths.m_RayGenPath.c_str()[0]) shader["RayGen"] = p.m_ShaderFilePaths.m_RayGenPath.c_str();
		if (p.m_ShaderFilePaths.m_AnyHitPath.c_str()[0]) shader["AnyHit"] = p.m_ShaderFilePaths.m_AnyHitPath.c_str();
		if (p.m_ShaderFilePaths.m_ClosestHitPath.c_str()[0]) shader["ClosestHit"] = p.m_ShaderFilePaths.m_ClosestHitPath.c_str();
		if (p.m_ShaderFilePaths.m_MissPath.c_str()[0]) shader["Miss"] = p.m_ShaderFilePaths.m_MissPath.c_str();
		if (p.m_ShaderFilePaths.m_ShadowMissPath.c_str()[0]) shader["ShadowMiss"] = p.m_ShaderFilePaths.m_ShadowMissPath.c_str();


		json j = json{
			{ "Name", p.m_Name }, { "Queue", ES::ToString(p.m_Queue) },
			{ "Shader", shader },
			{ "Reads", reads }, { "Writes", writes }, { "Bindings", bindings },
			{ "Dispatch", { { "Mode", ES::ToString(p.m_Dispatch.m_Mode) },
				{ "X", p.m_Dispatch.m_X }, { "Y", p.m_Dispatch.m_Y }, { "Z", p.m_Dispatch.m_Z },
				{ "TileSize", p.m_Dispatch.m_TileSize } } },
			{ "Bypass", { { "Enabled", p.m_BypassEnabled }, { "ClearOnBypass", p.m_ClearOnBypass } } },
			{ "OneShot", p.m_OneShot } };
		if (p.m_TrackWriteState)
			j["TrackWriteState"] = true;
		if (!p.m_Transitions.empty())
		{
			json transitions = json::array();
			for (const auto& t : p.m_Transitions) transitions.push_back(TransitionToJson(t));
			j["Transitions"] = transitions;
		}
		if (p.m_Raster.m_Enabled)
		{
			j["Raster"] = json{
				{ "RenderTargetCount", p.m_Raster.m_RenderTargetCount },
				{ "UseDepthBuffer", p.m_Raster.m_UseDepthBuffer },
				{ "IndirectDraw", p.m_Raster.m_IndirectDraw },
				{ "DepthEnable", p.m_Raster.m_DepthEnable },
				{ "DepthWrite", p.m_Raster.m_DepthWrite },
				{ "DepthCompare", ES::ToString(p.m_Raster.m_DepthCompare) },
				{ "DepthClamp", p.m_Raster.m_DepthClamp },
				{ "UseCulling", p.m_Raster.m_UseCulling },
				{ "CrossQueueExitToCommon", p.m_Raster.m_CrossQueueExitToCommon },
				{ "IndirectArgsBuffer", p.m_Raster.m_IndirectArgsBuffer } };
		}
		if (p.m_UseRaytracing)
			j["UseRaytracing"] = true;
		return j;
	}

	void PassFromJson(const json& j, PassNodeDesc& p)
	{
		p.m_Name = j.value("Name", std::string());
		p.m_Queue = ES::GPUEngineTypeFromString(j.value("Queue", std::string("Compute")));
		if (j.contains("Shader"))
		{
			p.m_ShaderFilePaths.m_VSPath = j["Shader"].value("VS", std::string()).c_str();
			p.m_ShaderFilePaths.m_PSPath = j["Shader"].value("PS", std::string()).c_str();
			p.m_ShaderFilePaths.m_CSPath = j["Shader"].value("CS", std::string()).c_str();
			p.m_ShaderFilePaths.m_RayGenPath = j["Shader"].value("RayGen", std::string()).c_str();
			p.m_ShaderFilePaths.m_AnyHitPath = j["Shader"].value("AnyHit", std::string()).c_str();
			p.m_ShaderFilePaths.m_ClosestHitPath = j["Shader"].value("ClosestHit", std::string()).c_str();
			p.m_ShaderFilePaths.m_MissPath = j["Shader"].value("Miss", std::string()).c_str();
			p.m_ShaderFilePaths.m_ShadowMissPath = j["Shader"].value("ShadowMiss", std::string()).c_str();
		}
		if (j.contains("Reads"))
			for (const auto& r : j["Reads"]) p.m_Reads.push_back(r.get<std::string>());
		if (j.contains("Writes"))
			for (const auto& w : j["Writes"]) p.m_Writes.push_back(w.get<std::string>());
		if (j.contains("Bindings"))
			for (const auto& bj : j["Bindings"]) { BindingDesc b; BindingFromJson(bj, b); p.m_Bindings.push_back(b); }
		if (j.contains("Transitions"))
			for (const auto& tj : j["Transitions"]) { TransitionDesc t; TransitionFromJson(tj, t); p.m_Transitions.push_back(t); }
		if (j.contains("Dispatch"))
		{
			p.m_Dispatch.m_Mode = ES::DispatchModeFromString(j["Dispatch"].value("Mode", std::string("Static")));
			p.m_Dispatch.m_X = j["Dispatch"].value("X", 1u);
			p.m_Dispatch.m_Y = j["Dispatch"].value("Y", 1u);
			p.m_Dispatch.m_Z = j["Dispatch"].value("Z", 1u);
			p.m_Dispatch.m_TileSize = j["Dispatch"].value("TileSize", 0u);
		}
		if (j.contains("Bypass"))
		{
			p.m_BypassEnabled = j["Bypass"].value("Enabled", false);
			p.m_ClearOnBypass = j["Bypass"].value("ClearOnBypass", false);
		}
		if (j.contains("Raster"))
		{
			const auto& rj = j["Raster"];
			p.m_Raster.m_Enabled = true;
			p.m_Raster.m_RenderTargetCount = rj.value("RenderTargetCount", 0u);
			p.m_Raster.m_UseDepthBuffer = rj.value("UseDepthBuffer", false);
			p.m_Raster.m_IndirectDraw = rj.value("IndirectDraw", false);
			p.m_Raster.m_DepthEnable = rj.value("DepthEnable", false);
			p.m_Raster.m_DepthWrite = rj.value("DepthWrite", false);
			p.m_Raster.m_DepthCompare = ES::ComparisionFunctionFromString(rj.value("DepthCompare", std::string("Never")));
			p.m_Raster.m_DepthClamp = rj.value("DepthClamp", false);
			p.m_Raster.m_UseCulling = rj.value("UseCulling", false);
			p.m_Raster.m_CrossQueueExitToCommon = rj.value("CrossQueueExitToCommon", false);
			p.m_Raster.m_IndirectArgsBuffer = rj.value("IndirectArgsBuffer", std::string());
		}
		p.m_TrackWriteState = j.value("TrackWriteState", false);
		p.m_OneShot = j.value("OneShot", false);
		p.m_UseRaytracing = j.value("UseRaytracing", false);
	}
}

void RenderGraphSerializer::to_json(json& j, const RenderGraphDesc& p)
{
	json resources = json::array();
	for (const auto& r : p.m_Resources) resources.push_back(ResourceToJson(r));
	json passes = json::array();
	for (const auto& pass : p.m_Passes) passes.push_back(PassToJson(pass));
	j = json{ { "Name", p.m_Name }, { "Resources", resources }, { "Passes", passes } };
}

void RenderGraphSerializer::from_json(const json& j, RenderGraphDesc& p)
{
	p.m_Name = j.value("Name", std::string());
	if (j.contains("Resources"))
		for (const auto& rj : j["Resources"]) { ResourceDesc r; ResourceFromJson(rj, r); p.m_Resources.push_back(r); }
	if (j.contains("Passes"))
		for (const auto& pj : j["Passes"]) { PassNodeDesc pass; PassFromJson(pj, pass); p.m_Passes.push_back(pass); }
}

bool RenderGraphSerializer::LoadFromFile(const char* fileName, RenderGraphDesc& out)
{
	json j;
	if (!JSONWrapper::Load(fileName, j))
		return false;

	from_json(j, out);
	return true;
}
