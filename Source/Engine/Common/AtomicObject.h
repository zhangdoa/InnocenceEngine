#pragma once
#include "STL14.h"

namespace Inno
{
    template <typename T>
    class AtomicObject
    {
    public:
        AtomicObject(const T& other)
        {
            if constexpr (std::is_pointer_v<T>)
                m_Object = other ? other : nullptr;
            else
                m_Object = other;
        }

        AtomicObject(T&& other)
        {
            if constexpr (std::is_pointer_v<T>)
            {
                m_Object = other ? other : nullptr;
                other = nullptr;
            }
            else
            {
                m_Object = std::move(other);
            }
        }

        AtomicObject(const AtomicObject& other)
        {
            SetObject(other.m_Object);
        }

        AtomicObject(AtomicObject&& other)
        {
            MoveObject(std::move(other));
        }

        AtomicObject& operator=(const AtomicObject& other)
        {
            if (this != &other)
                SetObject(other.m_Object);

            return *this;
        }

        AtomicObject& operator=(AtomicObject&& other)
        {
            if (this != &other)
                MoveObject(std::move(other));

            return *this;
        }

        ~AtomicObject() = default;

        T& Get()
        {
            return m_Object;
        }

        const T& Get() const
        {
            return m_Object;
        }

        void DeleteObject()
        {
            if constexpr (std::is_pointer_v<T>)
            {
                {
                    Lock();
                    if (m_Object)
                        delete m_Object;
                    Unlock();
                }

                SetObject(nullptr);
            }
            else
                SetObject(T{});
        }

        explicit operator bool() const
        {
            Lock();
            bool result = static_cast<bool>(m_Object);
            Unlock();
            return result;
        }

        bool operator!() const
        {
            Lock();
            bool result = !m_Object;
            Unlock();
            return result;
        }

    private:
        void Lock() const
        {
            while (m_Lock.test_and_set(std::memory_order_acquire))
            {
            }
        }

        void Unlock() const
        {
            m_Lock.clear(std::memory_order_release);
        }

        void SetObject(T object)
        {
            Lock();
            m_Object = object;
            Unlock();
        }

        void MoveObject(AtomicObject&& other)
        {
            SetObject(other.m_Object);
            other.SetObject(T{});
        }

        T m_Object;
        mutable std::atomic_flag m_Lock = ATOMIC_FLAG_INIT;
    };
}