#pragma once
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
		// Helper: Trim leading and trailing whitespace from a string_view.
		constexpr std::string_view Trim(std::string_view sv) {
			while (!sv.empty() && (sv.front() == ' ' || sv.front() == '\t'))
				sv.remove_prefix(1);
			while (!sv.empty() && (sv.back() == ' ' || sv.back() == '\t'))
				sv.remove_suffix(1);
			return sv;
		}

		// Splits a string_view by a delimiter (default: comma) and returns trimmed tokens.
		inline std::vector<std::string_view> SplitNames(std::string_view s, char delimiter = ',') {
			std::vector<std::string_view> result;
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

		// Default: an enum is not registered.
		template<typename T>
		struct IsRegisteredEnum : std::false_type {};


		// Traits to allow our generic ToString conversion.
		template <typename EnumT>
		struct InnoEnumTraits;

		// Helper that calls the traits.
		template <typename EnumT>
		inline const char* ToString(EnumT value) {
			const auto& names = InnoEnumTraits<EnumT>::FullNames();
			size_t idx = static_cast<size_t>(value);
			return (idx < names.size()) ? names[idx].c_str() : "Unknown";
		}

	} // namespace Enum
} // namespace Inno

// The macro to declare an enum and specialize its traits.
// This macro declares the enum (in the Inno::Enum namespace) and creates a specialization
// of InnoEnumTraits so that ToString returns "EnumName::Enumerator".
#define INNO_ENUM(EnumName, ...)                                              \
namespace Inno { namespace Enum {                                             \
    enum class EnumName { __VA_ARGS__ };                                       \
    template <>                                                               \
    struct InnoEnumTraits<EnumName> {                                           \
        static const std::vector<std::string_view>& RawNames() {                \
            static const std::vector<std::string_view> rawNames = SplitNames(#__VA_ARGS__); \
            return rawNames;                                                  \
        }                                                                     \
        static const std::vector<std::string>& FullNames() {                    \
            static const std::vector<std::string> fullNames = [](){             \
                std::vector<std::string> names;                               \
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
	/* Mark this enum as registered */                                       \
    template <>                                                               \
    struct IsRegisteredEnum<EnumName> : std::true_type {};                   \
    inline const char* ToString(EnumName value) {                             \
        return Inno::Enum::ToString<EnumName>(value);                         \
    }                                                                         \
} }

// Optional bitwise operators if needed.
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