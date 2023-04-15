#pragma once
#include "STL14.h"
#include "STL17.h"
#include "InnoIToA.h"
#include "InnoAllocator.h"

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

	template <typename T>
	class AtomicReader;
	template <typename T>
	class AtomicWriter;

	template <typename T>
	class Atomic
	{
		friend class AtomicReader<T>;
		friend class AtomicWriter<T>;

	public:
		Atomic() = default;

		Atomic(const Atomic<T> &rhs)
		{
			auto l_reader = AtomicReader(rhs);
			auto l_writer = AtomicWriter(*this);

			auto l_rhs = l_reader.Get();
			auto l_lhs = l_writer.Get();

			*l_lhs = *l_rhs;
		};

		Atomic<T> &operator=(const Atomic<T> &rhs)
		{
			auto l_reader = AtomicReader(rhs);
			auto l_writer = AtomicWriter(*this);

			auto l_rhs = l_reader.Get();
			auto l_lhs = l_writer.Get();

			*l_lhs = *l_rhs;

			return *this;
		}

		~Atomic()
		{
			std::unique_lock<std::shared_mutex> lock{m_Mutex};

			m_condition.wait(lock, [this]() {
				return IsWritable() && IsReadable();
			});
		};

		bool IsReadable()
		{
			return !m_IsWriting;
		}

		bool IsWritable()
		{
			return m_ReaderCount == 0;
		}

	protected:
		const T *StartToRead()
		{
			std::shared_lock<std::shared_mutex> lock{m_Mutex};

			m_condition.wait(lock, [this]() {
				return IsReadable();
			});

			m_ReaderCount++;

			return &m_value;
		}

		T *StartToWrite()
		{
			std::unique_lock<std::shared_mutex> lock{m_Mutex};

			m_condition.wait(lock, [this]() {
				return IsWritable() && IsReadable();
			});

			m_IsWriting = true;

			return &m_value;
		}

		void FinishReading()
		{
			m_ReaderCount--;
			if (m_ReaderCount == 0)
			{
				m_condition.notify_one();
			}
		}

		void FinishWriting()
		{
			m_IsWriting = false;
			m_condition.notify_one();
		}

	private:
		std::atomic<uint64_t> m_ReaderCount = 0;
		std::atomic<bool> m_IsWriting = false;
		std::condition_variable_any m_condition;

		mutable std::shared_mutex m_Mutex;
		T m_value;
	};

	template <typename T>
	class AtomicReader
	{
	public:
		AtomicReader(const Atomic<T> &rhs)
		{
			t = const_cast<Atomic<T> *>(&rhs);
		};

		~AtomicReader()
		{
			t->FinishReading();
		};

		auto Get()
		{
			return t->StartToRead();
		}

	private:
		Atomic<T> *t;
	};

	template <typename T>
	class AtomicWriter
	{
	public:
		AtomicWriter(Atomic<T> &rhs)
		{
			t = &rhs;
		};

		~AtomicWriter()
		{
			t->FinishWriting();
		};

		auto Get()
		{
			return t->StartToWrite();
		}

	private:
		Atomic<T> *t;
	};

	template <typename T>
	class ThreadSafeQueue
	{
	public:
		~ThreadSafeQueue(void)
		{
			invalidate();
		}

		bool tryPop(T &out)
		{
			std::lock_guard<std::shared_mutex> lock{m_mutex};
			if (m_queue.empty() || !m_valid)
			{
				return false;
			}
			out = std::move(m_queue.front());
			m_queue.pop();
			return true;
		}

		bool waitPop(T &out)
		{
			std::unique_lock<std::shared_mutex> lock{m_mutex};
			m_condition.wait(lock, [this]() {
				return !m_queue.empty() || !m_valid;
			});

			if (!m_valid)
			{
				return false;
			}
			out = std::move(m_queue.front());
			m_queue.pop();
			return true;
		}

		void push(const T &value)
		{
			std::lock_guard<std::shared_mutex> lock{m_mutex};
			m_queue.push(value);
			m_condition.notify_one();
		}

		void push(T &&value)
		{
			std::lock_guard<std::shared_mutex> lock{m_mutex};
			m_queue.push(std::move(value));
			m_condition.notify_one();
		}

		bool empty(void) const
		{
			std::shared_lock<std::shared_mutex> lock{m_mutex};
			return m_queue.empty();
		}

		void clear(void)
		{
			std::lock_guard<std::shared_mutex> lock{m_mutex};
			while (!m_queue.empty())
			{
				m_queue.pop();
			}
			m_condition.notify_all();
		}

		bool isValid(void) const
		{
			std::shared_lock<std::shared_mutex> lock{m_mutex};
			return m_valid;
		}

		void invalidate(void)
		{
			std::lock_guard<std::shared_mutex> lock{m_mutex};
			m_valid = false;
			m_condition.notify_all();
		}

		size_t size(void)
		{
			std::shared_lock<std::shared_mutex> lock{m_mutex};
			return m_queue.size();
		}

		std::queue<T> &getRawData(void)
		{
			std::shared_lock<std::shared_mutex> lock{m_mutex};
			return m_queue;
		}

		void setRawData(std::queue<T> &&values)
		{
			std::lock_guard<std::shared_mutex> lock{m_mutex};
			m_queue = values;
		}

	private:
		std::atomic_bool m_valid{true};
		mutable std::shared_mutex m_mutex;
		std::queue<T> m_queue;
		std::condition_variable_any m_condition;
	};

	template <typename T>
	class ThreadSafeVector
	{
	public:
		ThreadSafeVector()
		{
		}

		ThreadSafeVector(const ThreadSafeVector &rhs)
		{
			std::unique_lock<std::shared_mutex> lock{m_mutex};
			m_vector = rhs.m_vector;
		}
		ThreadSafeVector &operator=(const ThreadSafeVector &rhs)
		{
			std::unique_lock<std::shared_mutex> lock{m_mutex};
			m_vector = rhs.m_vector;
			return this;
		}
		ThreadSafeVector(ThreadSafeVector &&rhs)
		{
			std::unique_lock<std::shared_mutex> lock{m_mutex};
			m_vector = std::move(rhs.m_vector);
		}
		ThreadSafeVector &operator=(ThreadSafeVector &&rhs)
		{
			std::unique_lock<std::shared_mutex> lock{m_mutex};
			m_vector = std::move(rhs.m_vector);
			return this;
		}

		~ThreadSafeVector(void)
		{
			invalidate();
		}

		T &operator[](std::size_t pos)
		{
			std::shared_lock<std::shared_mutex> lock{m_mutex};
			return m_vector[pos];
		}

		const T &operator[](std::size_t pos) const
		{
			std::shared_lock<std::shared_mutex> lock{m_mutex};
			return m_vector[pos];
		}

		void reserve(const std::size_t _Newcapacity)
		{
			std::unique_lock<std::shared_mutex> lock{m_mutex};
			m_vector.reserve(_Newcapacity);
		}

		void push_back(T &&value)
		{
			std::unique_lock<std::shared_mutex> lock{m_mutex};
			m_vector.push_back(value);
		}

		template <class... Args>
		void emplace_back(Args &&... values)
		{
			std::unique_lock<std::shared_mutex> lock{m_mutex};
			m_vector.emplace_back(values...);
		}

		auto empty(void) const
		{
			std::shared_lock<std::shared_mutex> lock{m_mutex};
			return m_vector.empty();
		}

		void clear(void)
		{
			std::unique_lock<std::shared_mutex> lock{m_mutex};
			m_vector.clear();
		}

		auto begin(void)
		{
			std::shared_lock<std::shared_mutex> lock{m_mutex};
			return m_vector.begin();
		}

		auto begin(void) const
		{
			std::shared_lock<std::shared_mutex> lock{m_mutex};
			return m_vector.begin();
		}

		auto end(void)
		{
			std::shared_lock<std::shared_mutex> lock{m_mutex};
			return m_vector.end();
		}

		auto end(void) const
		{
			std::shared_lock<std::shared_mutex> lock{m_mutex};
			return m_vector.end();
		}

		void eraseByIndex(size_t index)
		{
			std::unique_lock<std::shared_mutex> lock{m_mutex};
			m_vector.erase(index);
		}

		void eraseByValue(const T &value)
		{
			std::unique_lock<std::shared_mutex> lock{m_mutex};
			m_vector.erase(std::remove(std::begin(m_vector), std::end(m_vector), value), std::end(m_vector));
		}

		typename std::vector<T>::iterator erase(typename std::vector<T>::const_iterator _First, typename std::vector<T>::const_iterator _Last)
		{
			std::unique_lock<std::shared_mutex> lock{m_mutex};
			return m_vector.erase(_First, _Last);
		}

		auto size(void)
		{
			std::shared_lock<std::shared_mutex> lock{m_mutex};
			return m_vector.size();
		}

		void shrink_to_fit()
		{
			std::unique_lock<std::shared_mutex> lock{m_mutex};
			m_vector.shrink_to_fit();
		}

		bool isValid(void) const
		{
			std::shared_lock<std::shared_mutex> lock{m_mutex};
			return m_valid;
		}

		void invalidate(void)
		{
			std::unique_lock<std::shared_mutex> lock{m_mutex};
			m_valid = false;
		}

		std::vector<T> &getRawData(void)
		{
			std::shared_lock<std::shared_mutex> lock{m_mutex};
			return m_vector;
		}

		void setRawData(std::vector<T> &&values)
		{
			std::unique_lock<std::shared_mutex> lock{m_mutex};
			m_vector = values;
		}

	private:
		std::atomic_bool m_valid{true};
		mutable std::shared_mutex m_mutex;
		std::vector<T> m_vector;
	};

	template <typename Key, typename T>
	class ThreadSafeUnorderedMap
	{
	public:
		~ThreadSafeUnorderedMap(void)
		{
			invalidate();
		}

		void reserve(const std::size_t _Newcapacity)
		{
			std::unique_lock<std::shared_mutex> lock{m_mutex};
			m_unordered_map.reserve(_Newcapacity);
			m_condition.notify_all();
		}

		void emplace(Key key, T value)
		{
			std::unique_lock<std::shared_mutex> lock{m_mutex};
			m_unordered_map.emplace(key, value);
			m_condition.notify_one();
		}

		void emplace(std::pair<Key, T> value)
		{
			std::unique_lock<std::shared_mutex> lock{m_mutex};
			m_unordered_map.emplace(value);
			m_condition.notify_one();
		}

		auto begin(void)
		{
			std::shared_lock<std::shared_mutex> lock{m_mutex};
			return m_unordered_map.begin();
		}

		auto begin(void) const
		{
			std::shared_lock<std::shared_mutex> lock{m_mutex};
			return m_unordered_map.begin();
		}

		auto end(void)
		{
			std::shared_lock<std::shared_mutex> lock{m_mutex};
			return m_unordered_map.end();
		}

		auto end(void) const
		{
			std::shared_lock<std::shared_mutex> lock{m_mutex};
			return m_unordered_map.end();
		}

		auto find(const Key &key)
		{
			std::shared_lock<std::shared_mutex> lock{m_mutex};
			return m_unordered_map.find(key);
		}

		auto find(const Key &key) const
		{
			std::shared_lock<std::shared_mutex> lock{m_mutex};
			return m_unordered_map.find(key);
		}

		auto erase(const Key &key)
		{
			std::unique_lock<std::shared_mutex> lock{m_mutex};
			return m_unordered_map.erase(key);
		}

		template <typename PredicateT>
		void erase_if(const PredicateT &predicate)
		{
			std::unique_lock<std::shared_mutex> lock{m_mutex};
			for (auto it = m_unordered_map.begin(); it != m_unordered_map.end();)
			{
				if (predicate(*it))
				{
					it = m_unordered_map.erase(it);
				}
				else
				{
					++it;
				}
			}
		}

		auto clear(void)
		{
			std::unique_lock<std::shared_mutex> lock{m_mutex};
			return m_unordered_map.clear();
		}

		void invalidate(void)
		{
			std::unique_lock<std::shared_mutex> lock{m_mutex};
			m_valid = false;
			m_condition.notify_all();
		}

		auto size(void)
		{
			std::shared_lock<std::shared_mutex> lock{m_mutex};
			return m_unordered_map.size();
		}

		std::unordered_map<Key, T> &getRawData(void)
		{
			std::shared_lock<std::shared_mutex> lock{m_mutex};
			return m_unordered_map;
		}

	private:
		std::atomic_bool m_valid{true};
		mutable std::shared_mutex m_mutex;
		std::unordered_map<Key, T> m_unordered_map;
		std::condition_variable_any m_condition;
	};

	namespace Container
	{
		template <typename U, bool cond>
		using EnableType = typename std::enable_if<cond, U>::type;

		template <typename U, bool cond>
		using DisableType = typename std::enable_if<!cond, U>::type;

		template <class T, bool ThreadSafe = false>
		class Array
		{
		public:
			Array() = default;
			Array(size_t elementCount)
			{
				reserve(elementCount);
			}

			~Array()
			{
				if (m_HeapAddress)
				{
					InnoMemory::Deallocate(m_HeapAddress);
					m_HeapAddress = nullptr;
					m_CurrentFreeIndex = 0;
					m_ElementCount = 0;
					m_ElementSize = 0;
				}
			}

			Array(const Array<T, ThreadSafe> &rhs)
			{
				m_ElementSize = rhs.m_ElementSize;
				m_ElementCount = rhs.m_ElementCount;
				m_HeapAddress = reinterpret_cast<T *>(InnoMemory::Allocate(m_ElementCount * m_ElementSize));
				std::memcpy(m_HeapAddress, rhs.m_HeapAddress, m_ElementCount * m_ElementSize);
				m_CurrentFreeIndex = rhs.m_CurrentFreeIndex;
			}

			Array<T, ThreadSafe> &operator=(const Array<T, ThreadSafe> &rhs)
			{
				m_ElementSize = rhs.m_ElementSize;
				m_ElementCount = rhs.m_ElementCount;
				m_HeapAddress = reinterpret_cast<T *>(InnoMemory::Allocate(m_ElementCount * m_ElementSize));
				std::memcpy(m_HeapAddress, rhs.m_HeapAddress, m_ElementCount * m_ElementSize);
				m_CurrentFreeIndex = rhs.m_CurrentFreeIndex;
				return *this;
			}

			Array(Array<T, ThreadSafe> &&rhs)
			{
				m_ElementSize = rhs.m_ElementSize;
				m_ElementCount = rhs.m_ElementCount;
				m_HeapAddress = rhs.m_HeapAddress;
				rhs.m_HeapAddress = nullptr;
				m_CurrentFreeIndex = rhs.m_CurrentFreeIndex;
			}

			Array<T, ThreadSafe> &operator=(Array<T, ThreadSafe> &&rhs)
			{
				m_ElementSize = rhs.m_ElementSize;
				m_ElementCount = rhs.m_ElementCount;
				m_HeapAddress = rhs.m_HeapAddress;
				rhs.m_HeapAddress = nullptr;
				m_CurrentFreeIndex = rhs.m_CurrentFreeIndex;
				return *this;
			}

			Array(const T *begin, const T *end)
			{
				m_ElementSize = sizeof(T);
				m_ElementCount = (end - begin);
				m_HeapAddress = reinterpret_cast<T *>(InnoMemory::Allocate(m_ElementCount * m_ElementSize));
				std::memcpy(m_HeapAddress, begin, m_ElementCount * m_ElementSize);
				m_CurrentFreeIndex = m_ElementCount;
			}

			template <typename U = T &>
			EnableType<U, ThreadSafe> operator[](size_t pos)
			{
				std::shared_lock<std::shared_mutex> lock{m_Mutex};

				assert(pos < m_ElementCount && "Trying to access out-of-boundary address.");
				assert(pos <= m_CurrentFreeIndex && "Trying to access non-initialized address.");

				return *(m_HeapAddress + pos);
			}

			template <typename U = T &>
			DisableType<U, ThreadSafe> operator[](size_t pos)
			{
				assert(pos < m_ElementCount && "Trying to access out-of-boundary address.");
				assert(pos <= m_CurrentFreeIndex && "Trying to access non-initialized address.");

				return *(m_HeapAddress + pos);
			}

			template <typename U = const T &>
			EnableType<U, ThreadSafe> operator[](size_t pos) const
			{
				std::shared_lock<std::shared_mutex> lock{m_Mutex};

				assert(pos < m_ElementCount && "Trying to access out-of-boundary address.");
				assert(pos <= m_CurrentFreeIndex && "Trying to access non-initialized address.");

				return *(m_HeapAddress + pos);
			}

			template <typename U = const T &>
			DisableType<U, ThreadSafe> operator[](size_t pos) const
			{
				assert(pos < m_ElementCount && "Trying to access out-of-boundary address.");
				assert(pos <= m_CurrentFreeIndex && "Trying to access non-initialized address.");

				return *(m_HeapAddress + pos);
			}

			auto begin()
			{
				return m_HeapAddress;
			}

			auto end()
			{
				return m_HeapAddress + m_ElementCount;
			}

			const auto begin() const
			{
				return m_HeapAddress;
			}

			const auto end() const
			{
				return m_HeapAddress + m_ElementCount;
			}

			const auto capacity() const
			{
				return m_ElementCount;
			}

			const auto size() const
			{
				return m_CurrentFreeIndex;
			}

			auto reserve(size_t elementCount)
			{
				m_ElementSize = sizeof(T);
				m_ElementCount = elementCount;
				m_HeapAddress = reinterpret_cast<T *>(InnoMemory::Allocate(m_ElementCount * m_ElementSize));
				std::memset(m_HeapAddress, 0, m_ElementCount * m_ElementSize);
				m_CurrentFreeIndex = 0;
			}

			auto fulfill()
			{
				std::fill(m_HeapAddress, m_HeapAddress + m_ElementCount, T());
				m_CurrentFreeIndex = m_ElementCount;
			}

			auto clear()
			{
				m_CurrentFreeIndex = 0;
			}

			template <typename U = void>
			EnableType<U, ThreadSafe> emplace_back(const T &value)
			{
				std::unique_lock<std::shared_mutex> lock{m_Mutex};

				assert(m_CurrentFreeIndex <= m_ElementCount && "Heap overflow occurred due to out-of-boundary emplace back operation.");

				*(m_HeapAddress + m_CurrentFreeIndex) = value;
				m_CurrentFreeIndex++;
			}

			template <typename U = void>
			DisableType<U, ThreadSafe> emplace_back(const T &value)
			{
				assert(m_CurrentFreeIndex <= m_ElementCount && "Heap overflow occurred due to out-of-boundary emplace back operation.");

				*(m_HeapAddress + m_CurrentFreeIndex) = value;
				m_CurrentFreeIndex++;
			}

		private:
			T *m_HeapAddress;
			size_t m_ElementSize;
			size_t m_ElementCount;
			size_t m_CurrentFreeIndex;
			std::shared_mutex m_Mutex;
		};

		template <typename T, bool ThreadSafe = false>
		class RingBuffer
		{
		public:
			RingBuffer() = default;
			RingBuffer(size_t elementCount)
			{
				reserve(elementCount);
			}

			~RingBuffer() = default;

			RingBuffer(const RingBuffer<T, ThreadSafe> &rhs)
			{
				m_CurrentElementIndex = rhs.m_CurrentElementIndex;
				m_ElementCount = rhs.m_ElementCount;
				m_isLoopingOverOnce = rhs.m_isLoopingOverOnce;
				m_Array = rhs.m_Array;
			}

			RingBuffer<T, ThreadSafe> &operator=(const RingBuffer<T, ThreadSafe> &rhs)
			{
				m_CurrentElementIndex = rhs.m_CurrentElementIndex;
				m_ElementCount = rhs.m_ElementCount;
				m_isLoopingOverOnce = rhs.m_isLoopingOverOnce;
				m_Array = rhs.m_Array;
				return *this;
			}

			RingBuffer(RingBuffer<T, ThreadSafe> &&rhs)
			{
				m_CurrentElementIndex = rhs.m_CurrentElementIndex;
				m_ElementCount = rhs.m_ElementCount;
				m_isLoopingOverOnce = rhs.m_isLoopingOverOnce;
				m_Array = std::move(rhs.m_Array);
			}

			RingBuffer<T, ThreadSafe> &operator=(RingBuffer<T, ThreadSafe> &&rhs)
			{
				m_CurrentElementIndex = rhs.m_CurrentElementIndex;
				m_ElementCount = rhs.m_ElementCount;
				m_isLoopingOverOnce = rhs.m_isLoopingOverOnce;
				m_Array = std::move(rhs.m_Array);
				return *this;
			}

			template <typename U = T &>
			EnableType<U, ThreadSafe> operator[](size_t pos)
			{
				std::shared_lock<std::shared_mutex> lock{m_Mutex};

				return m_Array[pos % m_ElementCount];
			}

			template <typename U = T &>
			DisableType<U, ThreadSafe> operator[](size_t pos)
			{
				return m_Array[pos % m_ElementCount];
			}

			template <typename U = const T &>
			EnableType<U, ThreadSafe> operator[](size_t pos) const
			{
				std::shared_lock<std::shared_mutex> lock{m_Mutex};

				return m_Array[pos % m_ElementCount];
			}

			template <typename U = const T &>
			DisableType<U, ThreadSafe> operator[](size_t pos) const
			{
				return m_Array[pos % m_ElementCount];
			}

			auto reserve(size_t elementCount)
			{
				m_ElementCount = elementCount;
				m_Array.reserve(m_ElementCount);
				m_Array.fulfill();
			}

			const auto capacity() const
			{
				return m_ElementCount;
			}

			const auto size() const
			{
				if (m_isLoopingOverOnce)
				{
					return m_ElementCount;
				}
				else
				{
					return m_CurrentElementIndex;
				}
			}

			const auto currentElementPos() const
			{
				return m_CurrentElementIndex == 0 ? 0 : m_CurrentElementIndex - 1;
			}

			template <typename U = T &>
			EnableType<U, ThreadSafe> currentElement()
			{
				std::shared_lock<std::shared_mutex> lock{m_Mutex};

				return m_Array[m_CurrentElementIndex == 0 ? 0 : m_CurrentElementIndex - 1];
			}

			template <typename U = T &>
			DisableType<U, ThreadSafe> currentElement()
			{
				return m_Array[m_CurrentElementIndex == 0 ? 0 : m_CurrentElementIndex - 1];
			}

			template <typename U = const T &>
			EnableType<U, ThreadSafe> currentElement() const
			{
				std::shared_lock<std::shared_mutex> lock{m_Mutex};

				return m_Array[m_CurrentElementIndex == 0 ? 0 : m_CurrentElementIndex - 1];
			}

			template <typename U = const T &>
			DisableType<U, ThreadSafe> currentElement() const
			{
				return m_Array[m_CurrentElementIndex == 0 ? 0 : m_CurrentElementIndex - 1];
			}

			template <typename U = void>
			EnableType<U, ThreadSafe> emplace_back(const T &value)
			{
				std::unique_lock<std::shared_mutex> lock{m_Mutex};

				m_Array[m_CurrentElementIndex] = value;

				m_CurrentElementIndex++;

				if (m_CurrentElementIndex >= m_ElementCount)
				{
					m_CurrentElementIndex = 0;
					m_isLoopingOverOnce = true;
				}
			}

			template <typename U = void>
			DisableType<U, ThreadSafe> emplace_back(const T &value)
			{
				m_Array[m_CurrentElementIndex] = value;

				m_CurrentElementIndex++;

				if (m_CurrentElementIndex >= m_ElementCount)
				{
					m_CurrentElementIndex = 0;
					m_isLoopingOverOnce = true;
				}
			}

		private:
			size_t m_CurrentElementIndex = 0;
			size_t m_ElementCount = 0;
			bool m_isLoopingOverOnce = false;
			Array<T, ThreadSafe> m_Array;
			mutable std::shared_mutex m_Mutex;
		};

		template <typename T, bool ThreadSafe = false>
		class DoubleBuffer
		{
		public:
			DoubleBuffer() = default;
			~DoubleBuffer() = default;

			template <typename U = T &>
			EnableType<U, ThreadSafe> GetOldValue()
			{
				std::shared_lock<std::shared_mutex> lock{m_Mutex};

				if (m_IsAReady)
				{
					return m_B;
				}
				else
				{
					return m_A;
				}
			}

			template <typename U = T &>
			DisableType<U, ThreadSafe> GetOldValue()
			{
				if (m_IsAReady)
				{
					return m_B;
				}
				else
				{
					return m_A;
				}
			}

			template <typename U = T &>
			EnableType<U, ThreadSafe> GetNewValue()
			{
				std::shared_lock<std::shared_mutex> lock{m_Mutex};

				if (m_IsAReady)
				{
					return m_A;
				}
				else
				{
					return m_B;
				}
			}

			template <typename U = T &>
			DisableType<U, ThreadSafe> GetNewValue()
			{
				if (m_IsAReady)
				{
					return m_A;
				}
				else
				{
					return m_B;
				}
			}

			template <typename U = void>
			EnableType<U, ThreadSafe> SetValue(T &&value)
			{
				std::unique_lock<std::shared_mutex> lock{m_Mutex};

				if (m_IsAReady)
				{
					m_B = std::move(value);
					m_IsAReady = false;
				}
				else
				{
					m_A = std::move(value);
					m_IsAReady = true;
				}
			}

			template <typename U = void>
			DisableType<U, ThreadSafe> SetValue(T &&value)
			{
				if (m_IsAReady)
				{
					m_B = std::move(value);
					m_IsAReady = false;
				}
				else
				{
					m_A = std::move(value);
					m_IsAReady = true;
				}
			}

			template <typename U = void>
			EnableType<U, ThreadSafe> Reserve(size_t elementCount)
			{
				std::unique_lock<std::shared_mutex> lock{m_Mutex};

				m_A.reserve(elementCount);
				m_B.reserve(elementCount);
			}

			template <typename U = void>
			DisableType<U, ThreadSafe> Reserve(size_t elementCount)
			{
				m_A.reserve(elementCount);
				m_B.reserve(elementCount);
			}

		private:
			std::atomic<bool> m_IsAReady = false;
			mutable std::shared_mutex m_Mutex;
			T m_A;
			T m_B;
		};
	} // namespace Container
} // namespace Inno

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
} // namespace std

using namespace Inno::Container;
