    
#pragma once
#include "STL14.h"
#include "IToA.h"

namespace Inno
{
    template <size_t S>
	class FixedSizeString
	{
	public:
		FixedSizeString() = default;

		FixedSizeString(const FixedSizeString<S> &rhs)
		{
			std::memcpy(m_content, rhs.c_str(), S);
		};

		FixedSizeString<S> &operator=(const FixedSizeString<S> &rhs)
		{
			std::memcpy(m_content, rhs.c_str(), S);

			return *this;
		}

		FixedSizeString(const char *content)
		{
			auto l_sizeOfContent = strlen(content);

			std::memcpy(m_content, content, l_sizeOfContent);
			m_content[l_sizeOfContent - 1] = '\0';
		};

		FixedSizeString<S> &operator=(const char *content)
		{
			auto l_sizeOfContent = strlen(content);

			if (l_sizeOfContent > S)
			{
				l_sizeOfContent = S;
			}

			std::memcpy(m_content, content, l_sizeOfContent);
			m_content[l_sizeOfContent - 1] = '\0';

			return *this;
		}

		FixedSizeString(int32_t content){};

		FixedSizeString(int64_t content){};

		~FixedSizeString() = default;

		bool operator==(const char *rhs) const
		{
			auto l_result = strcmp(m_content, rhs);

			if (l_result != 0)
			{
				return false;
			}
			else
			{
				return true;
			}
		}

		bool operator==(const FixedSizeString<S> &rhs) const
		{
			auto l_rhsCStr = rhs.c_str();

			return (*this == l_rhsCStr);
		}

		bool operator!=(const char *rhs) const
		{
			return !(*this == rhs);
		}

		bool operator!=(const FixedSizeString<S> &rhs) const
		{
			return !(*this == rhs);
		}

		const char *c_str() const
		{
			return &m_content[0];
		}

		const char *begin() const
		{
			return &m_content[0];
		}

		const char *end() const
		{
			return &m_content[S - 1];
		}

		const char *find(const char *rhs) const
		{
			return strstr(&m_content[0], rhs);
		}

		const size_t size() const
		{
			return strlen(m_content);
		}

	private:
		char m_content[S];
	};

	template <>
	inline FixedSizeString<11>::FixedSizeString(int32_t content)
	{
		i32toa_countlut(content, m_content);
	};

	template <>
	inline FixedSizeString<20>::FixedSizeString(int64_t content)
	{
		i64toa_countlut(content, m_content);
	};

	inline FixedSizeString<11> ToString(int32_t content)
	{
		return FixedSizeString<11>(content);
	}

	inline FixedSizeString<20> ToString(int64_t content)
	{
		return FixedSizeString<20>(content);
	}
}

namespace std
{
	template <size_t S>
	struct hash<Inno::FixedSizeString<S>>
	{
		std::size_t operator()(const Inno::FixedSizeString<S> &k) const
		{
			std::size_t h = 5381;
			int32_t c;
			auto l_cStr = k.c_str();
			while ((c = *l_cStr++))
				h = ((h << 5) + h) + c;
			return h;
		}
	};

	template <size_t S>
	struct less<Inno::FixedSizeString<S>>
	{
		bool operator()(const Inno::FixedSizeString<S> &s1, const Inno::FixedSizeString<S> &s2) const
		{
			return strcmp(s1.c_str(), s2.c_str()) < 0;
		}
	};
}