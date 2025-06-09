#include "AssimpUtils.h"
#include "assimp/matrix4x4.h"

using namespace Inno;

void AssimpUtils::to_json(nlohmann::json& j, const aiMatrix4x4& m)
{
	j["00"] = m.a1;
	j["01"] = m.a2;
	j["02"] = m.a3;
	j["03"] = m.a4;
	j["10"] = m.b1;
	j["11"] = m.b2;
	j["12"] = m.b3;
	j["13"] = m.b4;
	j["20"] = m.c1;
	j["21"] = m.c2;
	j["22"] = m.c3;
	j["23"] = m.c4;
	j["30"] = m.d1;
	j["31"] = m.d2;
	j["32"] = m.d3;
	j["33"] = m.d4;
}

void AssimpUtils::from_json(const nlohmann::json& j, aiMatrix4x4& m)
{
	m.a1 = j["00"];
	m.a2 = j["01"];
	m.a3 = j["02"];
	m.a4 = j["03"];
	m.b1 = j["10"];
	m.b2 = j["11"];
	m.b3 = j["12"];
	m.b4 = j["13"];
	m.c1 = j["20"];
	m.c2 = j["21"];
	m.c3 = j["22"];
	m.c4 = j["23"];
	m.d1 = j["30"];
	m.d2 = j["31"];
	m.d3 = j["32"];
	m.d4 = j["33"];
}

void AssimpUtils::MergeTransformation(nlohmann::json& j, const aiNode* node)
{
	// TODO: Implement transformation merging if needed
}

void AssimpUtils::DecomposeTransformation(nlohmann::json& j, const aiMatrix4x4& m)
{
	// TODO: Implement transformation decomposition if needed
}
