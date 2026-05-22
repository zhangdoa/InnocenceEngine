#pragma once
#include "EditorService.h"
#include "../Common/HashMap.h"
#include "../Common/Math.h"
#include "../ThirdParty/JSONWrapper/JSONWrapper.h"

#include <ixwebsocket/IXWebSocket.h>

#include <functional>
#include <initializer_list>
#include <mutex>
#include <stdexcept>
#include <string>
#include <unordered_map>

namespace Inno
{
	class EditorReqError : public std::runtime_error
	{
	public:
		EditorReqError(std::string code, std::string message)
			: std::runtime_error(message), m_Code(std::move(code))
		{
		}
		const std::string& Code() const { return m_Code; }
	private:
		std::string m_Code;
	};

	struct EditorServiceImpl
	{
		using Handler = std::function<json(const json& payload, ix::WebSocket& ws)>;
		std::mutex                                  mutex;
		Inno::HashMap<std::string, Handler>    handlers;
	};

	inline json SerializeVec(const Vec3& v) { return { v.x, v.y, v.z }; }
	inline json SerializeVec(const Vec4& v) { return { v.x, v.y, v.z, v.w }; }

	inline void RequireFields(const json& payload, std::initializer_list<const char*> fields)
	{
		for (auto* f : fields)
		{
			if (!payload.contains(f))
				throw EditorReqError("BAD_PAYLOAD", std::string("missing required field: ") + f);
		}
	}
}
