#pragma once
#include "STL14.h"
#include "IToA.h"

namespace Inno
{
    template <size_t S>
    class FixedSizeString
    {
        static_assert(S > 0, "FixedSizeString size must be positive");

    public:
        FixedSizeString()                                  { m_content[0] = '\0'; }
        FixedSizeString(const char* content)               { copy_from(content); }
        FixedSizeString(const FixedSizeString&)            = default;
        FixedSizeString& operator=(const char* content)    { copy_from(content); return *this; }
        FixedSizeString& operator=(const FixedSizeString&) = default;

        // Integer construction goes through named factories (ToString).
        // Implicit int->string via constructor was silent and surprising.
        FixedSizeString(int32_t) = delete;
        FixedSizeString(int64_t) = delete;

        ~FixedSizeString() = default;

        const char* c_str()  const noexcept { return m_content; }
        const char* begin()  const noexcept { return &m_content[0]; }
        const char* end()    const noexcept { return m_content + std::strlen(m_content); }
        size_t      size()   const noexcept { return std::strlen(m_content); }
        bool        empty() const noexcept  { return m_content[0] == '\0'; }

        static constexpr size_t capacity() noexcept { return S - 1; }

        const char* find(const char* rhs) const { return std::strstr(m_content, rhs); }

        bool operator==(const char* rhs) const noexcept
        {
            return rhs != nullptr && std::strcmp(m_content, rhs) == 0;
        }
        bool operator==(const FixedSizeString& rhs) const noexcept
        {
            return std::strcmp(m_content, rhs.m_content) == 0;
        }
        bool operator!=(const char* rhs) const noexcept        { return !(*this == rhs); }
        bool operator!=(const FixedSizeString& rhs) const noexcept { return !(*this == rhs); }

    private:
        void copy_from(const char* content)
        {
            if (content == nullptr)
            {
                m_content[0] = '\0';
                return;
            }
            size_t i = 0;
            for (; i < S - 1 && content[i] != '\0'; ++i)
                m_content[i] = content[i];
            m_content[i] = '\0';
        }

        char m_content[S];
    };

    // Integer specialisations — explicit factory, no implicit conversions.
    inline FixedSizeString<11> ToString(int32_t content)
    {
        FixedSizeString<11> result;
        char buf[11] = {};
        i32toa_countlut(content, buf);
        return FixedSizeString<11>(buf);
    }

    inline FixedSizeString<20> ToString(int64_t content)
    {
        FixedSizeString<20> result;
        char buf[20] = {};
        i64toa_countlut(content, buf);
        return FixedSizeString<20>(buf);
    }
}

namespace std
{
    template <size_t S>
    struct hash<Inno::FixedSizeString<S>>
    {
        std::size_t operator()(const Inno::FixedSizeString<S>& k) const noexcept
        {
            // djb2
            std::size_t h = 5381;
            for (const char* p = k.c_str(); *p; ++p)
                h = ((h << 5) + h) + static_cast<unsigned char>(*p);
            return h;
        }
    };

    template <size_t S>
    struct less<Inno::FixedSizeString<S>>
    {
        bool operator()(const Inno::FixedSizeString<S>& a, const Inno::FixedSizeString<S>& b) const noexcept
        {
            return std::strcmp(a.c_str(), b.c_str()) < 0;
        }
    };
}
