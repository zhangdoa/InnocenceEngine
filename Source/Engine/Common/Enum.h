#pragma once
#include "Array.h"
#include "Config.h"
#include "Metadata.h"
#include "STL14.h"
#include "STL17.h"

#if defined INNO_PLATFORM_WIN
#define INNO_FORCEINLINE __forceinline
#else
#define INNO_FORCEINLINE __attribute__((always_inline)) inline
#endif

namespace Inno {
	namespace Enum {
		constexpr std::string_view Trim(std::string_view sv) {
			while (!sv.empty() && (sv.front() == ' ' || sv.front() == '\t'))
				sv.remove_prefix(1);
			while (!sv.empty() && (sv.back() == ' ' || sv.back() == '\t'))
				sv.remove_suffix(1);
			return sv;
		}

		inline Inno::Array<std::string_view> SplitNames(std::string_view s, char delimiter = ',') {
			Inno::Array<std::string_view> result;
			size_t pos = 0;
			while (pos < s.size()) {
				size_t commaPos = s.find(delimiter, pos);
				if (commaPos == std::string_view::npos)
					commaPos = s.size();
				std::string_view token = s.substr(pos, commaPos - pos);
				token = Trim(token);
				if (!token.empty())
					result.push_back(token);
				pos = commaPos + 1;
			}
			return result;
		}

		template<typename T>
		struct IsRegisteredEnum : std::false_type {};

		template <typename EnumT>
		struct InnoEnumTraits;

		template <typename EnumT>
		inline const char* ToString(EnumT value) {
			const auto& names = InnoEnumTraits<EnumT>::FullNames();
			size_t idx = static_cast<size_t>(value);
			return (idx < names.size()) ? names[idx].c_str() : "Unknown";
		}

	} // namespace Enum
} // namespace Inno

// The underlying `enum class` lives in `Inno::Enum::EnumName` so traits
// specializations can see it; the `using` alias re-exports as `Inno::EnumName`
// for the natural spelling at call sites (including unqualified use inside
// `namespace Inno`).
#define INNO_ENUM(EnumName, ...)                                              \
namespace Inno { namespace Enum {                                             \
    enum class EnumName { __VA_ARGS__ };                                       \
    template <>                                                               \
    struct InnoEnumTraits<EnumName> {                                           \
        static const Inno::Array<std::string_view>& RawNames() {                \
            static const Inno::Array<std::string_view> rawNames = SplitNames(#__VA_ARGS__); \
            return rawNames;                                                  \
        }                                                                     \
        static const Inno::Array<std::string>& FullNames() {                    \
            static const Inno::Array<std::string> fullNames = [](){             \
                Inno::Array<std::string> names;                               \
                auto raw = RawNames();                                        \
                names.reserve(raw.size());                                    \
                for (auto name : raw) {                                       \
                    names.push_back(std::string(#EnumName) + "::" + std::string(name)); \
                }                                                             \
                return names;                                                 \
            }();                                                              \
            return fullNames;                                                 \
        }                                                                     \
    };                                                                        \
    template <>                                                               \
    struct IsRegisteredEnum<EnumName> : std::true_type {};                   \
    inline const char* ToString(EnumName value) {                             \
        return Inno::Enum::ToString<EnumName>(value);                         \
    }                                                                         \
} }                                                                           \
namespace Inno { using EnumName = Enum::EnumName; }

#define INNO_ENUM_OPERATORS(enumTypeName)                                   \
inline enumTypeName operator&(enumTypeName a, enumTypeName b) {               \
    return static_cast<enumTypeName>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b)); \
}                                                                           \
inline enumTypeName& operator&=(enumTypeName& a, enumTypeName b) {            \
    a = static_cast<enumTypeName>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b)); \
    return a;                                                               \
}                                                                           \
inline enumTypeName operator|(enumTypeName a, enumTypeName b) {               \
    return static_cast<enumTypeName>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b)); \
}                                                                           \
inline enumTypeName& operator|=(enumTypeName& a, enumTypeName b) {            \
    a = static_cast<enumTypeName>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b)); \
    return a;                                                               \
}

using namespace Inno::Enum;