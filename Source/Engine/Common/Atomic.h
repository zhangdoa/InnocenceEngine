#pragma once
#include "STL14.h"

namespace Inno
{
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
}