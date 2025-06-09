#include "JSONWrapper.h"
#include "../../Services/EntityManager.h"
#include "../../Services/ComponentManager.h"

#include "../../Engine.h"
using namespace Inno;

void JSONWrapper::to_json(json& j, const Entity& p)
{
	j = json
	{
		{"UUID", p.m_UUID},
		{"ObjectName", p.m_InstanceName.c_str()},
	};
}

void JSONWrapper::to_json(json& j, const Vec4& p)
{
	j = json
	{
		{
			"X", p.x
		},
		{
			"Y", p.y
		},
		{
			"Z", p.z
		},
		{
			"W", p.w
		}
	};
}

void JSONWrapper::to_json(json& j, const Mat4& p)
{
	j["00"] = p.m00;
	j["01"] = p.m01;
	j["02"] = p.m02;
	j["03"] = p.m03;
	j["10"] = p.m10;
	j["11"] = p.m11;
	j["12"] = p.m12;
	j["13"] = p.m13;
	j["20"] = p.m20;
	j["21"] = p.m21;
	j["22"] = p.m22;
	j["23"] = p.m23;
	j["30"] = p.m30;
	j["31"] = p.m31;
	j["32"] = p.m32;
	j["33"] = p.m33;
}

void JSONWrapper::from_json(const json& j, Mat4& p)
{
	p.m00 = j["00"];
	p.m01 = j["01"];
	p.m02 = j["02"];
	p.m03 = j["03"];
	p.m10 = j["10"];
	p.m11 = j["11"];
	p.m12 = j["12"];
	p.m13 = j["13"];
	p.m20 = j["20"];
	p.m21 = j["21"];
	p.m22 = j["22"];
	p.m23 = j["23"];
	p.m30 = j["30"];
	p.m31 = j["31"];
	p.m32 = j["32"];
	p.m33 = j["33"];
}

void JSONWrapper::to_json(json& j, const TransformVector& p)
{
	j = json
	{
		{
			"Position",
			{
				{
					"X", p.m_pos.x
				},
				{
					"Y", p.m_pos.y
				},
				{
					"Z", p.m_pos.z
				}
			}
		},
		{
			"Rotation",
			{
				{
					"X", p.m_rot.x
				},
				{
					"Y", p.m_rot.y
				},
				{
					"Z", p.m_rot.z
				},
				{
					"W", p.m_rot.w
				}
			}
		},
		{
			"Scale",
			{
				{
					"X", p.m_scale.x
				},
				{
					"Y", p.m_scale.y
				},
				{
					"Z", p.m_scale.z
				},
			}
		}
	};
}

void JSONWrapper::from_json(const json& j, Vec4& p)
{
	p.x = j["X"];
	p.y = j["Y"];
	p.z = j["Z"];
	p.w = j["W"];
}

void JSONWrapper::from_json(const json& j, TransformVector& p)
{
	p.m_pos.x = j["Position"]["X"];
	p.m_pos.y = j["Position"]["Y"];
	p.m_pos.z = j["Position"]["Z"];
	p.m_pos.w = 1.0f;

	p.m_rot.x = j["Rotation"]["X"];
	p.m_rot.y = j["Rotation"]["Y"];
	p.m_rot.z = j["Rotation"]["Z"];
	p.m_rot.w = j["Rotation"]["W"];

	p.m_scale.x = j["Scale"]["X"];
	p.m_scale.y = j["Scale"]["Y"];
	p.m_scale.z = j["Scale"]["Z"];
	p.m_scale.w = 1.0f;
}