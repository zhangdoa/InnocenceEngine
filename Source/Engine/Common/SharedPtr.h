#pragma once
#include <atomic>
#include <cstddef>

namespace Inno
{
	// Engine-native ref-counted shared-ownership smart pointer. Refcount is
	// atomic; m_Object is plain T*. Concurrent reassignment of the SAME
	// instance from multiple threads is UB — same as std::shared_ptr.
	template <typename T>
	class SharedPtr
	{
	public:
		SharedPtr(std::nullptr_t = nullptr)
			: m_Object(nullptr), m_RefCount(nullptr) {}

		explicit SharedPtr(T* object)
			: m_Object(object), m_RefCount(object ? new std::atomic<int>(1) : nullptr) {}

		SharedPtr(const SharedPtr& other)
			: m_Object(other.m_Object), m_RefCount(other.m_RefCount)
		{
			AddRef();
		}

		SharedPtr(SharedPtr&& other) noexcept
			: m_Object(other.m_Object), m_RefCount(other.m_RefCount)
		{
			other.m_Object = nullptr;
			other.m_RefCount = nullptr;
		}

		SharedPtr& operator=(const SharedPtr& other)
		{
			if (this != &other)
			{
				ReleaseRef();
				m_Object = other.m_Object;
				m_RefCount = other.m_RefCount;
				AddRef();
			}
			return *this;
		}

		SharedPtr& operator=(SharedPtr&& other) noexcept
		{
			if (this != &other)
			{
				ReleaseRef();
				m_Object = other.m_Object;
				m_RefCount = other.m_RefCount;
				other.m_Object = nullptr;
				other.m_RefCount = nullptr;
			}
			return *this;
		}

		SharedPtr& operator=(std::nullptr_t)
		{
			ReleaseRef();
			m_Object = nullptr;
			m_RefCount = nullptr;
			return *this;
		}

		SharedPtr& operator=(T* object)
		{
			ReleaseRef();
			m_Object = object;
			m_RefCount = object ? new std::atomic<int>(1) : nullptr;
			return *this;
		}

		~SharedPtr() { ReleaseRef(); }

		T*       operator->()       { return m_Object; }
		const T* operator->() const { return m_Object; }

		T&       GetRef()             { return *m_Object; }
		const T& GetConstRef() const  { return *m_Object; }

		T*       Get()       noexcept { return m_Object; }
		const T* Get() const noexcept { return m_Object; }

		explicit operator bool() const noexcept { return m_Object != nullptr; }
		bool operator!() const noexcept         { return m_Object == nullptr; }

		bool operator==(const SharedPtr& other) const noexcept { return m_Object == other.m_Object; }
		bool operator!=(const SharedPtr& other) const noexcept { return m_Object != other.m_Object; }
		bool operator< (const SharedPtr& other) const noexcept { return m_Object <  other.m_Object; }
		bool operator<=(const SharedPtr& other) const noexcept { return m_Object <= other.m_Object; }
		bool operator> (const SharedPtr& other) const noexcept { return m_Object >  other.m_Object; }
		bool operator>=(const SharedPtr& other) const noexcept { return m_Object >= other.m_Object; }

	private:
		void AddRef() noexcept
		{
			if (m_RefCount)
				m_RefCount->fetch_add(1, std::memory_order_relaxed);
		}

		void ReleaseRef() noexcept
		{
			if (m_RefCount && m_RefCount->fetch_sub(1, std::memory_order_acq_rel) == 1)
			{
				delete m_Object;
				delete m_RefCount;
			}
			m_Object = nullptr;
			m_RefCount = nullptr;
		}

		T*                m_Object;
		std::atomic<int>* m_RefCount;
	};
}
