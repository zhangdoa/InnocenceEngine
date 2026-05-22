#pragma once
#include "AtomicObject.h"

namespace Inno
{
    template <typename T>
    class Handle
    {
    public:
        Handle(std::nullptr_t = nullptr)
            : m_Object(nullptr), m_RefCount(new std::atomic<int>(0)) {}

        Handle(T* object)
            : m_Object(object), m_RefCount(new std::atomic<int>(1)) {}

        Handle(const Handle& other)
            : m_Object(other.m_Object), m_RefCount(other.m_RefCount)
        {
            AddRef();
        }

        Handle(Handle&& other)
            : m_Object(std::move(other.m_Object)), m_RefCount(std::move(other.m_RefCount)) {}

        Handle& operator=(const Handle& other)
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

        Handle& operator=(Handle&& other)
        {
            if (this != &other)
            {
                ReleaseRef();
                m_Object = std::move(other.m_Object);
                m_RefCount = std::move(other.m_RefCount);
            }
            return *this;
        }

        Handle& operator=(std::nullptr_t)
        {
            ReleaseRef();
            m_Object = nullptr;
            m_RefCount = new std::atomic<int>(0);
            return *this;
        }

        Handle& operator=(T* object)
        {
            ReleaseRef();
            m_Object = object;
            m_RefCount = new std::atomic<int>(1);
            return *this;
        }

        ~Handle()
        {
            ReleaseRef();
        }

        T* operator->()
        {
            return m_Object.Get();
        }

        const T* operator->() const
        {
            return m_Object.Get();
        }

        T& GetRef()
        {
            return *m_Object.Get();
        }

        const T& GetConstRef() const
        {
            return *m_Object.Get();
        }

        explicit operator bool() const
        {
            return static_cast<bool>(m_Object);
        }

        bool operator!() const
        {
            return !m_Object;
        }

        bool operator==(const Handle& other) const
        {
            return m_Object == other.m_Object;
        }

        bool operator!=(const Handle& other) const
        {
            return m_Object != other.m_Object;
        }

        bool operator<(const Handle& other) const
        {
            return m_Object < other.m_Object;
        }

        bool operator<=(const Handle& other) const
        {
            return m_Object <= other.m_Object;
        }

        bool operator>(const Handle& other) const
        {
            return m_Object > other.m_Object;
        }

        bool operator>=(const Handle& other) const
        {
            return m_Object >= other.m_Object;
        }

    private:
        void AddRef()
        {
            m_RefCount.Get()->fetch_add(1, std::memory_order_relaxed);
        }

        void ReleaseRef()
        {
            if (m_RefCount && m_RefCount.Get()->fetch_sub(1, std::memory_order_acq_rel) == 1)
            {
                m_Object.DeleteObject();
                m_RefCount.DeleteObject();
            }
        }

        AtomicObject<T*> m_Object;
        AtomicObject<std::atomic<int>*> m_RefCount;
    };
}